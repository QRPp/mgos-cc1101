/* vim: set filetype=c: */

static bool cc1101_spi_read_regs(const struct mgos_cc1101 *cc1101, uint8_t cnt,
                                 uint8_t *trx);
static void cc1101_spi_restart(const struct mgos_cc1101 *cc1101);
static bool cc1101_spi_write_regs(const struct mgos_cc1101 *cc1101, uint8_t cnt,
                           uint8_t *tx);

#define spi_get_reg_gt(cc1101, reg)                 \
  ({                                                \
    uint8_t val;                                    \
    TRY_GT(cc1101_spi_read_reg, cc1101, reg, &val); \
    val;                                            \
  })

static bool cc1101_spi_mod_regs(struct mgos_cc1101 *cc1101, cc1101_reg_t from,
                         cc1101_reg_t to, mgos_cc1101_mod_regs_cb cb,
                         void *opaque) {
  if (to < from) FNERR_RETF("to < from");
  size_t cnt = to - from + 1;
  struct CC1101_ANY any[cnt + 1], copy[cnt];

  /* Read. */
  any[0].reg = from;
  TRY_RETF(cc1101_spi_read_regs, cc1101, cnt, &any[0].val);

  /* Modify. */
  memcpy(copy, any + 1, sizeof(copy));
  struct mgos_cc1101_regs regs = {any : any, from : from, to : to};
  TRY_RETF(cb, cc1101, &regs, opaque);

  /* Changes made? */
  if (memcmp(copy, any + 1, sizeof(copy))) {
    /* Restart SPI after burst access. */
    if (cnt > 1) cc1101_spi_restart(cc1101);
    /* Write. */
    any[0].reg = from;
    TRY_RETF(cc1101_spi_write_regs, cc1101, cnt, &any[0].val);
  }

  return true;
}

static inline int cc1101_spi_spd(const struct mgos_cc1101 *cc1101,
                                 uint8_t len) {
  return len > 2 ? cc1101->spd->many
                 : len > 1 ? cc1101->spd->two : cc1101->spd->one;
}

static inline void cc1101_spi_start(const struct mgos_cc1101 *cc1101) {
  mgos_gpio_write(cc1101->gpio.cs, false);
  while (mgos_gpio_read(cc1101->gpio.so)) (void) 0;
}

static inline void cc1101_spi_stop(const struct mgos_cc1101 *cc1101) {
  mgos_gpio_write(cc1101->gpio.cs, true);
}

static inline void cc1101_spi_restart(const struct mgos_cc1101 *cc1101) {
  cc1101_spi_stop(cc1101);
  cc1101_spi_start(cc1101);
}

static inline bool cc1101_spi_write(const struct mgos_cc1101 *cc1101,
                                    uint8_t len, uint8_t *tx) {
  struct mgos_spi_txn txn = {
    cs : -1,
    freq : cc1101_spi_spd(cc1101, len),
    hd : {tx_len : len, tx_data : tx}
  };
  return mgos_spi_run_txn(cc1101->spi, false, &txn);
}

static bool cc1101_spi_read_reg(const struct mgos_cc1101 *cc1101, uint8_t reg,
                         uint8_t *val) {
  reg |= reg < CC1101_PARTNUM ? CC1101_READ : CC1101_READ_STATUS;
  uint8_t trx[2] = {reg};
  struct mgos_spi_txn txn = {
    cs : -1,
    freq : cc1101_spi_spd(cc1101, sizeof(trx)),
    fd : {len : sizeof(trx), rx_data : &trx, tx_data : &trx}
  };
  if (!mgos_spi_run_txn(cc1101->spi, true, &txn)) return false;
  *val = trx[1];
  return true;
}

static bool cc1101_spi_read_regs(const struct mgos_cc1101 *cc1101, uint8_t cnt,
                                 uint8_t *trx) {
  trx[0] |= cnt > 1 ? CC1101_READ_BURST : CC1101_READ;
  struct mgos_spi_txn txn = {
    cs : -1,
    freq : cc1101_spi_spd(cc1101, cnt + 1),
    fd : {len : cnt + 1, rx_data : trx, tx_data : trx}
  };
  return mgos_spi_run_txn(cc1101->spi, true, &txn);
}

static inline bool cc1101_spi_write_cmd(const struct mgos_cc1101 *cc1101,
                                        uint8_t cmd) {
  return cc1101_spi_write(cc1101, sizeof(cmd), &cmd);
}

static bool cc1101_spi_write_reg(const struct mgos_cc1101 *cc1101, uint8_t reg,
                          uint8_t val) {
  uint8_t tx[] = {reg | CC1101_WRITE, val};
  return cc1101_spi_write(cc1101, sizeof(tx), tx);
}

static bool cc1101_spi_write_regs(const struct mgos_cc1101 *cc1101, uint8_t cnt,
                           uint8_t *tx) {
  tx[0] |= cnt > 1 ? CC1101_WRITE_BURST : CC1101_WRITE;
  return cc1101_spi_write(cc1101, cnt + 1, tx);
}
