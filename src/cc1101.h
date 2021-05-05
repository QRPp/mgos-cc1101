#pragma once

#include <mgos_spi.h>

#include <mgos-cc1101.h>
#include <pq.h>

struct mgos_cc1101 {
  struct {
    int cs, gdo, so;
  } gpio;
  uint8_t gdo_reg;
  uint32_t osc_khz;
  struct mgos_spi *spi;
  const struct mgos_config_cc1101_spi_spd *spd;
  struct mgos_cc1101_tx {
    pq_handle pq;
    size_t todo;
    uint8_t *data, *end, *pos;
    uint8_t end_len, pos_done;
    bool infinite;
    mgos_timer_id timer_id;
    int timer_us;
    struct mgos_cc1101_tx_stats st;
  } tx;
};

#define spi_get_reg_gt(cc1101, reg)                 \
  ({                                                \
    uint8_t val;                                    \
    TRY_GT(cc1101_spi_read_reg, cc1101, reg, &val); \
    val;                                            \
  })

#ifdef __cplusplus
extern "C" {
#endif

bool cc1101_spi_mod_regs(struct mgos_cc1101 *cc1101, cc1101_reg_t from,
                         cc1101_reg_t to, mgos_cc1101_mod_regs_cb cb,
                         void *opaque);
bool cc1101_spi_read_reg(const struct mgos_cc1101 *cc1101, uint8_t reg,
                         uint8_t *val);

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

static inline bool cc1101_spi_write_cmd(const struct mgos_cc1101 *cc1101,
                                        uint8_t cmd) {
  return cc1101_spi_write(cc1101, sizeof(cmd), &cmd);
}

bool cc1101_spi_write_reg(const struct mgos_cc1101 *cc1101, uint8_t reg,
                          uint8_t val);
bool cc1101_spi_write_regs(const struct mgos_cc1101 *cc1101, uint8_t cnt,
                           uint8_t *tx);

#ifdef __cplusplus
}
#endif
