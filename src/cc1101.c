#include <math.h>
#include <stdlib.h>

#include <mgos_config.h>
#include <mgos_gpio.h>
#include <mgos_spi.h>
#include <mgos_system.h>
#include <mgos_time.h>
#include <mgos_timers.h>
#include <pq.h>

#include <mgos-helpers/log.h>

#include <mgos-cc1101.h>

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

#include "cc1101_spi.inc"
#include "cc1101_regs.inc"
#include "cc1101_singleton.inc"
#include "cc1101_tx.inc"

struct mgos_cc1101 *mgos_cc1101_create(int cs, int gdo0_gpio, int gdo2_gpio) {
  struct mgos_spi *spi = mgos_spi_get_global();
  const struct mgos_config_spi *spi_cfg = mgos_sys_config_get_spi();
  if (!spi || !spi_cfg) return NULL;

  cs = cs == 0 ? spi_cfg->cs0_gpio
               : cs == 1 ? spi_cfg->cs1_gpio : cs == 2 ? spi_cfg->cs2_gpio : -1;
  if (cs < 0) return NULL;

  struct mgos_cc1101 *cc1101 = malloc(sizeof(*cc1101));
  if (!cc1101) return NULL;
  cc1101->gpio.cs = cs;
  if (gdo0_gpio >= 0) {
    cc1101->gpio.gdo = gdo0_gpio;
    cc1101->gdo_reg = CC1101_IOCFG0;
  } else if (gdo2_gpio >= 0) {
    cc1101->gpio.gdo = gdo2_gpio;
    cc1101->gdo_reg = CC1101_IOCFG2;
  } else {
    cc1101->gpio.gdo = -1;
    cc1101->gdo_reg = UINT8_MAX;
  }
  cc1101->gpio.so = spi_cfg->miso_gpio;
  cc1101->osc_khz = mgos_sys_config_get_cc1101_osc_khz();
  cc1101->spi = spi;
  cc1101->spd = mgos_sys_config_get_cc1101_spi_spd();
  cc1101->tx.data = NULL;
  pq_set_defaults(&cc1101->tx.pq);
  cc1101->tx.pq.name = "CC1101 PQ";
  cc1101->tx.pq.prio = 24;
  return cc1101;
}

bool mgos_cc1101_mod_regs(struct mgos_cc1101 *cc1101, cc1101_reg_t from,
                          cc1101_reg_t to, mgos_cc1101_mod_regs_cb cb,
                          void *opaque) {
  cc1101_spi_start(cc1101);
  bool ok = cc1101_spi_mod_regs(cc1101, from, to, cb, opaque);
  cc1101_spi_stop(cc1101);
  return ok;
}

bool mgos_cc1101_read_reg(const struct mgos_cc1101 *cc1101, cc1101_reg_t reg,
                          uint8_t *val) {
  cc1101_spi_start(cc1101);
  bool ok = cc1101_spi_read_reg(cc1101, reg, val);
  cc1101_spi_stop(cc1101);
  return ok;
}

bool mgos_cc1101_read_regs(const struct mgos_cc1101 *cc1101, uint8_t cnt,
                           uint8_t *vals) {
  cc1101_spi_start(cc1101);
  bool ok = cc1101_spi_read_regs(cc1101, cnt, vals);
  cc1101_spi_stop(cc1101);
  return ok;
}

bool mgos_cc1101_reset(const struct mgos_cc1101 *cc1101) {
// clang-format off
#define RCFG(reg, ...) {r: CC1101_##reg, v: {reg: {__VA_ARGS__}}}
  static const struct cc1101_reg_val {
    cc1101_reg_t r;
    struct CC1101_ANY v;
  } cc1101_init_conf[] = {
    RCFG(IOCFG2, GDO2_CFG : 46),  // GDO2_CFG : 41
    RCFG(IOCFG0, GDO0_CFG : 46),  // GDO0_CFG : 63
    RCFG(MDMCFG1, CHANSPC_E : 2),  // NUM_PREAMBLE : 2
    RCFG(MCSM0, FS_AUTOCAL : 1, PO_TIMEOUT : 2),  // FS_AUTOCAL : 0, PO_TIMEOUT : 1
    {r : UINT8_MAX}};
#undef RCFG
  // clang-format on

  /* Reset. */
  bool ok = false;
  TRY_RET(ok, mgos_gpio_setup_output, cc1101->gpio.cs, false);
  int64_t enough = mgos_uptime_micros() + 40;
  cc1101_spi_stop(cc1101);
  while (mgos_uptime_micros() < enough) (void) 0;
  cc1101_spi_start(cc1101);
  TRY_GT(cc1101_spi_write_cmd, cc1101, CC1101_SRES);
  while (mgos_gpio_read(cc1101->gpio.so)) (void) 0;
  cc1101_spi_restart(cc1101);

  /* Sane configuration. */
  const struct cc1101_reg_val *rv = cc1101_init_conf;
  for (; rv->r != UINT8_MAX; rv++) {
    TRY_GT(cc1101_spi_write_reg, cc1101, rv->r, rv->v.val);
    uint8_t val = spi_get_reg_gt(cc1101, rv->r);
    if (val != rv->v.val)
      FNERR_GT("reg %02x: wrote %02x, read %02x", rv->r, rv->v.val, val);
  }

  /* Service FIFO via interrupts, if a GPIO is connected. */
  if (cc1101->gpio.gdo >= 0) {
    TRY_GT(mgos_gpio_setup_input, cc1101->gpio.gdo, MGOS_GPIO_PULL_NONE);
    struct CC1101_IOCFG0 r = {GDO0_CFG : 46};
    TRY_GT(cc1101_spi_write_reg, cc1101, cc1101->gdo_reg, r.val);
  }
  ok = true;

err:
  cc1101_spi_stop(cc1101);
  return ok;
}

bool mgos_cc1101_write_reg(const struct mgos_cc1101 *cc1101, cc1101_reg_t reg,
                           uint8_t val) {
  cc1101_spi_start(cc1101);
  bool ok = cc1101_spi_write_reg(cc1101, reg, val);
  cc1101_spi_stop(cc1101);
  return ok;
}

bool mgos_cc1101_init() {
  mgos_cc1101 = mgos_cc1101_create(mgos_sys_config_get_cc1101_cs(),
                                   mgos_sys_config_get_cc1101_gpio_gdo0(),
                                   mgos_sys_config_get_cc1101_gpio_gdo2());
  if (!mgos_cc1101) return true;
  mgos_cc1101_lock = mgos_rlock_create();
  return true;
}
