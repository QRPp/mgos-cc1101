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

#define spi_get_reg_gt(cc1101, reg)                 \
  ({                                                \
    uint8_t val;                                    \
    TRY_GT(cc1101_spi_read_reg, cc1101, reg, &val); \
    val;                                            \
  })

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

static bool cc1101_spi_read_regs(const struct mgos_cc1101 *cc1101, uint8_t cnt,
                                 uint8_t *trx);

bool cc1101_spi_mod_regs(struct mgos_cc1101 *cc1101, cc1101_reg_t from,
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

bool cc1101_spi_read_reg(const struct mgos_cc1101 *cc1101, uint8_t reg,
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

bool cc1101_spi_write_reg(const struct mgos_cc1101 *cc1101, uint8_t reg,
                          uint8_t val) {
  uint8_t tx[] = {reg | CC1101_WRITE, val};
  return cc1101_spi_write(cc1101, sizeof(tx), tx);
}

bool cc1101_spi_write_regs(const struct mgos_cc1101 *cc1101, uint8_t cnt,
                           uint8_t *tx) {
  tx[0] |= cnt > 1 ? CC1101_WRITE_BURST : CC1101_WRITE;
  return cc1101_spi_write(cc1101, cnt + 1, tx);
}

static struct mgos_cc1101 *mgos_cc1101 = NULL;
static struct mgos_rlock_type *mgos_cc1101_lock = NULL;

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

bool mgos_cc1101_get_data_rate(const struct mgos_cc1101 *cc1101,
                               const struct mgos_cc1101_regs *regs,
                               float *kbaud) {
  uint8_t e = CC1101_REGS_REG(regs, MDMCFG4).DRATE_E,
          m = CC1101_REGS_REG(regs, MDMCFG3).DRATE_M;
  *kbaud = ldexpf((256 + m) * cc1101->osc_khz, e - 28);
  return true;
}

struct mgos_cc1101 *mgos_cc1101_get_global() {
  return mgos_cc1101;
}

struct mgos_cc1101 *mgos_cc1101_get_global_locked() {
  if (mgos_cc1101) mgos_rlock(mgos_cc1101_lock);
  return mgos_cc1101;
}

bool mgos_cc1101_mod_regs(struct mgos_cc1101 *cc1101, cc1101_reg_t from,
                          cc1101_reg_t to, mgos_cc1101_mod_regs_cb cb,
                          void *opaque) {
  cc1101_spi_start(cc1101);
  bool ok = cc1101_spi_mod_regs(cc1101, from, to, cb, opaque);
  cc1101_spi_stop(cc1101);
  return ok;
}

void mgos_cc1101_put_global_locked() {
  if (mgos_cc1101) mgos_runlock(mgos_cc1101_lock);
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

bool mgos_cc1101_set_data_rate(const struct mgos_cc1101 *cc1101,
                               struct mgos_cc1101_regs *regs, float kbaud) {
  int e;
  uint8_t m = frexp(kbaud / cc1101->osc_khz, &e) * (1 << 9) - (1 << 8);
  e += 19;
  bool ok = !(e & ~0xf);
  FNLOG(ok ? LL_INFO : LL_ERROR, "%f kbaud → %sDRATE %ue%d", kbaud,
        ok ? "" : "BAD ", m, e);
  if (!ok) return ok;
  CC1101_REGS_REG(regs, MDMCFG4).DRATE_E = e;
  CC1101_REGS_REG(regs, MDMCFG3).DRATE_M = m;
  return ok;
}

bool mgos_cc1101_set_deviation(const struct mgos_cc1101 *cc1101,
                               struct mgos_cc1101_regs *regs, float dev_khz) {
  int e;
  uint8_t m = frexp(dev_khz / cc1101->osc_khz, &e) * (1 << 4) - (1 << 3);
  e += 13;
  bool ok = !(e & ~7);
  FNLOG(ok ? LL_INFO : LL_ERROR, "%f kHz → %sDEVIATION %ue%d", dev_khz,
        ok ? "" : "BAD ", m, e);
  if (!ok) return ok;
  CC1101_REGS_REG(regs, DEVIATN).DEVIATION_E = e;
  CC1101_REGS_REG(regs, DEVIATN).DEVIATION_M = m;
  return ok;
}

bool mgos_cc1101_set_frequency(const struct mgos_cc1101 *cc1101,
                               struct mgos_cc1101_regs *regs, float mhz) {
  uint32_t freq = mhz * (1 << 16) * 1000 / cc1101->osc_khz;
  bool ok = freq < 1 << 22;
  FNLOG(ok ? LL_INFO : LL_ERROR, "%f MHz → %sFREQ 0x%06x", mhz,
        ok ? "" : "BAD ", freq);
  if (!ok) return ok;
  CC1101_REGS_REG(regs, FREQ2).FREQ_p2 = freq >> 16;
  CC1101_REGS_REG(regs, FREQ1).FREQ_p1 = freq >> 8;
  CC1101_REGS_REG(regs, FREQ0).FREQ_p0 = freq;
  return ok;
}

bool mgos_cc1101_set_modulation(const struct mgos_cc1101 *cc1101,
                                struct mgos_cc1101_regs *regs, uint8_t format,
                                uint8_t sync_mode, bool manchester, bool fec) {
  if (manchester && fec)
    FNERR_RETF("Manchester incompatible with FEC/interleaving");
  switch (format) {
    case CC1101_MOD_FORMAT_2FSK:
    case CC1101_MOD_FORMAT_GFSK:
    case CC1101_MOD_FORMAT_ASK_OOK:
      break;
    case CC1101_MOD_FORMAT_4FSK:
    case CC1101_MOD_FORMAT_MSK:
      if (manchester) FNERR_RETF("Manchester incompatible w/ MSK/4-FSK");
      break;
    default:
      FNERR_RETF("invalid modulation format %u", format);
  }
  CC1101_REGS_REG(regs, MDMCFG2).MOD_FORMAT = format;
  CC1101_REGS_REG(regs, MDMCFG2).MANCHESTER_EN = manchester;
  CC1101_REGS_REG(regs, MDMCFG2).SYNC_MODE = sync_mode;
  CC1101_REGS_REG(regs, MDMCFG1).FEC_EN = fec;
  return true;
}

bool mgos_cc1101_write_reg(const struct mgos_cc1101 *cc1101, cc1101_reg_t reg,
                           uint8_t val) {
  cc1101_spi_start(cc1101);
  bool ok = cc1101_spi_write_reg(cc1101, reg, val);
  cc1101_spi_stop(cc1101);
  return ok;
}

#include "cc1101_tx.inc"

bool mgos_cc1101_init() {
  mgos_cc1101 = mgos_cc1101_create(mgos_sys_config_get_cc1101_cs(),
                                   mgos_sys_config_get_cc1101_gpio_gdo0(),
                                   mgos_sys_config_get_cc1101_gpio_gdo2());
  if (!mgos_cc1101) return true;
  mgos_cc1101_lock = mgos_rlock_create();
  return true;
}
