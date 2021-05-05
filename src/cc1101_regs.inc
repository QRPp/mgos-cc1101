/* vim: set filetype=c: */

bool mgos_cc1101_get_data_rate(const struct mgos_cc1101 *cc1101,
                               const struct mgos_cc1101_regs *regs,
                               float *kbaud) {
  uint8_t e = CC1101_REGS_REG(regs, MDMCFG4).DRATE_E,
          m = CC1101_REGS_REG(regs, MDMCFG3).DRATE_M;
  *kbaud = ldexpf((256 + m) * cc1101->osc_khz, e - 28);
  return true;
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
