/* vim: set filetype=c: */

/* {{{1 TX Q */
static void tx_rt_start(void *opaque);

static void tx_q_done(struct mgos_cc1101_tx_op *op) {
  if (op->req.free_data) free(op->req.data);
  if (op->req.cb && mgos_invoke_cb(op->req.cb, op, false)) return;
  if (op->req.cb) FNERR("error scheduling %s", "TX req final callback");
  free(op);
}

static void tx_q_new(void *opaque) {
  struct mgos_cc1101_tx_op *op = opaque;
  struct cc_tx *tx = &op->cc1101->tx;
  if (tx->q.op) {  // TX op in progress?
    if (xQueueSendToBack(tx->q.bl, &opaque, 0)) return;
    FNERR("TX queue full");
    op->err = CC1101_TX_QUEUE_FULL;
  } else {
    tx->q.op = op;
    if (pq_invoke_cb(&tx->rt.pq, NULL, tx_rt_start, op, false, false)) return;
    tx->q.op = NULL;
    FNERR("error scheduling %s", "TX op RT start");
    op->err = CC1101_TX_RT_BUSY;
  }
  tx_q_done(op);
}

static void tx_q_rt_end(void *opaque) {
  struct mgos_cc1101_tx_op *op = opaque;
  struct cc_tx_q *q = &op->cc1101->tx.q;
  q->op = NULL;
  tx_q_done(op);
  if (xQueueReceive(q->bl, &op, 0)) tx_q_new(op);
}

/* {{{1 TX RT */
/* {{{2 TX stats */
static void tx_st_update(struct mgos_cc1101_distrib *d, uint32_t val) {
  d->tot += val;
  d->cnt++;
  if (d->max < val) d->max = val;
  if (d->min > val) d->min = val;
}

static void tx_st_reset(struct mgos_cc1101_tx_stats *st) {
  st->delay_us = st->feed_us = st->fifo_byt =
      (struct mgos_cc1101_distrib){min : UINT32_MAX};
}

/* {{{2 CC1101 TX register ops */
static bool tx_reg_setup_drate(struct mgos_cc1101 *cc1101,
                               struct mgos_cc1101_regs *regs, float *kbps) {
  TRY_RETF(mgos_cc1101_get_data_rate, cc1101, regs, kbps);
  if (CC1101_REGS_REG(regs, MDMCFG2).MOD_FORMAT == CC1101_MOD_FORMAT_4FSK)
    *kbps *= 2;
  if (CC1101_REGS_REG(regs, MDMCFG2).MANCHESTER_EN) *kbps /= 2;
  return true;
}

static bool tx_reg_setup(struct mgos_cc1101 *cc1101,
                         struct mgos_cc1101_regs *regs, void *opaque) {
  struct cc_tx_rt *rt = &cc1101->tx.rt;

  float kbps;
  if (cc1101->gpio.gdo < 0) {
    rt->timer_us = mgos_sys_config_get_cc1101_timer_us();
    if (rt->timer_us < 0) {
      TRY_RETF(tx_reg_setup_drate, cc1101, regs, &kbps);
      rt->timer_us =
          CC1101_FIFO_SIZE * 8 / 3 * 1000 / kbps;  // ~1/3 FIFO at DRATE.
      FNLOG(LL_DEBUG, "%f kbps → %d us polling", kbps, rt->timer_us);
    }
  } else {
    TRY_RETF(tx_reg_setup_drate, cc1101, regs, &kbps);
    // Poll 20 ms after the whole message should've been sent: may get stuck.
    rt->timer_us = rt->todo * 8 * 1000 / kbps + 20000;
    FNLOG(LL_INFO, "TX sanity poll in %u us", rt->timer_us);
    int fifo_thr = mgos_sys_config_get_cc1101_fifo_thr();
    if (fifo_thr < 1) {
      // 300 us worth of DRATE, 8 bits/byte, 4 bytes/unit of FIFO_THR.
      fifo_thr = 15 - (3 * kbps / 8 / 10 + 2) / 4;
      if (fifo_thr < 0) fifo_thr = 0;
      FNLOG(LL_INFO, "%f kbps → %d FIFO_THR", kbps, fifo_thr);
    }
    CC1101_REGS_ANY(regs, cc1101->gdo_reg)->IOCFG0 =
        (struct CC1101_IOCFG0){GDO0_CFG : 2};
    CC1101_REGS_REG(regs, FIFOTHR).FIFO_THR = fifo_thr;
  }

  rt->infinite = rt->todo > 255;
  uint8_t pkt_len = rt->todo & 255,
          len_cfg = rt->infinite ? CC1101_LENGTH_CONFIG_INFINITE
                                 : CC1101_LENGTH_CONFIG_FIXED;
  CC1101_REGS_REG(regs, PKTLEN).PACKET_LENGTH = pkt_len;
  CC1101_REGS_REG(regs, PKTCTRL0).PKT_FORMAT = CC1101_PKT_FORMAT_FIFO;
  CC1101_REGS_REG(regs, PKTCTRL0).LENGTH_CONFIG = len_cfg;
  FNLOG(LL_DEBUG, "LENGTH_CONFIG %u, PACKET_LENGTH %u", len_cfg, pkt_len);
  return true;
}

static bool tx_reg_tail(struct mgos_cc1101 *cc1101,
                        struct mgos_cc1101_regs *regs, void *opaque) {
  cc1101->tx.rt.infinite = false;
  CC1101_REGS_REG(regs, PKTCTRL0).LENGTH_CONFIG = CC1101_LENGTH_CONFIG_FIXED;
  FNLOG(LL_DEBUG, "LENGTH_CONFIG %u", CC1101_LENGTH_CONFIG_FIXED);
  return true;
}

/* {{{2 CC1101 TX FIFO feeding */
static inline uint8_t tx_fifo_bits(struct cc_tx_rt *rt, uint8_t *buf,
                                   uint8_t room) {
  uint8_t *pos, byte;

  for (pos = buf; room--; *pos++ = byte) {
    uint8_t todo = rt->pos_done;  // Need beyond current data byte.
    byte = *rt->pos++ << todo;    // Remainder of current data byte.
    if (todo && (rt->pos < rt->end || rt->end_len))  // Need & have bits beyond?
      byte |= *rt->pos >> (8 - todo);                // |= them on.
    rt->todo--;                                      // One fewer byte to go.

    if (rt->pos < rt->end ||
        (rt->pos == rt->end && rt->pos_done < rt->end_len))  // More data given?
      todo = 0;
    else {                                      // Out of data.
      todo = (rt->pos_done - rt->end_len) & 7;  // Need beyond given data.
      if (todo) byte &= 255 << todo;            // Clear bits beyond given data.

      if (!rt->todo)  // Done with repetitions?
        room = 0;     // May not be able to fill the FIFO.
      else {
        rt->pos = rt->data;                        // Rewind.
        rt->pos_done = todo;                       // Bits consumed now.
        if (todo) byte |= *rt->pos >> (8 - todo);  // |= them on.
      }
    }
  }

  return pos - buf;
}

static inline uint8_t tx_fifo_bytes(struct cc_tx_rt *rt, uint8_t *buf,
                                    uint8_t room) {
  uint8_t *pos = buf;

  while (room) {
    size_t run = rt->end - rt->pos;
    if (run > room) run = room;
    if (run > rt->todo) run = rt->todo;
    memcpy(pos, rt->pos, run);
    rt->todo -= run;
    pos += run;
    rt->pos += run;
    if (rt->pos < rt->end || !rt->todo) break;
    rt->pos = rt->data;
    room -= run;
  }

  return pos - buf;
}

static bool tx_fifo_feed(struct mgos_cc1101 *cc1101, bool setup) {
  int64_t now = mgos_uptime_micros();
  struct cc_tx_rt *rt = &cc1101->tx.rt;
  if (!setup) {
    tx_st_update(&rt->st.delay_us, now - rt->fed_at);
    spi_start(cc1101);
  }
  rt->fed_at = now;

  struct CC1101_TXBYTES tb = {val : spi_get_reg_gt(cc1101, CC1101_TXBYTES)};
  if (!setup && rt->todo) tx_st_update(&rt->st.fifo_byt, tb.NUM_TXBYTES);
  if (tb.TXFIFO_UNDERFLOW) {
    cc1101->tx.q.op->err = CC1101_TX_FIFO_UNDERFLOW;
    FNERR_GTL(flush, "FIFO underflow");
  }

  uint8_t fill = CC1101_FIFO_SIZE - tb.NUM_TXBYTES;
  if (fill && rt->todo) {
    uint8_t *buf = alloca(fill + 1);
    buf[0] = CC1101_FIFO;
    fill = (rt->end_len ? tx_fifo_bits : tx_fifo_bytes)(rt, buf + 1, fill);
    TRY_RETF(spi_write_regs, cc1101, fill, buf);

    bool req_last_int = !rt->todo && cc1101->gpio.gdo >= 0,
         set_fix_len = rt->infinite && rt->todo < 256 - CC1101_FIFO_SIZE;
    if ((req_last_int || set_fix_len || setup) && fill > 1) spi_restart(cc1101);
    if (req_last_int) {
      struct CC1101_ANY ic = {IOCFG0 : {GDO0_CFG : 6}};
      TRY_GT(spi_write_reg, cc1101, cc1101->gdo_reg, ic.val);
    }
    if (set_fix_len)
      TRY_GT(spi_mod_regs, cc1101, CC1101_PKTCTRL0, CC1101_PKTCTRL0,
             tx_reg_tail, NULL);
  }

  if (!setup) {
    spi_stop(cc1101);
    tx_st_update(&rt->st.feed_us, mgos_uptime_micros() - now);
  }
  if (rt->todo + tb.NUM_TXBYTES > 0) return true;
  return (cc1101->tx.q.op->err = CC1101_TX_OK);

err:
  spi_restart(cc1101);
  spi_write_cmd(cc1101, CC1101_SIDLE);
flush:
  spi_write_cmd(cc1101, CC1101_SFTX);
  if (!setup) spi_stop(cc1101);
  return false;
}
/* }}}2 */

static void tx_rt_end(struct mgos_cc1101 *cc1101) {
  struct cc_tx_rt *rt = &cc1101->tx.rt;
  struct mgos_cc1101_tx_op *op = cc1101->tx.q.op;
  rt->data = NULL;
  if (rt->timer_id) mgos_clear_timer(rt->timer_id);
  if (cc1101->gpio.gdo >= 0)
    mgos_gpio_remove_int_handler(cc1101->gpio.gdo, NULL, NULL);
  op->st = rt->st;
  FNLOG(rt->todo ? LL_ERROR : LL_DEBUG, "TX end (%u B unsent, FIFO min %u B)",
        rt->todo, rt->st.fifo_byt.min);
  if (!pq_invoke_cb(&cc1101->tx.q.pq, NULL, tx_q_rt_end, op, false, true))
    FNERR("error scheduling %s", "TX op RT end; TX stuck/op mem leaked");
}

static void tx_rt_poll(void *opaque) {
  struct mgos_cc1101 *cc1101 = opaque;
  if (cc1101->tx.rt.data && !tx_fifo_feed(cc1101, false)) tx_rt_end(cc1101);
}

static IRAM void tx_rt_tmr(void *opaque) {
  struct mgos_cc1101 *cc1101 = opaque;
  pq_invoke_cb(&cc1101->tx.rt.pq, NULL, tx_rt_poll, opaque, true, false);
}

static IRAM void tx_rt_intr(int gpio, void *opaque) {
  tx_rt_tmr(opaque);
}

static void tx_rt_start(void *opaque) {
  struct mgos_cc1101_tx_op *op = opaque;
  struct mgos_cc1101 *cc1101 = op->cc1101;
  struct cc_tx_rt *rt = &cc1101->tx.rt;

  bool ok = false;
  op->err = CC1101_TX_SYS_ERR;
  spi_start(cc1101);
  struct CC1101_MARCSTATE ms = {val : spi_get_reg_gt(cc1101, CC1101_MARCSTATE)};
  if (ms.MARC_STATE != CC1101_MARC_STATE_IDLE) {
    op->err = CC1101_TX_HW_NOT_READY;
    FNERR_GT("MARC_STATE %u, not IDLE (%u)", ms.MARC_STATE,
             CC1101_MARC_STATE_IDLE);
  }
  TRY_GT(spi_write_cmd, cc1101, CC1101_SFTX);

  rt->data = rt->pos = op->req.data;
  rt->pos_done = 0;
  rt->end = rt->data + op->req.len / 8;
  rt->end_len = op->req.len & 7;
  rt->todo = ((op->req.copies + 1) * op->req.len + 7) / 8;
  tx_st_reset(&rt->st);

  FNLOG(LL_DEBUG, "TX start (%u*%u b → %u B)", op->req.copies + 1, op->req.len,
        rt->todo);
  TRY_GT(spi_mod_regs, cc1101, CC1101_IOCFG2, CC1101_MDMCFG2, tx_reg_setup,
         NULL);
  spi_restart(cc1101);

  if (cc1101->gpio.gdo >= 0) {
    TRY_GT(mgos_gpio_set_int_handler_isr, cc1101->gpio.gdo,
           MGOS_GPIO_INT_EDGE_NEG, tx_rt_intr, cc1101);
    TRY_GT(mgos_gpio_enable_int, cc1101->gpio.gdo);
  }

  TRY_GT(tx_fifo_feed, cc1101, true);

  rt->timer_id = mgos_set_hw_timer(
      rt->timer_us, cc1101->gpio.gdo >= 0 ? 0 : MGOS_TIMER_REPEAT, tx_rt_tmr,
      cc1101);
  if (!rt->timer_id) FNERR_GT(CALL_FAILED(mgos_set_hw_timer));

  ok = TRY_GT(spi_write_cmd, cc1101, CC1101_STX);
  op->err = CC1101_TX_INCOMPLETE;

err:
  spi_stop(cc1101);
  if (ok) return;
  tx_rt_end(cc1101);
}

/* {{{1 TX tasks/queue setup */
static bool tx_sys_init(struct cc_tx *tx) {
  tx->q.op = NULL;
  tx->rt.data = NULL;

  tx->q.bl = xQueueCreate(32, sizeof(struct mgos_cc1101_tx_op *));
  if (!tx->q.bl) FNERR_RETF(CALL_FAILED(xQueueCreate));

  pq_set_defaults(&tx->q.pq);
  tx->q.pq.name = "CC1101 PQ";
  tx->q.pq.prio = MGOS_TASK_PRIORITY + 1;
  TRY_GT(pq_start, &tx->q.pq);

  pq_set_defaults(&tx->rt.pq);
  tx->rt.pq.name = "CC1101 RT";
  tx->rt.pq.prio = configMAX_PRIORITIES - 1;
  TRY_GT(pq_start, &tx->rt.pq);

  return true;
err:
  vQueueDelete(tx->q.bl);
  return false;
}
/* }}}1 */

/*
 * [MGOS]             [TX Q PQ]        [TX RT PQ]      [ISR]
 * mgos_cc1101_tx() → tx_q_new() →→→→→ tx_rt_start() → tx_rt_intr/tmr()
 *                    ↓    ↓  ↑        ↓               ↓
 *                    ↓    ↓  ↑        ↓ tx_rt_poll() ←'
 *                    ↓  q.bl ↑        ↓ ↓
 *                    ↓    ↓  ↑        tx_rt_end()
 *                    ↓ tx_q_rt_end() ←'
 *                    ↓ ↓
 *        req->cb() ← tx_q_done()
 *
 *        	      sync: tx.q.op    sync: tx.rt.data
 */
bool mgos_cc1101_tx(struct mgos_cc1101 *cc1101,
                    struct mgos_cc1101_tx_req *req) {
  if (!req->data) FNERR_RETF("no data given");
  if (req->len < 8) FNERR_RETF("min supported len is 8 bits");
  struct mgos_cc1101_tx_op *op = TRY_MALLOC_OR(return false, op);
  op->cc1101 = cc1101;
  op->err = CC1101_TX_UNHANDLED;
  op->req = *req;
  bool ok = pq_invoke_cb(&cc1101->tx.q.pq, NULL, tx_q_new, op, false, false);
  if (!ok) free(op);
  return ok;
}
