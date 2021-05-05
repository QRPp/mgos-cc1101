/* vim: set filetype=c: */

static struct mgos_cc1101 *mgos_cc1101 = NULL;
static struct mgos_rlock_type *mgos_cc1101_lock = NULL;

struct mgos_cc1101 *mgos_cc1101_get_global() {
  return mgos_cc1101;
}

struct mgos_cc1101 *mgos_cc1101_get_global_locked() {
  if (mgos_cc1101) mgos_rlock(mgos_cc1101_lock);
  return mgos_cc1101;
}

void mgos_cc1101_put_global_locked() {
  if (mgos_cc1101) mgos_runlock(mgos_cc1101_lock);
}
