# GhostLock — onn. 4K Plus (coffey)

Local root exploit for the **onn. 4K Plus Streaming** box (`coffey`) via
[CVE-2026-43499](https://nvd.nist.gov/vuln/detail/CVE-2026-43499), a use-after-free in the
Linux kernel futex PI (priority-inheritance) subsystem.

Works on a **locked bootloader** device with no UART, on an Android 14 **user** build.

## Target

| | |
|---|---|
| Device | onn. 4K Plus Streaming (`coffey`) |
| SoC | Amlogic S905X5M (`s7d` platform), 4 cores, 2 GB RAM |
| Kernel | `5.15.196-android14-11-g3d26c188ef7e-ab14657224` (aarch64) |
| Build | `URO4.260304.011.B1` — `onn/coffey/coffey:14/URO4.260304.011.B1/…:user/release-keys` |
| SELinux | enforcing, `setenforce` not available to any domain |
| Bootloader | locked, `dm-verity` + read-only `erofs` system partitions |

## The vulnerability

`remove_waiter()` is called from the `-EDEADLK` rollback path in
`rt_mutex_start_proxy_lock()`. It clears `current->pi_blocked_on` (the requeuer's, which is
already NULL) instead of the waiter task's. The waiter task's `pi_blocked_on` is left
pointing at a freed `rt_mutex_waiter` that lives on the kernel stack.

A subsequent `sched_setattr()` runs `rt_mutex_adjust_pi()` →
`rt_mutex_adjust_prio_chain()`, which follows that dangling pointer and walks a PI chain
over attacker-controlled data placed on the reclaimed page. The walk ends in
`rb_erase`, which gives a controlled write.

## Root method

### 1. Leaks and reclaim

| Stage | Technique |
|---|---|
| KASLR | `perf_event_open` + `PERF_SAMPLE_IP`; the minimum kernel IP lies in `.entry.text`, `_text` is 2 MB-aligned, so `min_ip & ~0x1fffff` is the runtime base |
| `task_struct` | `perf_event_open` + `PERF_SAMPLE_REGS_INTR` (modal linear-map value) |
| `init_user_ns` check | perf-samples `&init_user_ns` out of `security_capable()` during a `setpriority(-20)` storm and validates it against `kaslr_base + offset` before any blind write |
| `mm_struct` | KernelSnitch — futex hash-collision timing side channel |
| Heap spray | SLUB discard choreography + `io_uring_setup(256)` order-2 page reclaim |

### 2. Stack overlay (coffey geometry)

`AF_UNIX SOCK_SEQPACKET` `sendmsg()` copies a 128-byte sockaddr to the kernel stack with
`move_addr_to_kernel()`, overlaying the dangling `rt_mutex_waiter`:

```
rt_mutex_waiter at T-0x1d0    (T = kernel stack top)
iovstack         at T-0x1f8
OVL_SHIFT = 0, base = 0x28    (task at +0x58, lock at +0x60 from the overlay)
```

The overlay is laid down inside the `SIGUSR1` handler that interrupts the futex wait, and
the handler spins purely in user space (no syscalls) so `CONFIG_INIT_STACK_ALL_ZERO`
cannot recycle the waiter region before the walk fires.

### 3. Write primitive (`rb_erase` Case 1)

With the fake node `{pc = (TARGET-8)|1, rb_right = VALUE, rb_left = 0}`:

* `VALUE != 0` → writes `VALUE` to `*TARGET`, then writes `pc` into `*VALUE`
  (so `VALUE` must be a writable kernel address);
* `VALUE == 0` → a clean 8-byte zero at `*TARGET` (no side store).

One reclaim drives **three** walks on the same page (`hits=3/3`), one per rung of the nice
ladder:

| Walk | Write |
|---|---|
| 0 | `selinux_state` (see below) |
| 1 | `task->cred = fake_cred` |
| 2 | `task->real_cred = fake_cred` |

### 4. SELinux: permissive **without** breaking binder

`selinux_state` was parsed from the kernel's own BTF (it is **not** `__randomize_layout`):

```
struct selinux_state (size 136)
  enforcing     @0     checkreqprot @1    initialized @2    policycap[8] @3
  status_page   @16    status_lock @24    avc         @72   policy @80
  policy_mutex  @88
```

The obvious write — 8 zero bytes — also clears `initialized`. That breaks
`security_secid_to_secctx()`, which binder calls for every transaction, so the framework
dies with `binder: … failed …/-22` and the box reboots. `setenforce` cannot be used either:
the write path needs `security:security setenforce`, and no domain on this user build holds
it.

Instead the exploit writes a **page-aligned address of its own reclaim page** to
`selinux_state`:

* byte 0 = `0` → `enforcing = 0` (permissive);
* byte 2 = the page's physical bits 16–23 → must be **odd**.

The odd requirement comes from the kernel itself:

```
add   x8, x24, #2
ldarb w8, [x8]            ; smp_load_acquire(&state->initialized)
tbz   w8, #0, not_init    ; bool load → tests BIT 0
```

So `initialized` must have bit 0 set, not merely be non-zero. Since that byte is fixed per
page, `do_one_write()` skips pages with an even byte 2 and lets the attempt loop reclaim
another (`CRED_ATTEMPTS` defaults to 6).

`child != NULL` makes `rebalance == NULL`, so no colour walk runs; the extra
`parent_color` store lands in the page's unused self-test scratch area. Side effect:
`policycap[0..4]` become the pointer's upper bytes (all set), harmless while permissive.

### 5. Root shell

The fake credential (uid/gid 0, full caps, `user_ns = &init_user_ns`, groups inherited from
the adb shell) is executed by cloning a single-threaded child and `execve("/system/bin/sh")`.
`commit_creds()` in the exec path copies the fake cred into a clean slab credential, and the
parent waits so the child owns the pty. The shell runs with **permissive SELinux** and full
capabilities.

## Building

```bash
export ANDROID_NDK_HOME=/path/to/android-ndk-r27d
make -j$(nproc)
```

Produces a statically linked aarch64 binary (`ghostlock`). Tested with NDK r27d.

## Running

```bash
adb push ghostlock /data/local/tmp/
adb shell chmod 755 /data/local/tmp/ghostlock
adb shell -t
cd /data/local/tmp
./ghostlock --cred
```

`--cred` is the default and now runs the binder-safe permissive path. The exploit takes
~20–40 s (leaks + spray + KernelSnitch + three overlay rounds). On success it execs
`/system/bin/sh` **in the same adb session**; the session stays blocked until you `exit`.

> **It might take a few tries — and the device can reboot during the exploit.**
> The reclaim is probabilistic (`hits=2/3`, `[WALKCHK 0] NOT FOUND`) and a bad run can
> corrupt kernel state enough to trip the watchdog. If the box reboots mid-run: don't
> panic, that's expected. Let it come back, wait for adb (`adb wait-for-device`), and
> run `./ghostlock --cred` again. Repeat until you get `hits=3/3`, `enforce=0` and
> `init check: attr/current=…`.

To use `pm` / framework tools, open a **second** adb shell (the root shell has a minimal
environment):

```bash
adb shell
pm list packages | head
getenforce          # Permissive
```

### Options

| Option | Meaning |
|---|---|
| `--cred` | cred swap + SELinux (default) |
| `--permissive` | keep `initialized=1` (default) |
| `--zero-selinux` | legacy 8-byte zero write — permissive but breaks binder |
| `--cred-only` | mode 6: cred swap only, SELinux stays enforcing |
| `--attempts N` | reclaim attempts 1..6 (default 6) |
| `--domain CTX` | run in a domain, e.g. `u:r:installd:s0` (implies `--cred-only`) |
| `--selftest` | write-primitive self-test (no root) |
| `--help` | usage |

Every knob is also an environment variable; the option wins over an inherited value.

| Environment | Meaning |
|---|---|
| `GHOST_SELINUX=0` | mode 6 (cred only) |
| `GHOST_ENFORCE_INIT=0` | legacy zero write |
| `CRED_ATTEMPTS=1..6` | reclaim attempts |
| `GHOST_EXEC=0` | write the battery and exit(99) instead of spawning the shell |
| `GHOST_SID_ANY=1` | force `current_sid()` to `GHOST_SID` |
| `GHOST_SID_SCAN_CTX=<substr>` | locate a domain by SELinux context |
| `GHOST_SID_SCAN_SETENFORCE=1` | probe `security:security setenforce` per domain |
| `GHOST_SID_KEEP=1` | keep the forced SID for the battery and root shell |
| `GHOST_ZTEST=1` | zero-write variant of `--selftest` |

### Test scripts

```bash
sh /data/local/tmp/test_both.sh b        # default permissive run
sh /data/local/tmp/test_both.sh b-zero   # legacy zero write
sh /data/local/tmp/test_both.sh a        # run in u:r:installd:s0
sh /data/local/tmp/test_both.sh a-sys    # run in u:r:system_server:s0
sh /data/local/tmp/test_enforce.sh       # scan domains for setenforce
```

## What success looks like

```
=== ROOT: uid=0 hits=3/3 ===
selinux write: page byte2=0x49 (odd -> initialized bit0=1)
[SELINUX] main: enforce=0 -> SELinux OFF
init check: attr/current=kernel            <- readback OK, initialized is valid
=== ROOT SHELL: clone+exec /system/bin/sh ===
```

The battery is also written to `/data/local/tmp/.ghostlock_out`.

## Debugging

* **`hits=2/3` / `[WALKCHK 0] NOT FOUND`** — the reclaim missed a plan. Re-run; mode 7 is
  probabilistic. `--attempts 6` widens the window.
* **`selinux write: page byte2=0x.. EVEN … retrying`** — the parity guard rejected a page
  (its byte 2 would make `initialized` read false). Normal; it tries another.
* **`init check: attr/current=FAIL … initialized==0`** — the permissive write did not keep
  `initialized`. Binder will fail (`binder … /-22`), and `pm` will not work. Re-run, or use
  `--cred-only`.
* **`dmesg | grep -c 'initial load_policy'`** — non-zero means `initialized == 0` again.
* **`dmesg | grep -E 'refcount_t|rt_mutex_adjust_prio_chain'`** — the exploit's own nice
  ladder leaves PI/rt_mutex refcount warnings. This is the main remaining post-root
  instability (not SELinux).
* **`getenforce` / `cat /sys/fs/selinux/enforce`** — should print `Permissive` / `0`.
* **`pm` fails only in the root shell** — expected: the root shell has only `PATH`, `HOME`,
  `TERM`. Use a fresh `adb shell`.
* **Battery file errors** — stale `.ghostlock_out` / `gl2_test_create.txt` from earlier runs
  can be labelled oddly. `rm -f /data/local/tmp/.ghostlock_out /data/local/tmp/gl2_test_create.txt`.
* **`kaslr_base`** is printed in the battery; `selinux_state` is at `kaslr_base + 0x29E9078`
  on this build.
* **Self-test**: `./ghostlock --selftest` exercises the write primitive without touching
  credentials; `GHOST_ZTEST=1 ./ghostlock --selftest` exercises the zero-write shape.

## Notes and limitations

* **No persistence.** `/system` is read-only `erofs` behind `dm-verity`, and the bootloader is
  locked, so root lives only for the current boot.
* **`setenforce` cannot be used.** `sel_write_enforce()` requires
  `security:security setenforce`; on this user build no domain has it (verified by scanning
  every SID). Permissive is reached only through the `selinux_state` data write.
* **`policycap[0..4]` side effect.** With `--permissive`, those policy capabilities become
  true. Harmless while `enforcing=0`; if you ever see odd network-labelling behaviour, this
  is why.
* **PI refcount warnings.** The nice ladder triggers `refcount_t` warnings in
  `rt_mutex_adjust_prio_chain`; they indicate corruption and can eventually destabilise the
  device. A long soak is the real test.
* Device-side symbol offsets are in `src/devices/coffey/offsets.h` and
  `src/core/runtime_struct_offsets.h`.

## Credits

* **[CyberMeowfia / IonStack](https://github.com/NebuSec/CyberMeowfia)** — original GhostLock
  exploit.
* **[KernelSnitch](https://github.com/isec-tugraz/KernelSnitch)** — heap-address timing side
  channel (Gruss et al., TU Graz).
* [CVE-2026-43499](https://nvd.nist.gov/vuln/detail/CVE-2026-43499) — the futex PI
  use-after-free.
* Linux 5.15 — `kernel/futex/`, `kernel/locking/rtmutex.c`, `lib/rbtree.c`.

## Disclaimer

Published for security research and education on a device owned by the researcher. Do not
use it on devices you do not own or without authorization.

## License

MIT
