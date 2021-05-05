/* vim: set filetype=c: */

static void distrib_update(struct mgos_cc1101_distrib *d, uint32_t val) {
  d->tot += val;
  d->cnt++;
  if (d->max < val) d->max = val;
  if (d->min > val) d->min = val;
}

static bool mgos_cc1101_tx_reg_setup_drate(struct mgos_cc1101 *cc1101,
                                           struct mgos_cc1101_regs *regs,
                                           float *kbps) {
  TRY_RETF(mgos_cc1101_get_data_rate, cc1101, regs, kbps);
  if (CC1101_REGS_REG(regs, MDMCFG2).MOD_FORMAT == CC1101_MOD_FORMAT_4FSK)
    *kbps *= 2;
  if (CC1101_REGS_REG(regs, MDMCFG2).MANCHESTER_EN) *kbps /= 2;
  return true;
}

static bool mgos_cc1101_tx_reg_setup(struct mgos_cc1101 *cc1101,
                                     struct mgos_cc1101_regs *regs,
                                     void *opaque) {
  struct mgos_cc1101_tx *tx = &cc1101->tx;

  float kbps;
  if (cc1101->gpio.gdo < 0) {
    tx->timer_us = mgos_sys_config_get_cc1101_timer_us();
    if (tx->timer_us < 0) {
      TRY_RETF(mgos_cc1101_tx_reg_setup_drate, cc1101, regs, &kbps);
      tx->timer_us =
          CC1101_FIFO_SIZE * 8 / 3 * 1000 / kbps;  // ~1/3 FIFO at DRATE.
      FNLOG(LL_INFO, "%f kbps → %d us polling", kbps, tx->timer_us);
    }
  } else {
    TRY_RETF(mgos_cc1101_tx_reg_setup_drate, cc1101, regs, &kbps);
    // Poll 20 ms after the whole message should've been sent: may get stuck.
    tx->timer_us = tx->todo * 8 * 1000 / kbps + 20000;
    FNLOG(LL_INFO, "TX sanity poll in %u us", tx->timer_us);
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

  tx->infinite = tx->todo > 255;
  uint8_t pkt_len = tx->todo & 255,
          len_cfg = tx->infinite ? CC1101_LENGTH_CONFIG_INFINITE
                                 : CC1101_LENGTH_CONFIG_FIXED;
  CC1101_REGS_REG(regs, PKTLEN).PACKET_LENGTH = pkt_len;
  CC1101_REGS_REG(regs, PKTCTRL0).LENGTH_CONFIG = len_cfg;
  FNLOG(LL_DEBUG, "LENGTH_CONFIG %u, PACKET_LENGTH %u", len_cfg, pkt_len);
  return true;
}

static bool mgos_cc1101_tx_reg_tail(struct mgos_cc1101 *cc1101,
                                    struct mgos_cc1101_regs *regs,
                                    void *opaque) {
  cc1101->tx.infinite = false;
  CC1101_REGS_REG(regs, PKTCTRL0).LENGTH_CONFIG = CC1101_LENGTH_CONFIG_FIXED;
  FNLOG(LL_DEBUG, "LENGTH_CONFIG %u", CC1101_LENGTH_CONFIG_FIXED);
  return true;
}

static inline uint8_t mgos_cc1101_tx_fifo_fill_bits(struct mgos_cc1101_tx *tx,
                                                    uint8_t *buf,
                                                    uint8_t room) {
  uint8_t *pos, byte;

  for (pos = buf; room--; *pos++ = byte) {
    uint8_t todo = tx->pos_done;  // Need beyond current data byte.
    byte = *tx->pos++ << todo;    // Remainder of current data byte.
    if (todo && (tx->pos < tx->end || tx->end_len))  // Need & have bits beyond?
      byte |= *tx->pos >> (8 - todo);                // |= them on.
    tx->todo--;                                      // One fewer byte to go.

    if (tx->pos < tx->end ||
        (tx->pos == tx->end && tx->pos_done < tx->end_len))  // More data given?
      todo = 0;
    else {                                      // Out of data.
      todo = (tx->pos_done - tx->end_len) & 7;  // Need beyond given data.
      if (todo) byte &= 255 << todo;            // Clear bits beyond given data.

      if (!tx->todo)  // Done with repetitions?
        room = 0;     // May not be able to fill the FIFO.
      else {
        tx->pos = tx->data;                        // Rewind.
        tx->pos_done = todo;                       // Bits consumed now.
        if (todo) byte |= *tx->pos >> (8 - todo);  // |= them on.
      }
    }
  }

  return pos - buf;
}

static inline uint8_t mgos_cc1101_tx_fifo_fill_bytes(struct mgos_cc1101_tx *tx,
                                                     uint8_t *buf,
                                                     uint8_t room) {
  uint8_t *pos = buf;

  while (room) {
    size_t run = tx->end - tx->pos;
    if (run > room) run = room;
    if (run > tx->todo) run = tx->todo;
    memcpy(pos, tx->pos, run);
    tx->todo -= run;
    pos += run;
    tx->pos += run;
    if (tx->pos < tx->end || !tx->todo) break;
    tx->pos = tx->data;
    room -= run;
  }

  return pos - buf;
}

static bool mgos_cc1101_tx_fifo_feed(struct mgos_cc1101 *cc1101, bool setup) {
  int64_t now = mgos_uptime_micros();
  struct mgos_cc1101_tx *tx = &cc1101->tx;
  if (!setup) {
    distrib_update(&tx->st.delay_us, now - tx->st.last_fed);
    spi_start(cc1101);
  }
  tx->st.last_fed = now;

  struct CC1101_TXBYTES tb = {val : spi_get_reg_gt(cc1101, CC1101_TXBYTES)};
  if (!setup && tx->todo) distrib_update(&tx->st.fifo_byt, tb.NUM_TXBYTES);
  if (tb.TXFIFO_UNDERFLOW) {
    tx->st.underflow = true;
    FNERR_GTL(flush, "FIFO underflow");
  }

  uint8_t fill = CC1101_FIFO_SIZE - tb.NUM_TXBYTES;
  if (fill && tx->todo) {
    uint8_t *buf = alloca(fill + 1);
    buf[0] = CC1101_FIFO;
    fill = (tx->end_len ? mgos_cc1101_tx_fifo_fill_bits
                        : mgos_cc1101_tx_fifo_fill_bytes)(tx, buf + 1, fill);
    TRY_RETF(spi_write_regs, cc1101, fill, buf);

    bool req_last_int = !tx->todo && cc1101->gpio.gdo >= 0,
         set_fix_len = tx->infinite && tx->todo < 256 - CC1101_FIFO_SIZE;
    if ((req_last_int || set_fix_len || setup) && fill > 1) spi_restart(cc1101);
    if (req_last_int) {
      struct CC1101_ANY ic = {IOCFG0 : {GDO0_CFG : 6}};
      TRY_GT(spi_write_reg, cc1101, cc1101->gdo_reg, ic.val);
    }
    if (set_fix_len)
      TRY_GT(spi_mod_regs, cc1101, CC1101_PKTCTRL0, CC1101_PKTCTRL0,
             mgos_cc1101_tx_reg_tail, NULL);
  }

  if (!setup) {
    spi_stop(cc1101);
    distrib_update(&tx->st.feed_us, mgos_uptime_micros() - now);
  }
  return tx->todo + tb.NUM_TXBYTES > 0;

err:
  spi_restart(cc1101);
  spi_write_cmd(cc1101, CC1101_SIDLE);
flush:
  spi_write_cmd(cc1101, CC1101_SFTX);
  if (!setup) spi_stop(cc1101);
  return false;
}

static void mgos_cc1101_tx_end(struct mgos_cc1101 *cc1101, bool own_data) {
  if (cc1101->tx.timer_id) mgos_clear_timer(cc1101->tx.timer_id);
  if (cc1101->gpio.gdo >= 0)
    mgos_gpio_remove_int_handler(cc1101->gpio.gdo, NULL, NULL);
  if (own_data) free(cc1101->tx.data);
  cc1101->tx.data = NULL;
  cc1101->tx.st.busy = false;
  cc1101->tx.st.ok = !cc1101->tx.st.underflow && !cc1101->tx.todo;
}

static void mgos_cc1101_tx_poll(void *opaque) {
  struct mgos_cc1101 *cc1101 = opaque;
  struct mgos_cc1101_tx *tx = &cc1101->tx;
  if (tx->data && !mgos_cc1101_tx_fifo_feed(cc1101, false)) {
    mgos_cc1101_tx_end(cc1101, true);
    FNLOG(LL_INFO, "TX end (%u B unsent, FIFO min %u B%s)", tx->todo,
          tx->st.fifo_byt.min, !tx->st.underflow ? "" : ", underflow");
  }
}

static IRAM void mgos_cc1101_tx_timer(void *opaque) {
  struct mgos_cc1101 *cc1101 = opaque;
  pq_invoke_cb(&cc1101->tx.pq, NULL, mgos_cc1101_tx_poll, opaque, true, false);
}

static IRAM void mgos_cc1101_tx_int(int gpio, void *opaque) {
  mgos_cc1101_tx_timer(opaque);
}

static void mgos_cc1101_tx_st_reset(struct mgos_cc1101_tx_stats *st) {
  st->busy = true;
  st->ok = false;
  st->underflow = false;
  st->last_fed = 0;
  st->delay_us = st->feed_us = st->fifo_byt =
      (struct mgos_cc1101_distrib){min : UINT32_MAX};
}

static void mgos_cc1101_tx_setup(void *opaque) {
  struct mgos_cc1101 *cc1101 = opaque;
  struct mgos_cc1101_tx *tx = &cc1101->tx;

  bool ok = false;
  spi_start(cc1101);
  TRY_GT(mgos_cc1101_tx_fifo_feed, cc1101, true);
  TRY_GT(spi_write_cmd, cc1101, CC1101_STX);
  if (cc1101->gpio.gdo < 0)
    tx->timer_id = TRY_GT(mgos_set_hw_timer, tx->timer_us, MGOS_TIMER_REPEAT,
                          mgos_cc1101_tx_timer, cc1101);
  ok = true;
err:
  spi_stop(cc1101);
  if (!ok) mgos_cc1101_tx_end(cc1101, true);
}

bool mgos_cc1101_tx(struct mgos_cc1101 *cc1101, size_t len, uint8_t *data,
                    size_t copies) {
  if (len < 8) FNERR_RETF("min supported len is 8 bits");
  struct mgos_cc1101_tx *tx = &cc1101->tx;
  if (tx->data) FNERR_RETF("CC1101 TX busy");
  if (!tx->pq.task) TRY_RETF(pq_start, &tx->pq);

  bool ok = false;
  spi_start(cc1101);
  struct CC1101_MARCSTATE ms = {val : spi_get_reg_gt(cc1101, CC1101_MARCSTATE)};
  if (ms.MARC_STATE != CC1101_MARC_STATE_IDLE)
    FNERR_GT("MARC_STATE %u, not IDLE (%u)", ms.MARC_STATE,
             CC1101_MARC_STATE_IDLE);

  tx->data = tx->pos = data;
  tx->pos_done = 0;
  tx->end = data + len / 8;
  tx->end_len = len & 7;
  tx->todo = ((copies + 1) * len + 7) / 8;
  mgos_cc1101_tx_st_reset(&tx->st);

  FNLOG(LL_INFO, "TX start (%u*%u b → %u B)", copies + 1, len, tx->todo);
  TRY_GT(spi_mod_regs, cc1101, CC1101_IOCFG2, CC1101_MDMCFG2,
         mgos_cc1101_tx_reg_setup, NULL);
  spi_restart(cc1101);

  if (cc1101->gpio.gdo >= 0) {
    TRY_GT(mgos_gpio_set_int_handler_isr, cc1101->gpio.gdo,
           MGOS_GPIO_INT_EDGE_NEG, mgos_cc1101_tx_int, cc1101);
    TRY_GT(mgos_gpio_enable_int, cc1101->gpio.gdo);
    tx->timer_id = TRY_GT(mgos_set_hw_timer, tx->timer_us, 0,
                          mgos_cc1101_tx_timer, cc1101);
  }

  ok = TRY_GT(mgos_set_timer, 0, MGOS_TIMER_RUN_NOW, mgos_cc1101_tx_setup,
              cc1101);

err:
  spi_stop(cc1101);
  if (!ok) mgos_cc1101_tx_end(cc1101, false);
  return ok;
}

const struct mgos_cc1101_tx_stats *mgos_cc1101_tx_stats(
    const struct mgos_cc1101 *cc1101) {
  return &cc1101->tx.st;
}
