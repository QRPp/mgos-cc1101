#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <mgos_system.h>

#include <mgos-cc1101-gen-structs.h>

/* {{{1 Sundry. */
#define CC1101_FIFO_SIZE 64

/* {{{1 Control bits. */
typedef enum CC1101_CMD_MOD {
  CC1101_WRITE = 0x00,
  CC1101_WRITE_BURST = 0x40,
  CC1101_READ = 0x80,
  CC1101_READ_BURST = 0xc0,
  CC1101_READ_STATUS = CC1101_READ_BURST
} cc1101_cmd_mod_t;

/* {{{1 Command strobes. */
typedef enum CC1101_CMD {
  CC1101_SRES = 0x30,
  CC1101_SFSTXON,
  CC1101_SXOFF,
  CC1101_SCAL,
  CC1101_SRX,
  CC1101_STX,
  CC1101_SIDLE,
  CC1101_SWOR = 0x38,
  CC1101_SPWD,
  CC1101_SFRX,
  CC1101_SFTX,
  CC1101_SWORRST,
  CC1101_SNOP,
  CC1101_PATABLE,
  CC1101_FIFO
} cc1101_cmd_t;

/* {{{1 Register fields. */
/* {{{2 MDMCFG12.MOD_FORMAT */
typedef enum CC1101_MOD_FORMAT {
  CC1101_MOD_FORMAT_2FSK = 0,
  CC1101_MOD_FORMAT_GFSK,
  CC1101_MOD_FORMAT_ASK_OOK = 3,
  CC1101_MOD_FORMAT_4FSK,
  CC1101_MOD_FORMAT_MSK = 7
} cc1101_MOD_FORMAT_t;

/* {{{2 MARCSTATE.MARC_STATE */
typedef enum CC1101_MARC_STATE {
  CC1101_MARC_STATE_SLEEP = 0,
  CC1101_MARC_STATE_IDLE,
  CC1101_MARC_STATE_XOFF,
  CC1101_MARC_STATE_VCOON_MC,
  CC1101_MARC_STATE_REGON_MC,
  CC1101_MARC_STATE_MANCAL,
  CC1101_MARC_STATE_VCOON,
  CC1101_MARC_STATE_REGON,
  CC1101_MARC_STATE_STARTCAL,
  CC1101_MARC_STATE_BWBOOST,
  CC1101_MARC_STATE_FS_LOCK,
  CC1101_MARC_STATE_IFADCON,
  CC1101_MARC_STATE_ENDCAL,
  CC1101_MARC_STATE_RX,
  CC1101_MARC_STATE_RX_END,
  CC1101_MARC_STATE_RX_RST,
  CC1101_MARC_STATE_TXRX_SWITCH,
  CC1101_MARC_STATE_RXFIFO_OVERFLOW,
  CC1101_MARC_STATE_FSTXON,
  CC1101_MARC_STATE_TX,
  CC1101_MARC_STATE_TX_END,
  CC1101_MARC_STATE_RXTX_SWITCH,
  CC1101_MARC_STATE_TXFIFO_UNDERFLOW
} cc1101_MARC_STATE_t;

/* {{{2 PKTCTRL0.LENGTH_CONFIG */
typedef enum CC1101_LENGTH_CONFIG {
  CC1101_LENGTH_CONFIG_FIXED = 0,
  CC1101_LENGTH_CONFIG_VARIABLE,
  CC1101_LENGTH_CONFIG_INFINITE,
  CC1101_LENGTH_CONFIG_RESERVED
} cc1101_LENGTH_CONFIG_t;
/* {{{2 PKTCTRL0.PKT_FORMAT */
typedef enum CC1101_PKT_FORMAT {
  CC1101_PKT_FORMAT_FIFO = 0,
  CC1101_PKT_FORMAT_SYNC,
  CC1101_PKT_FORMAT_RANDOM,
  CC1101_PKT_FORMAT_ASYNC
} cc1101_PKT_FORMAT_t;
/* }}}1 */

struct mgos_cc1101;

struct mgos_cc1101_distrib {
  uint32_t max, min, cnt, tot;
};

struct mgos_cc1101_regs {
  struct CC1101_ANY *any;
  cc1101_reg_t from, to;
};

#define CC1101_REGS_ANY(regs, reg)                                         \
  ({                                                                       \
    const cc1101_reg_t r = (reg);                                          \
    if (regs->from > r || regs->to < r)                                    \
      FNERR_RETF("have regs %u–%u, need reg %u", regs->from, regs->to, r); \
    regs->any + r - regs->from + 1;                                        \
  })
#define CC1101_REGS_REG(regs, reg) CC1101_REGS_ANY(regs, CC1101_##reg)->reg

struct mgos_cc1101_tx_req {
  uint8_t *data;
  size_t len, copies;
  bool free_data;
  mgos_cb_t cb;
  void *opaque;
};

enum mgos_cc1101_tx_err {
  CC1101_TX_OK = 0,
  CC1101_TX_FIFO_UNDERFLOW,
  CC1101_TX_HW_NOT_READY,
  CC1101_TX_INCOMPLETE,
  CC1101_TX_QUEUE_FULL,
  CC1101_TX_RT_BUSY,
  CC1101_TX_SYS_ERR,
  CC1101_TX_UNHANDLED
};

struct mgos_cc1101_tx_stats {
  struct mgos_cc1101_distrib delay_us, feed_us, fifo_byt;
};

struct mgos_cc1101_tx_op {
  struct mgos_cc1101 *cc1101;
  enum mgos_cc1101_tx_err err;
  struct mgos_cc1101_tx_req req;
  struct mgos_cc1101_tx_stats st;
};

#ifdef __cplusplus
extern "C" {
#endif

struct mgos_cc1101 *mgos_cc1101_create(int cs, int gdo0_gpio, int gdo2_gpio);
struct mgos_cc1101 *mgos_cc1101_get_global();
struct mgos_cc1101 *mgos_cc1101_get_global_locked();
bool mgos_cc1101_get_data_rate(const struct mgos_cc1101 *cc1101,
                               const struct mgos_cc1101_regs *regs,
                               float *kbaud);
typedef bool (*mgos_cc1101_mod_regs_cb)(struct mgos_cc1101 *cc1101,
                                        struct mgos_cc1101_regs *regs,
                                        void *opaque);
bool mgos_cc1101_mod_regs(struct mgos_cc1101 *cc1101, cc1101_reg_t from,
                          cc1101_reg_t to, mgos_cc1101_mod_regs_cb cb,
                          void *opaque);
void mgos_cc1101_put_global_locked();
bool mgos_cc1101_read_reg(const struct mgos_cc1101 *cc1101, cc1101_reg_t reg,
                          uint8_t *val);
bool mgos_cc1101_read_regs(const struct mgos_cc1101 *cc1101, uint8_t cnt,
                           uint8_t *vals);
bool mgos_cc1101_reset(const struct mgos_cc1101 *cc1101);
bool mgos_cc1101_set_data_rate(const struct mgos_cc1101 *cc1101,
                               struct mgos_cc1101_regs *regs, float kbaud);
bool mgos_cc1101_set_deviation(const struct mgos_cc1101 *cc1101,
                               struct mgos_cc1101_regs *regs, float dev_khz);
bool mgos_cc1101_set_frequency(const struct mgos_cc1101 *cc1101,
                               struct mgos_cc1101_regs *regs, float mhz);
bool mgos_cc1101_set_modulation(const struct mgos_cc1101 *cc1101,
                                struct mgos_cc1101_regs *regs, uint8_t format,
                                uint8_t sync_mode, bool manchester, bool fec);
bool mgos_cc1101_write_reg(const struct mgos_cc1101 *cc1101, cc1101_reg_t reg,
                           uint8_t val);
bool mgos_cc1101_tx(struct mgos_cc1101 *cc1101, struct mgos_cc1101_tx_req *req);

#ifdef __cplusplus
}
#endif
