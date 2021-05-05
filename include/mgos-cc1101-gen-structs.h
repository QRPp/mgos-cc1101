#pragma once

#include <endian.h>
#include <stdint.h>

#if BYTE_ORDER != BIG_ENDIAN && BYTE_ORDER != LITTLE_ENDIAN
#error Only big and little endian architectures are supported.
#endif

/* {{{1 Registers. */
typedef enum cc1101_reg {
  CC1101_IOCFG2 = 0x00,
  CC1101_IOCFG1,
  CC1101_IOCFG0,
  CC1101_FIFOTHR,
  CC1101_SYNC1,
  CC1101_SYNC0,
  CC1101_PKTLEN,
  CC1101_PKTCTRL1,
  CC1101_PKTCTRL0,
  CC1101_ADDR,
  CC1101_CHANNR,
  CC1101_FSCTRL1,
  CC1101_FSCTRL0,
  CC1101_FREQ2,
  CC1101_FREQ1,
  CC1101_FREQ0,
  CC1101_MDMCFG4,
  CC1101_MDMCFG3,
  CC1101_MDMCFG2,
  CC1101_MDMCFG1,
  CC1101_MDMCFG0,
  CC1101_DEVIATN,
  CC1101_MCSM2,
  CC1101_MCSM1,
  CC1101_MCSM0,
  CC1101_FOCCFG,
  CC1101_BSCFG,
  CC1101_AGCCTRL2,
  CC1101_AGCCTRL1,
  CC1101_AGCCTRL0,
  CC1101_WOREVT1,
  CC1101_WOREVT0,
  CC1101_WORCTRL,
  CC1101_FREND1,
  CC1101_FREND0,
  CC1101_FSCAL3,
  CC1101_FSCAL2,
  CC1101_FSCAL1,
  CC1101_FSCAL0,
  CC1101_RCCTRL1,
  CC1101_RCCTRL0,
  CC1101_FSTEST,
  CC1101_PTEST,
  CC1101_AGCTEST,
  CC1101_TEST2,
  CC1101_TEST1,
  CC1101_TEST0,
  CC1101_PARTNUM = 0x30,
  CC1101_VERSION,
  CC1101_FREQEST,
  CC1101_LQI,
  CC1101_RSSI,
  CC1101_MARCSTATE,
  CC1101_WORTIME1,
  CC1101_WORTIME0,
  CC1101_PKTSTATUS,
  CC1101_VCO_VC_DAC,
  CC1101_TXBYTES,
  CC1101_RXBYTES,
  CC1101_RCCTRL1_STATUS,
  CC1101_RCCTRL0_STATUS,
} cc1101_reg_t;

/* {{{1 Register structures. */
/* {{{2 CHIP_STATUS */
struct CC1101_CHIP_STATUS {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t CHIP_RDYn : 1;
      uint8_t STATE : 3;
      uint8_t FIFO_BYTES_AVAILABLE : 4;
#else
      uint8_t FIFO_BYTES_AVAILABLE : 4;
      uint8_t STATE : 3;
      uint8_t CHIP_RDYn : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 ADDR */
struct CC1101_ADDR {
  union {
    uint8_t DEVICE_ADDR;
    uint8_t val;
  };
};

/* {{{2 AGCCTRL0 */
struct CC1101_AGCCTRL0 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t HYST_LEVEL : 2;
      uint8_t WAIT_TIME : 2;
      uint8_t AGC_FREEZE : 2;
      uint8_t FILTER_LENGTH : 2;
#else
      uint8_t FILTER_LENGTH : 2;
      uint8_t AGC_FREEZE : 2;
      uint8_t WAIT_TIME : 2;
      uint8_t HYST_LEVEL : 2;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 AGCCTRL1 */
struct CC1101_AGCCTRL1 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused : 1;
      uint8_t AGC_LNA_PRIORITY : 1;
      uint8_t CARRIER_SENSE_REL_THR : 2;
      uint8_t CARRIER_SENSE_ABS_THR : 4;
#else
      uint8_t CARRIER_SENSE_ABS_THR : 4;
      uint8_t CARRIER_SENSE_REL_THR : 2;
      uint8_t AGC_LNA_PRIORITY : 1;
      uint8_t unused : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 AGCCTRL2 */
struct CC1101_AGCCTRL2 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t MAX_DVGA_GAIN : 2;
      uint8_t MAX_LNA_GAIN : 3;
      uint8_t MAGN_TARGET : 3;
#else
      uint8_t MAGN_TARGET : 3;
      uint8_t MAX_LNA_GAIN : 3;
      uint8_t MAX_DVGA_GAIN : 2;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 AGCTEST */
struct CC1101_AGCTEST {
  union {
    uint8_t AGCTEST;
    uint8_t val;
  };
};

/* {{{2 BSCFG */
struct CC1101_BSCFG {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t BS_PRE_KI : 2;
      uint8_t BS_PRE_KP : 2;
      uint8_t BS_POST_KI : 1;
      uint8_t BS_POST_KP : 1;
      uint8_t BS_LIMIT : 2;
#else
      uint8_t BS_LIMIT : 2;
      uint8_t BS_POST_KP : 1;
      uint8_t BS_POST_KI : 1;
      uint8_t BS_PRE_KP : 2;
      uint8_t BS_PRE_KI : 2;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 CHANNR */
struct CC1101_CHANNR {
  union {
    uint8_t CHAN;
    uint8_t val;
  };
};

/* {{{2 DEVIATN */
struct CC1101_DEVIATN {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused1 : 1;
      uint8_t DEVIATION_E : 3;
      uint8_t unused2 : 1;
      uint8_t DEVIATION_M : 3;
#else
      uint8_t DEVIATION_M : 3;
      uint8_t unused2 : 1;
      uint8_t DEVIATION_E : 3;
      uint8_t unused1 : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 FIFOTHR */
struct CC1101_FIFOTHR {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused : 1;
      uint8_t ADC_RETENTION : 1;
      uint8_t CLOSE_IN_RX : 2;
      uint8_t FIFO_THR : 4;
#else
      uint8_t FIFO_THR : 4;
      uint8_t CLOSE_IN_RX : 2;
      uint8_t ADC_RETENTION : 1;
      uint8_t unused : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 FOCCFG */
struct CC1101_FOCCFG {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused : 2;
      uint8_t FOC_BS_CS_GATE : 1;
      uint8_t FOC_PRE_K : 2;
      uint8_t FOC_POST_K : 1;
      uint8_t FOC_LIMIT : 2;
#else
      uint8_t FOC_LIMIT : 2;
      uint8_t FOC_POST_K : 1;
      uint8_t FOC_PRE_K : 2;
      uint8_t FOC_BS_CS_GATE : 1;
      uint8_t unused : 2;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 FREND0 */
struct CC1101_FREND0 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused1 : 2;
      uint8_t LODIV_BUF_CURRENT_TX : 2;
      uint8_t unused2 : 1;
      uint8_t PA_POWER : 3;
#else
      uint8_t PA_POWER : 3;
      uint8_t unused2 : 1;
      uint8_t LODIV_BUF_CURRENT_TX : 2;
      uint8_t unused1 : 2;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 FREND1 */
struct CC1101_FREND1 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t LNA_CURRENT : 2;
      uint8_t LNA2MIX_CURRENT : 2;
      uint8_t LODIV_BUF_CURRENT_RX : 2;
      uint8_t MIX_CURRENT : 2;
#else
      uint8_t MIX_CURRENT : 2;
      uint8_t LODIV_BUF_CURRENT_RX : 2;
      uint8_t LNA2MIX_CURRENT : 2;
      uint8_t LNA_CURRENT : 2;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 FREQ0 */
struct CC1101_FREQ0 {
  union {
    uint8_t FREQ_p0;
    uint8_t val;
  };
};

/* {{{2 FREQ1 */
struct CC1101_FREQ1 {
  union {
    uint8_t FREQ_p1;
    uint8_t val;
  };
};

/* {{{2 FREQ2 */
struct CC1101_FREQ2 {
  union {
    uint8_t FREQ_p2;
    uint8_t val;
  };
};

/* {{{2 FREQEST */
struct CC1101_FREQEST {
  union {
    uint8_t FREQOFF_EST;
    uint8_t val;
  };
};

/* {{{2 FSCAL0 */
struct CC1101_FSCAL0 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused : 1;
      uint8_t FSCAL0 : 7;
#else
      uint8_t FSCAL0 : 7;
      uint8_t unused : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 FSCAL1 */
struct CC1101_FSCAL1 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused : 2;
      uint8_t FSCAL1 : 6;
#else
      uint8_t FSCAL1 : 6;
      uint8_t unused : 2;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 FSCAL2 */
struct CC1101_FSCAL2 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused : 2;
      uint8_t VCO_CORE_H_EN : 1;
      uint8_t FSCAL2 : 5;
#else
      uint8_t FSCAL2 : 5;
      uint8_t VCO_CORE_H_EN : 1;
      uint8_t unused : 2;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 FSCAL3 */
struct CC1101_FSCAL3 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t FSCAL3_p2 : 2;
      uint8_t CHP_CURR_CAL_EN : 2;
      uint8_t FSCAL3_p0 : 4;
#else
      uint8_t FSCAL3_p0 : 4;
      uint8_t CHP_CURR_CAL_EN : 2;
      uint8_t FSCAL3_p2 : 2;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 FSCTRL0 */
struct CC1101_FSCTRL0 {
  union {
    uint8_t FREQOFF;
    uint8_t val;
  };
};

/* {{{2 FSCTRL1 */
struct CC1101_FSCTRL1 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused : 2;
      uint8_t reserved : 1;
      uint8_t FREQ_IF : 5;
#else
      uint8_t FREQ_IF : 5;
      uint8_t reserved : 1;
      uint8_t unused : 2;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 FSTEST */
struct CC1101_FSTEST {
  union {
    uint8_t FSTEST;
    uint8_t val;
  };
};

/* {{{2 IOCFG0 */
struct CC1101_IOCFG0 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t TEMP_SENSOR_ENABLE : 1;
      uint8_t GDO0_INV : 1;
      uint8_t GDO0_CFG : 6;
#else
      uint8_t GDO0_CFG : 6;
      uint8_t GDO0_INV : 1;
      uint8_t TEMP_SENSOR_ENABLE : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 IOCFG1 */
struct CC1101_IOCFG1 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t GDO_DS : 1;
      uint8_t GDO1_INV : 1;
      uint8_t GDO1_CFG : 6;
#else
      uint8_t GDO1_CFG : 6;
      uint8_t GDO1_INV : 1;
      uint8_t GDO_DS : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 IOCFG2 */
struct CC1101_IOCFG2 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused : 1;
      uint8_t GDO2_INV : 1;
      uint8_t GDO2_CFG : 6;
#else
      uint8_t GDO2_CFG : 6;
      uint8_t GDO2_INV : 1;
      uint8_t unused : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 LQI */
struct CC1101_LQI {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t CRC_OK : 1;
      uint8_t LQI_EST : 7;
#else
      uint8_t LQI_EST : 7;
      uint8_t CRC_OK : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 MARCSTATE */
struct CC1101_MARCSTATE {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused : 3;
      uint8_t MARC_STATE : 5;
#else
      uint8_t MARC_STATE : 5;
      uint8_t unused : 3;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 MCSM0 */
struct CC1101_MCSM0 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused : 2;
      uint8_t FS_AUTOCAL : 2;
      uint8_t PO_TIMEOUT : 2;
      uint8_t PIN_CTRL_EN : 1;
      uint8_t XOSC_FORCE_ON : 1;
#else
      uint8_t XOSC_FORCE_ON : 1;
      uint8_t PIN_CTRL_EN : 1;
      uint8_t PO_TIMEOUT : 2;
      uint8_t FS_AUTOCAL : 2;
      uint8_t unused : 2;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 MCSM1 */
struct CC1101_MCSM1 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused : 2;
      uint8_t CCA_MODE : 2;
      uint8_t RXOFF_MODE : 2;
      uint8_t TXOFF_MODE : 2;
#else
      uint8_t TXOFF_MODE : 2;
      uint8_t RXOFF_MODE : 2;
      uint8_t CCA_MODE : 2;
      uint8_t unused : 2;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 MCSM2 */
struct CC1101_MCSM2 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused : 3;
      uint8_t RX_TIME_RSSI : 1;
      uint8_t RX_TIME_QUAL : 1;
      uint8_t RX_TIME : 3;
#else
      uint8_t RX_TIME : 3;
      uint8_t RX_TIME_QUAL : 1;
      uint8_t RX_TIME_RSSI : 1;
      uint8_t unused : 3;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 MDMCFG0 */
struct CC1101_MDMCFG0 {
  union {
    uint8_t CHANSPC_M;
    uint8_t val;
  };
};

/* {{{2 MDMCFG1 */
struct CC1101_MDMCFG1 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t FEC_EN : 1;
      uint8_t NUM_PREAMBLE : 3;
      uint8_t unused : 2;
      uint8_t CHANSPC_E : 2;
#else
      uint8_t CHANSPC_E : 2;
      uint8_t unused : 2;
      uint8_t NUM_PREAMBLE : 3;
      uint8_t FEC_EN : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 MDMCFG2 */
struct CC1101_MDMCFG2 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t DEM_DCFILT_OFF : 1;
      uint8_t MOD_FORMAT : 3;
      uint8_t MANCHESTER_EN : 1;
      uint8_t SYNC_MODE : 3;
#else
      uint8_t SYNC_MODE : 3;
      uint8_t MANCHESTER_EN : 1;
      uint8_t MOD_FORMAT : 3;
      uint8_t DEM_DCFILT_OFF : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 MDMCFG3 */
struct CC1101_MDMCFG3 {
  union {
    uint8_t DRATE_M;
    uint8_t val;
  };
};

/* {{{2 MDMCFG4 */
struct CC1101_MDMCFG4 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t CHANBW_E : 2;
      uint8_t CHANBW_M : 2;
      uint8_t DRATE_E : 4;
#else
      uint8_t DRATE_E : 4;
      uint8_t CHANBW_M : 2;
      uint8_t CHANBW_E : 2;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 PARTNUM */
struct CC1101_PARTNUM {
  union {
    uint8_t PARTNUM;
    uint8_t val;
  };
};

/* {{{2 PKTCTRL0 */
struct CC1101_PKTCTRL0 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused1 : 1;
      uint8_t WHITE_DATA : 1;
      uint8_t PKT_FORMAT : 2;
      uint8_t unused2 : 1;
      uint8_t CRC_EN : 1;
      uint8_t LENGTH_CONFIG : 2;
#else
      uint8_t LENGTH_CONFIG : 2;
      uint8_t CRC_EN : 1;
      uint8_t unused2 : 1;
      uint8_t PKT_FORMAT : 2;
      uint8_t WHITE_DATA : 1;
      uint8_t unused1 : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 PKTCTRL1 */
struct CC1101_PKTCTRL1 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t PQT : 3;
      uint8_t unused : 1;
      uint8_t CRC_AUTOFLUSH : 1;
      uint8_t APPEND_STATUS : 1;
      uint8_t ADR_CHK : 2;
#else
      uint8_t ADR_CHK : 2;
      uint8_t APPEND_STATUS : 1;
      uint8_t CRC_AUTOFLUSH : 1;
      uint8_t unused : 1;
      uint8_t PQT : 3;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 PKTLEN */
struct CC1101_PKTLEN {
  union {
    uint8_t PACKET_LENGTH;
    uint8_t val;
  };
};

/* {{{2 PKTSTATUS */
struct CC1101_PKTSTATUS {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t CRC_OK : 1;
      uint8_t CS : 1;
      uint8_t PQT_REACHED : 1;
      uint8_t CCA : 1;
      uint8_t SFD : 1;
      uint8_t GDO2 : 1;
      uint8_t unused : 1;
      uint8_t GDO0 : 1;
#else
      uint8_t GDO0 : 1;
      uint8_t unused : 1;
      uint8_t GDO2 : 1;
      uint8_t SFD : 1;
      uint8_t CCA : 1;
      uint8_t PQT_REACHED : 1;
      uint8_t CS : 1;
      uint8_t CRC_OK : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 PTEST */
struct CC1101_PTEST {
  union {
    uint8_t PTEST;
    uint8_t val;
  };
};

/* {{{2 RCCTRL0 */
struct CC1101_RCCTRL0 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused : 1;
      uint8_t RCCTRL1 : 7;
#else
      uint8_t RCCTRL1 : 7;
      uint8_t unused : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 RCCTRL0_STATUS */
struct CC1101_RCCTRL0_STATUS {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused : 1;
      uint8_t RCCTRL0_STATUS : 7;
#else
      uint8_t RCCTRL0_STATUS : 7;
      uint8_t unused : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 RCCTRL1 */
struct CC1101_RCCTRL1 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused : 1;
      uint8_t RCCTRL1 : 7;
#else
      uint8_t RCCTRL1 : 7;
      uint8_t unused : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 RCCTRL1_STATUS */
struct CC1101_RCCTRL1_STATUS {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t unused : 1;
      uint8_t RCCTRL1_STATUS : 7;
#else
      uint8_t RCCTRL1_STATUS : 7;
      uint8_t unused : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 RSSI */
struct CC1101_RSSI {
  union {
    uint8_t RSSI;
    uint8_t val;
  };
};

/* {{{2 RXBYTES */
struct CC1101_RXBYTES {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t RXFIFO_OVERFLOW : 1;
      uint8_t NUM_RXBYTES : 7;
#else
      uint8_t NUM_RXBYTES : 7;
      uint8_t RXFIFO_OVERFLOW : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 SYNC0 */
struct CC1101_SYNC0 {
  union {
    uint8_t SYNC_p0;
    uint8_t val;
  };
};

/* {{{2 SYNC1 */
struct CC1101_SYNC1 {
  union {
    uint8_t SYNC_p1;
    uint8_t val;
  };
};

/* {{{2 TEST0 */
struct CC1101_TEST0 {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t TEST0_p2 : 6;
      uint8_t VCO_SEL_CAL_EN : 1;
      uint8_t TEST0_p0 : 1;
#else
      uint8_t TEST0_p0 : 1;
      uint8_t VCO_SEL_CAL_EN : 1;
      uint8_t TEST0_p2 : 6;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 TEST1 */
struct CC1101_TEST1 {
  union {
    uint8_t TEST1;
    uint8_t val;
  };
};

/* {{{2 TEST2 */
struct CC1101_TEST2 {
  union {
    uint8_t TEST2;
    uint8_t val;
  };
};

/* {{{2 TXBYTES */
struct CC1101_TXBYTES {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t TXFIFO_UNDERFLOW : 1;
      uint8_t NUM_TXBYTES : 7;
#else
      uint8_t NUM_TXBYTES : 7;
      uint8_t TXFIFO_UNDERFLOW : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 VCO_VC_DAC */
struct CC1101_VCO_VC_DAC {
  union {
    uint8_t VCO_VC_DAC;
    uint8_t val;
  };
};

/* {{{2 VERSION */
struct CC1101_VERSION {
  union {
    uint8_t VERSION;
    uint8_t val;
  };
};

/* {{{2 WORCTRL */
struct CC1101_WORCTRL {
  union {
    struct {
#if BYTE_ORDER == BIG_ENDIAN
      uint8_t RC_PD : 1;
      uint8_t EVENT1 : 3;
      uint8_t RC_CAL : 1;
      uint8_t unused : 1;
      uint8_t WOR_RES : 2;
#else
      uint8_t WOR_RES : 2;
      uint8_t unused : 1;
      uint8_t RC_CAL : 1;
      uint8_t EVENT1 : 3;
      uint8_t RC_PD : 1;
#endif
    };
    uint8_t val;
  };
};

/* {{{2 WOREVT0 */
struct CC1101_WOREVT0 {
  union {
    uint8_t EVENT0_p0;
    uint8_t val;
  };
};

/* {{{2 WOREVT1 */
struct CC1101_WOREVT1 {
  union {
    uint8_t EVENT0_p1;
    uint8_t val;
  };
};

/* {{{2 WORTIME0 */
struct CC1101_WORTIME0 {
  union {
    uint8_t TIME_p0;
    uint8_t val;
  };
};

/* {{{2 WORTIME1 */
struct CC1101_WORTIME1 {
  union {
    uint8_t TIME_p1;
    uint8_t val;
  };
};

/* {{{2 ANY */
struct CC1101_ANY {
  union {
    struct CC1101_CHIP_STATUS CHIP_STATUS;
    struct CC1101_ADDR ADDR;
    struct CC1101_AGCCTRL0 AGCCTRL0;
    struct CC1101_AGCCTRL1 AGCCTRL1;
    struct CC1101_AGCCTRL2 AGCCTRL2;
    struct CC1101_AGCTEST AGCTEST;
    struct CC1101_BSCFG BSCFG;
    struct CC1101_CHANNR CHANNR;
    struct CC1101_DEVIATN DEVIATN;
    struct CC1101_FIFOTHR FIFOTHR;
    struct CC1101_FOCCFG FOCCFG;
    struct CC1101_FREND0 FREND0;
    struct CC1101_FREND1 FREND1;
    struct CC1101_FREQ0 FREQ0;
    struct CC1101_FREQ1 FREQ1;
    struct CC1101_FREQ2 FREQ2;
    struct CC1101_FREQEST FREQEST;
    struct CC1101_FSCAL0 FSCAL0;
    struct CC1101_FSCAL1 FSCAL1;
    struct CC1101_FSCAL2 FSCAL2;
    struct CC1101_FSCAL3 FSCAL3;
    struct CC1101_FSCTRL0 FSCTRL0;
    struct CC1101_FSCTRL1 FSCTRL1;
    struct CC1101_FSTEST FSTEST;
    struct CC1101_IOCFG0 IOCFG0;
    struct CC1101_IOCFG1 IOCFG1;
    struct CC1101_IOCFG2 IOCFG2;
    struct CC1101_LQI LQI;
    struct CC1101_MARCSTATE MARCSTATE;
    struct CC1101_MCSM0 MCSM0;
    struct CC1101_MCSM1 MCSM1;
    struct CC1101_MCSM2 MCSM2;
    struct CC1101_MDMCFG0 MDMCFG0;
    struct CC1101_MDMCFG1 MDMCFG1;
    struct CC1101_MDMCFG2 MDMCFG2;
    struct CC1101_MDMCFG3 MDMCFG3;
    struct CC1101_MDMCFG4 MDMCFG4;
    struct CC1101_PARTNUM PARTNUM;
    struct CC1101_PKTCTRL0 PKTCTRL0;
    struct CC1101_PKTCTRL1 PKTCTRL1;
    struct CC1101_PKTLEN PKTLEN;
    struct CC1101_PKTSTATUS PKTSTATUS;
    struct CC1101_PTEST PTEST;
    struct CC1101_RCCTRL0 RCCTRL0;
    struct CC1101_RCCTRL0_STATUS RCCTRL0_STATUS;
    struct CC1101_RCCTRL1 RCCTRL1;
    struct CC1101_RCCTRL1_STATUS RCCTRL1_STATUS;
    struct CC1101_RSSI RSSI;
    struct CC1101_RXBYTES RXBYTES;
    struct CC1101_SYNC0 SYNC0;
    struct CC1101_SYNC1 SYNC1;
    struct CC1101_TEST0 TEST0;
    struct CC1101_TEST1 TEST1;
    struct CC1101_TEST2 TEST2;
    struct CC1101_TXBYTES TXBYTES;
    struct CC1101_VCO_VC_DAC VCO_VC_DAC;
    struct CC1101_VERSION VERSION;
    struct CC1101_WORCTRL WORCTRL;
    struct CC1101_WOREVT0 WOREVT0;
    struct CC1101_WOREVT1 WOREVT1;
    struct CC1101_WORTIME0 WORTIME0;
    struct CC1101_WORTIME1 WORTIME1;
    uint8_t reg, val;
  };
};
/* }}}1 */
