/* drivers/misc/lowmemorykiller.c
 *
 * The lowmemorykiller driver lets user-space specify a set of memory thresholds
 * where processes with a range of oom_score_adj values will get killed. Specify
 * the minimum oom_score_adj values in
 * /sys/module/lowmemorykiller/parameters/adj and the number of free pages in
 * /sys/module/lowmemorykiller/parameters/minfree. Both files take a comma
 * separated list of numbers in ascending order.
 *
 * For example, write "0,8" to /sys/module/lowmemorykiller/parameters/adj and
 * "1024,4096" to /sys/module/lowmemorykiller/parameters/minfree to kill
 * processes with a oom_score_adj value of 8 or higher when the free memory
 * drops below 4096 pages and kill processes with a oom_score_adj value of 0 or
 * higher when the free memory drops below 1024 pages.
 *
 * The driver considers memory used for caches to be free, but if a large
 * percentage of the cached memory is locked this can be very inaccurate
 * and processes may not get killed until the normal oom killer is triggered.
 *
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/oom.h>
#include <linux/sched.h>
#include <linux/swap.h>
#include <linux/rcupdate.h>
#include <linux/notifier.h>
#include <linux/delay.h>
#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_DO_NOT_KILL_PROCESS
#include <linux/string.h>
#endif

#include <linux/vmpressure.h>

#define CREATE_TRACE_POINTS
#include <trace/events/almk.h>

#ifdef CONFIG_HIGHMEM
#define _ZONE ZONE_HIGHMEM
#else
#define _ZONE ZONE_NORMAL
#endif

#ifdef CONFIG_SEC_DEBUG_LMK_COUNT_INFO
#define OOM_COUNT_READ
#endif

#ifdef CONFIG_SEC_OOM_KILLER
#define MULTIPLE_OOM_KILLER
#endif

#ifdef OOM_COUNT_READ
static uint32_t oom_count = 0;
#endif

#ifdef MULTIPLE_OOM_KILLER
#define OOM_DEPTH 7
#endif

static uint32_t lowmem_debug_level = 1;
static short lowmem_adj[6] = {
	0,
	1,
	6,
	12,
};
static int lowmem_adj_size = 6;
static int lowmem_minfree[6] = {
	 3 *  512,	/* Foreground App: 	6 MB	*/
	 2 * 1024,	/* Visible App: 	8 MB	*/
	 4 * 1024,	/* Secondary Server: 	16 MB	*/
	16 * 1024,	/* Hidden App: 		64 MB	*/
	28 * 1024,	/* Content Provider: 	112 MB	*/
	32 * 1024,	/* Empty App: 		128 MB	*/
};
static int lowmem_minfree_size = 6;
static uint32_t lowmem_lmkcount = 0;
static int lmk_fast_run = 1;

static unsigned long lowmem_deathpending_timeout;

#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_DO_NOT_KILL_PROCESS
#define MAX_NOT_KILLABLE_PROCESSES	25	/* Max number of not killable processes */
#define MANAGED_PROCESS_TYPES		3	/* Numer of managed process types (lowmem_process_type) */

/*
 * Enumerator for the managed process types
 */
enum lowmem_process_type {
	KILLABLE_PROCESS,
	DO_NOT_KILL_PROCESS,
	DO_NOT_KILL_SYSTEM_PROCESS
};

/*
 * Data struct for the management of not killable processes
 */
struct donotkill {
	uint enabled;
	char *names[MAX_NOT_KILLABLE_PROCESSES];
	int names_count;
};

static struct donotkill donotkill_proc;		/* User processes to preserve from killing */
static struct donotkill donotkill_sysproc;	/* System processes to preserve from killing */

/*
 * Checks if a process name is inside a list of processes to be preserved from killing
 */
static bool is_in_donotkill_list(char *proc_name, struct donotkill *donotkill_proc)
{
	int i = 0;

	/* If the do not kill feature is enabled and the process names to be preserved
	 * is not empty, then check if the passed process name is contained inside it */
	if (donotkill_proc->enabled && donotkill_proc->names_count > 0) {
		for (i = 0; i < donotkill_proc->names_count; i++) {
			if (strstr(donotkill_proc->names[i], proc_name) != NULL)
				return true; /* The process must be preserved from killing */
		}
	}

	return false; /* The process is not contained inside the process names list */
}

/*
 * Checks if a process name is inside a list of user processes to be preserved from killing
 */
static bool is_in_donotkill_proc_list(char *proc_name)
{
	return is_in_donotkill_list(proc_name, &donotkill_proc);
}

/*
 * Checks if a process name is inside a list of system processes to be preserved from killing
 */
static bool is_in_donotkill_sysproc_list(char *proc_name)
{
	return is_in_donotkill_list(proc_name, &donotkill_sysproc);
}
#else
#define MANAGED_PROCESS_TYPES		1	/* Numer of managed process types (lowmem_process_type) */

/*
 * Enumerator for the managed process types
 */
enum lowmem_process_type {
	KILLABLE_PROCESS
};
#endif

#define lowmem_print(level, x...)			\
	do {						\
		if (lowmem_debug_level >= (level))	\
			pr_info(x);			\
	} while (0)
#if defined(CONFIG_SEC_DEBUG_LMK_MEMINFO)
static void dump_tasks_info(void)
{
	struct task_struct *p;
	struct task_struct *task;

	pr_info("[ pid ]   uid	tgid total_vm	   rss cpu oom_adj oom_score_adj name\n");
	for_each_process(p) {
		/* check unkillable tasks */
		if (is_global_init(p))
			continue;
		if (p->flags & PF_KTHREAD)
			continue;

		task = find_lock_task_mm(p);
		if (!task) {
			/*
			* This is a kthread or all of p's threads have already
			* detached their mm's.	There's no need to report
			* them; they can't be oom killed anyway.
			*/
			continue;
		}

		pr_info("[%5d] %5d %5d %8lu %8lu %3u	 %3d	     %5d %s\n",
		task->pid, task_uid(task), task->tgid,
		task->mm->total_vm, get_mm_rss(task->mm),
		task_cpu(task), task->signal->oom_adj,
		task->signal->oom_score_adj, task->comm);
		task_unlock(task);
	}
}
#endif

static atomic_t shift_adj = ATOMIC_INIT(0);
static short adj_max_shift = 353;

/* User knob to enable/disable adaptive lmk feature */
static int enable_adaptive_lmk = 1;
module_param_named(enable_adaptive_lmk, enable_adaptive_lmk, int,
	S_IRUGO | S_IWUSR);

/*
 * This parameter controls the behaviour of LMK when vmpressure is in
 * the range of 90-94. Adaptive lmk triggers based on number of file
 * pages wrt vmpressure_file_min, when vmpressure is in the range of
 * 90-94. Usually this is a pseudo minfree value, higher than the
 * highest configured value in minfree array.
 */
static int vmpressure_file_min;
module_param_named(vmpressure_file_min, vmpressure_file_min, int,
	S_IRUGO | S_IWUSR);

enum {
	VMPRESSURE_NO_ADJUST = 0,
	VMPRESSURE_ADJUST_ENCROACH,
	VMPRESSURE_ADJUST_NORMAL,
};

int adjust_minadj(short *min_score_adj)
{
	int ret = VMPRESSURE_NO_ADJUST;

	if (!enable_adaptive_lmk)
		return 0;

	if (atomic_read(&shift_adj) &&
		(*min_score_adj > adj_max_shift)) {
		if (*min_score_adj == OOM_SCORE_ADJ_MAX + 1)
			ret = VMPRESSURE_ADJUST_ENCROACH;
		else
			ret = VMPRESSURE_ADJUST_NORMAL;
		*min_score_adj = adj_max_shift;
	}
	atomic_set(&shift_adj, 0);

	return ret;
}

static int lmk_vmpressure_notifier(struct notifier_block *nb,
			unsigned long action, void *data)
{
	int other_free, other_file;
	unsigned long pressure = action;
	int array_size = ARRAY_SIZE(lowmem_adj);

	if (!enable_adaptive_lmk)
		return 0;

	if (pressure >= 95) {
		other_file = global_page_state(NR_FILE_PAGES) -
			global_page_state(NR_SHMEM) -
			total_swapcache_pages();
		other_free = global_page_state(NR_FREE_PAGES);

		atomic_set(&shift_adj, 1);
		trace_almk_vmpressure(pressure, other_free, other_file);
	} else if (pressure >= 90) {
		if (lowmem_adj_size < array_size)
			array_size = lowmem_adj_size;
		if (lowmem_minfree_size < array_size)
			array_size = lowmem_minfree_size;

		other_file = global_page_state(NR_FILE_PAGES) -
			global_page_state(NR_SHMEM) -
			total_swapcache_pages();

		other_free = global_page_state(NR_FREE_PAGES);

		if ((other_free < lowmem_minfree[array_size - 1]) &&
			(other_file < vmpressure_file_min)) {
				atomic_set(&shift_adj, 1);
				trace_almk_vmpressure(pressure, other_free,
					other_file);
		}
	}

	return 0;
}

static struct notifier_block lmk_vmpr_nb = {
	.notifier_call = lmk_vmpressure_notifier,
};

static int test_task_flag(struct task_struct *p, int flag)
{
	struct task_struct *t;

	for_each_thread(p, t) {
		task_lock(t);
		if (test_tsk_thread_flag(t, flag)) {
			task_unlock(t);
			return 1;
		}
		task_unlock(t);
	}

	return 0;
}

int can_use_cma_pages(gfp_t gfp_mask)
{
	int can_use = 0;
	int mtype = allocflags_to_migratetype(gfp_mask);
	int i = 0;
	int *mtype_fallbacks = get_migratetype_fallbacks(mtype);

	if (is_migrate_cma(mtype)) {
		can_use = 1;
	} else {
		for (i = 0;; i++) {
			int fallbacktype = mtype_fallbacks[i];

			if (is_migrate_cma(fallbacktype)) {
				can_use = 1;
				break;
			}

			if (fallbacktype == MIGRATE_RESERVE)
				break;
		}
	}
	return can_use;
}

void tune_lmk_zone_param(struct zonelist *zonelist, int classzone_idx,
					int *other_free, int *other_file,
					int use_cma_pages)
{
	struct zone *zone;
	struct zoneref *zoneref;
	int zone_idx;

	for_each_zone_zonelist(zone, zoneref, zonelist, MAX_NR_ZONES) {
		zone_idx = zonelist_zone_idx(zoneref);
		if (zone_idx == ZONE_MOVABLE) {
			if (!use_cma_pages)
				*other_free -=
				    zone_page_state(zone, NR_FREE_CMA_PAGES);
			continue;
		}

		if (zone_idx > classzone_idx) {
			if (other_free != NULL)
				*other_free -= zone_page_state(zone,
							       NR_FREE_PAGES);
			if (other_file != NULL)
				*other_file -= zone_page_state(zone,
							       NR_FILE_PAGES)
					      - zone_page_state(zone, NR_SHMEM);
		} else if (zone_idx < classzone_idx) {
			if (zone_watermark_ok(zone, 0, 0, classzone_idx, 0)) {
				if (!use_cma_pages) {
					*other_free -= min(
					  zone->lowmem_reserve[classzone_idx] +
					  zone_page_state(
					    zone, NR_FREE_CMA_PAGES),
					  zone_page_state(
					    zone, NR_FREE_PAGES));
				} else {
					*other_free -=
					  zone->lowmem_reserve[classzone_idx];
				}
			} else {
				*other_free -=
					   zone_page_state(zone, NR_FREE_PAGES);
			}
		}
	}
}

#ifdef CONFIG_HIGHMEM
void adjust_gfp_mask(gfp_t *gfp_mask)
{
	struct zone *preferred_zone;
	struct zonelist *zonelist;
	enum zone_type high_zoneidx;

	if (current_is_kswapd()) {
		zonelist = node_zonelist(0, *gfp_mask);
		high_zoneidx = gfp_zone(*gfp_mask);
		first_zones_zonelist(zonelist, high_zoneidx, NULL,
				&preferred_zone);

		if (high_zoneidx == ZONE_NORMAL) {
			if (zone_watermark_ok_safe(preferred_zone, 0,
					high_wmark_pages(preferred_zone), 0,
					0))
				*gfp_mask |= __GFP_HIGHMEM;
		} else if (high_zoneidx == ZONE_HIGHMEM) {
			*gfp_mask |= __GFP_HIGHMEM;
		}
	}
}
#else
void adjust_gfp_mask(gfp_t *unused)
{
}
#endif

#ifdef CONFIG_ANDROID_LMK_ADJ_RBTREE
static struct task_struct *pick_next_from_adj_tree(struct task_struct *task);
static struct task_struct *pick_first_task(void);
static struct task_struct *pick_last_task(void);
#endif

void tune_lmk_param(int *other_free, int *other_file, struct shrink_control *sc)
{
	gfp_t gfp_mask;
	struct zone *preferred_zone;
	struct zonelist *zonelist;
	enum zone_type high_zoneidx, classzone_idx;
	unsigned long balance_gap;
	int use_cma_pages;

	gfp_mask = sc->gfp_mask;
	adjust_gfp_mask(&gfp_mask);

	zonelist = node_zonelist(0, gfp_mask);
	high_zoneidx = gfp_zone(gfp_mask);
	first_zones_zonelist(zonelist, high_zoneidx, NULL, &preferred_zone);
	classzone_idx = zone_idx(preferred_zone);
	use_cma_pages = can_use_cma_pages(gfp_mask);

	balance_gap = min(low_wmark_pages(preferred_zone),
			  (preferred_zone->present_pages +
			   KSWAPD_ZONE_BALANCE_GAP_RATIO-1) /
			   KSWAPD_ZONE_BALANCE_GAP_RATIO);

	if (likely(current_is_kswapd() && zone_watermark_ok(preferred_zone, 0,
			  high_wmark_pages(preferred_zone) + SWAP_CLUSTER_MAX +
			  balance_gap, 0, 0))) {
		if (lmk_fast_run)
			tune_lmk_zone_param(zonelist, classzone_idx, other_free,
				       other_file, use_cma_pages);
		else
			tune_lmk_zone_param(zonelist, classzone_idx, other_free,
				       NULL, use_cma_pages);

		if (zone_watermark_ok(preferred_zone, 0, 0, _ZONE, 0)) {
			if (!use_cma_pages) {
				*other_free -= min(
				  preferred_zone->lowmem_reserve[_ZONE]
				  + zone_page_state(
				    preferred_zone, NR_FREE_CMA_PAGES),
				  zone_page_state(
				    preferred_zone, NR_FREE_PAGES));
			} else {
				*other_free -=
				  preferred_zone->lowmem_reserve[_ZONE];
			}
		} else {
			*other_free -= zone_page_state(preferred_zone,
						      NR_FREE_PAGES);
		}

		lowmem_print(4, "lowmem_shrink of kswapd tunning for highmem "
			     "ofree %d, %d\n", *other_free, *other_file);
	} else {
		tune_lmk_zone_param(zonelist, classzone_idx, other_free,
			       other_file, use_cma_pages);

		if (!use_cma_pages) {
			*other_free -=
			  zone_page_state(preferred_zone, NR_FREE_CMA_PAGES);
		}

		lowmem_print(4, "lowmem_shrink tunning for others ofree %d, "
			     "%d\n", *other_free, *other_file);
	}
}

#if defined(CONFIG_ZSWAP)
extern atomic_t zswap_pool_pages;
extern atomic_t zswap_stored_pages;
#endif

static bool avoid_to_kill(uid_t uid)
{
	/* 
	 * uid info
	 * uid == 0 > root
	 * uid == 1001 > radio
	 * uid == 1002 > bluetooth
	 * uid == 1010 > wifi
	 * uid == 1014 > dhcp
	 */
	if (uid == 0 || uid == 1001 || uid == 1002 || uid == 1010 ||
			uid == 1014)
		return 1;
	return 0;
}

static bool protected_apps(char *comm)
{
	if (strcmp(comm, "d.process.acore") == 0 ||
			strcmp(comm, "ndroid.systemui") == 0 ||
			strcmp(comm, "ndroid.contacts") == 0 ||
			strcmp(comm, "system:ui") == 0)
		return 1;
	return 0;
}

static int lowmem_shrink(struct shrinker *s, struct shrink_control *sc)
{
	struct task_struct *tsk;
	struct task_struct *selected[MANAGED_PROCESS_TYPES] = {NULL};
	const struct cred *pcred;
	unsigned int uid = 0;
	int rem = 0;
	int ret = 0;
	int tasksize;
	int i;
	short min_score_adj = OOM_SCORE_ADJ_MAX + 1;
	int minfree = 0;
	enum lowmem_process_type proc_type = KILLABLE_PROCESS;
	int selected_tasksize[MANAGED_PROCESS_TYPES] = {0};
	int selected_oom_score_adj[MANAGED_PROCESS_TYPES];
	int array_size = ARRAY_SIZE(lowmem_adj);
	int other_free = global_page_state(NR_FREE_PAGES);
	int other_file = global_page_state(NR_FILE_PAGES) -
						global_page_state(NR_SHMEM);
	struct reclaim_state *reclaim_state = current->reclaim_state;

#ifdef CONFIG_ZSWAP
	/* to prevent other_file underflow and then be negative */
	if (other_file > total_swapcache_pages())
		other_file -= total_swapcache_pages();
	else
		other_file = 0;
#endif

	tune_lmk_param(&other_free, &other_file, sc);

	if (lowmem_adj_size < array_size)
		array_size = lowmem_adj_size;
	if (lowmem_minfree_size < array_size)
		array_size = lowmem_minfree_size;
	for (i = 0; i < array_size; i++) {
		minfree = lowmem_minfree[i];
		if (other_free < minfree && other_file < minfree) {
			min_score_adj = lowmem_adj[i];
			break;
		}
	}
	if (sc->nr_to_scan > 0) {
		ret = adjust_minadj(&min_score_adj);
		lowmem_print(3, "lowmem_shrink %lu, %x, ofree %d %d, ma %hd\n",
				sc->nr_to_scan, sc->gfp_mask, other_free,
				other_file, min_score_adj);
	}

	rem = global_page_state(NR_ACTIVE_ANON) +
		global_page_state(NR_ACTIVE_FILE) +
		global_page_state(NR_INACTIVE_ANON) +
		global_page_state(NR_INACTIVE_FILE);
	if (sc->nr_to_scan <= 0 || min_score_adj == OOM_SCORE_ADJ_MAX + 1) {
		lowmem_print(5, "lowmem_shrink %lu, %x, return %d\n",
			     sc->nr_to_scan, sc->gfp_mask, rem);

		if ((min_score_adj == OOM_SCORE_ADJ_MAX + 1) &&
			(sc->nr_to_scan > 0))
			trace_almk_shrink(0, ret, other_free, other_file, 0);

		return rem;
	}

	/* Set the initial oom_score_adj for each managed process type */
	for (proc_type = KILLABLE_PROCESS; proc_type < MANAGED_PROCESS_TYPES; proc_type++)
		selected_oom_score_adj[proc_type] = min_score_adj;

	rcu_read_lock();
#ifdef CONFIG_ANDROID_LMK_ADJ_RBTREE
	for (tsk = pick_first_task();
		tsk != pick_last_task() && tsk != NULL;
		tsk = pick_next_from_adj_tree(tsk)) {
#else
	for_each_process(tsk) {
#endif
		struct task_struct *p;
		short oom_score_adj;

		if (tsk->flags & PF_KTHREAD)
			continue;

		if (time_before_eq(jiffies, lowmem_deathpending_timeout)) {
			if (test_task_flag(tsk, TIF_MEMDIE)) {
				rcu_read_unlock();
	        /* give the system time to free up the memory */
        	msleep_interruptible(20);
				return 0;
			}
		}

		p = find_lock_task_mm(tsk);
		if (!p)
			continue;

		oom_score_adj = p->signal->oom_score_adj;
		if (oom_score_adj < min_score_adj) {
			task_unlock(p);
#ifdef CONFIG_ANDROID_LMK_ADJ_RBTREE
			break;
#else
			continue;
#endif
		}
		tasksize = get_mm_rss(p->mm);
#if defined(CONFIG_ZSWAP)
		if (atomic_read(&zswap_stored_pages)) {
			lowmem_print(3, "shown tasksize : %d\n", tasksize);
			tasksize += atomic_read(&zswap_pool_pages) * get_mm_counter(p->mm, MM_SWAPENTS)
				/ atomic_read(&zswap_stored_pages);
			lowmem_print(3, "real tasksize : %d\n", tasksize);
		}
#endif

		task_unlock(p);
		if (tasksize <= 0)
			continue;
#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_DO_NOT_KILL_PROCESS
		/* Check if the process name is contained inside the process to be preserved lists */
		if (is_in_donotkill_proc_list(p->comm)) {
			/* This user process must be preserved from killing */
			proc_type = DO_NOT_KILL_PROCESS;
			lowmem_print(2, "The process '%s' is inside the donotkill_proc_names", p->comm);
		} else if (is_in_donotkill_sysproc_list(p->comm)) {
			/* This system process must be preserved from killing */
			proc_type = DO_NOT_KILL_SYSTEM_PROCESS;
			lowmem_print(2, "The process '%s' is inside the donotkill_sysproc_names", p->comm);
		}
#endif

		if (selected[proc_type]) {
			if (oom_score_adj < selected_oom_score_adj[proc_type])
				continue;
			if (oom_score_adj == selected_oom_score_adj[proc_type] &&
			    tasksize <= selected_tasksize[proc_type])
				continue;
		}
		selected[proc_type] = p;
		selected_tasksize[proc_type] = tasksize;
		selected_oom_score_adj[proc_type] = oom_score_adj;
		lowmem_print(2, "select '%s' (%d), adj %hd, size %d, to kill\n",
			     p->comm, p->pid, oom_score_adj, tasksize);
	}
	/* For each managed process type check if a process to be killed has been found:
	 * - check first if a standard killable process has been found, if so kill it
	 * - if there is no killable process, then check if a user process has been found,
	 *   if so kill it to prevent system slowdowns, hangs, etc.
	 * - if there is no killable and user process, then check if a system process has been found,
	 *   if so kill it to prevent system slowdowns, hangs, etc. */
	for (proc_type = KILLABLE_PROCESS; proc_type < MANAGED_PROCESS_TYPES; proc_type++) {
		if (selected[proc_type]) {
			lowmem_print(1, "Killing '%s' (%d), adj %d,\n" \
					"   to free %ldkB on behalf of '%s' (%d) because\n" \
					"   cache %ldkB is below limit %ldkB for oom_score_adj %d\n" \
					"   Free memory is %ldkB above reserved\n",
					 selected[proc_type]->comm, selected[proc_type]->pid,
					 selected_oom_score_adj[proc_type],
					 selected_tasksize[proc_type] * (long)(PAGE_SIZE / 1024),
					 current->comm, current->pid,
					 other_file * (long)(PAGE_SIZE / 1024),
					 minfree * (long)(PAGE_SIZE / 1024),
					 min_score_adj,
					 other_free * (long)(PAGE_SIZE / 1024));
			lowmem_deathpending_timeout = jiffies + HZ;
			send_sig(SIGKILL, selected[proc_type], 0);
			set_tsk_thread_flag(selected[proc_type], TIF_MEMDIE);
			rem -= selected_tasksize[proc_type];
			break;
		}
	}

	trace_almk_shrink(1, ret, other_free, other_file, 0);
	rcu_read_unlock();

		lowmem_lmkcount++;

	lowmem_print(4, "lowmem_shrink %lu, %x, return %d\n",
		     sc->nr_to_scan, sc->gfp_mask, rem);
	return rem;
}

/*
 * CONFIG_SEC_OOM_KILLER : klaatu@sec
 *
 * The way to select victim by oom-killer provided by
 * linux kernel is totally different from android policy.
 * Hence, it makes more sense that we select the oom victim
 * as android does when LMK is invoked.
 *
*/
#ifdef CONFIG_SEC_OOM_KILLER

static int android_oom_handler(struct notifier_block *nb,
				      unsigned long val, void *data)
{
	struct task_struct *tsk;
#ifdef MULTIPLE_OOM_KILLER
	struct task_struct *selected[OOM_DEPTH] = {NULL,};
#else
	struct task_struct *selected = NULL;
#endif
	int rem = 0;
	int tasksize;
	int i;
	int min_score_adj = OOM_SCORE_ADJ_MAX + 1;
#ifdef MULTIPLE_OOM_KILLER
	int selected_tasksize[OOM_DEPTH] = {0,};
	int selected_oom_score_adj[OOM_DEPTH] = {OOM_ADJUST_MAX,};
	int all_selected_oom = 0;
	int max_selected_oom_idx = 0;
#else
	int selected_tasksize = 0;
	int selected_oom_score_adj;
#endif
#ifdef CONFIG_SEC_DEBUG_LMK_MEMINFO
	static DEFINE_RATELIMIT_STATE(oom_rs, DEFAULT_RATELIMIT_INTERVAL/5, 1);
#endif

	unsigned long *freed = data;

	/* show status */
	pr_warning("%s invoked Android-oom-killer: "
		" oom_score_adj=%d\n",
		current->comm, current->signal->oom_score_adj);
#ifdef CONFIG_SEC_DEBUG_LMK_MEMINFO
	dump_stack();
	show_mem(SHOW_MEM_FILTER_NODES);
	if (__ratelimit(&oom_rs))
		dump_tasks_info();
#endif

	min_score_adj = 0;
#ifdef MULTIPLE_OOM_KILLER
	for (i = 0; i < OOM_DEPTH; i++)
		selected_oom_score_adj[i] = min_score_adj;
#else
	selected_oom_score_adj = min_score_adj;
#endif

	read_lock(&tasklist_lock);
	for_each_process(tsk) {
		struct task_struct *p;
		int oom_score_adj;
#ifdef MULTIPLE_OOM_KILLER
		int is_exist_oom_task = 0;
#endif

		if (tsk->flags & PF_KTHREAD)
			continue;

		p = find_lock_task_mm(tsk);
		if (!p)
			continue;

		oom_score_adj = p->signal->oom_score_adj;
		if (oom_score_adj < min_score_adj) {
			task_unlock(p);
#ifdef CONFIG_ANDROID_LMK_ADJ_RBTREE
			break;
#else
			continue;
#endif
		}
		if (fatal_signal_pending(p) ||
				((p->flags & PF_EXITING) &&
					test_tsk_thread_flag(p, TIF_MEMDIE))) {
			lowmem_print(2, "skip slow dying process %d\n", p->pid);
			task_unlock(p);
			continue;
		}
		tasksize = get_mm_rss(p->mm);
		task_unlock(p);
		if (tasksize <= 0)
			continue;

		lowmem_print(2, "oom: ------ %d (%s), adj %d, size %d\n",
			     p->pid, p->comm, oom_score_adj, tasksize);
#ifdef MULTIPLE_OOM_KILLER
		if (all_selected_oom < OOM_DEPTH) {
			for (i = 0; i < OOM_DEPTH; i++) {
				if (!selected[i]) {
					is_exist_oom_task = 1;
					max_selected_oom_idx = i;
					break;
				}
			}
		} else if (selected_oom_score_adj[max_selected_oom_idx] < oom_score_adj ||
			(selected_oom_score_adj[max_selected_oom_idx] == oom_score_adj &&
			selected_tasksize[max_selected_oom_idx] < tasksize)) {
			is_exist_oom_task = 1;
		}

		if (is_exist_oom_task) {
			selected[max_selected_oom_idx] = p;
			selected_tasksize[max_selected_oom_idx] = tasksize;
			selected_oom_score_adj[max_selected_oom_idx] = oom_score_adj;

			if (all_selected_oom < OOM_DEPTH)
				all_selected_oom++;

			if (all_selected_oom == OOM_DEPTH) {
				for (i = 0; i < OOM_DEPTH; i++) {
					if (selected_oom_score_adj[i] < selected_oom_score_adj[max_selected_oom_idx])
						max_selected_oom_idx = i;
					else if (selected_oom_score_adj[i] == selected_oom_score_adj[max_selected_oom_idx] &&
						selected_tasksize[i] < selected_tasksize[max_selected_oom_idx])
						max_selected_oom_idx = i;
				}
			}

			lowmem_print(2, "oom: max_selected_oom_idx(%d) select %d (%s), adj %d, \
					size %d, to kill\n",
				max_selected_oom_idx, p->pid, p->comm, oom_score_adj, tasksize);
		}
#else
		if (selected) {
			if (oom_score_adj < selected_oom_score_adj)
#ifdef CONFIG_ANDROID_LMK_ADJ_RBTREE
				break;
#else
				continue;
#endif
			if (oom_score_adj == selected_oom_score_adj &&
			    tasksize <= selected_tasksize)
				continue;
		}
		pcred = __task_cred(p);
		uid = pcred->uid;
		if (avoid_to_kill(uid) || protected_apps(p->comm)){
			if (tasksize * (long)(PAGE_SIZE / 1024) >= 100000){
				selected = p;
				selected_tasksize = tasksize;
				selected_oom_score_adj = oom_score_adj;
				lowmem_print(3, "select protected %d (%s), adj %hd, size %d, to kill\n",
				     	p->pid, p->comm, oom_score_adj, tasksize);
			} else
			lowmem_print(3, "skip protected %d (%s), adj %hd, size %d, to kill\n",
			     	p->pid, p->comm, oom_score_adj, tasksize);
		} else {
			selected = p;
			selected_tasksize = tasksize;
			selected_oom_score_adj = oom_score_adj;
			lowmem_print(3, "select %d (%s), adj %hd, size %d, to kill\n",
			     	p->pid, p->comm, oom_score_adj, tasksize);
		}
#endif
	}
#ifdef MULTIPLE_OOM_KILLER
	for (i = 0; i < OOM_DEPTH; i++) {
		if (selected[i]) {
			lowmem_print(1, "oom: send sigkill to %d (%s), adj %d,\
				     size %d\n",
				     selected[i]->pid, selected[i]->comm,
				     selected_oom_score_adj[i],
				     selected_tasksize[i]);
			send_sig(SIGKILL, selected[i], 0);
			rem -= selected_tasksize[i];
			*freed += (unsigned long)selected_tasksize[i];
#ifdef OOM_COUNT_READ
			oom_count++;
#endif

		}
	}
#else
	if (selected) {
		lowmem_print(1, "oom: send sigkill to %d (%s), adj %d, size %d\n",
			     selected->pid, selected->comm,
			     selected_oom_score_adj, selected_tasksize);
		send_sig(SIGKILL, selected, 0);
		set_tsk_thread_flag(selected, TIF_MEMDIE);
		rem -= selected_tasksize;
		rcu_read_unlock();
		*freed += (unsigned long)selected_tasksize;
#ifdef OOM_COUNT_READ
		oom_count++;
#endif
		/* give the system time to free up the memory */
		msleep_interruptible(20);
		if(reclaim_state)
		 reclaim_state->reclaimed_slab += selected_tasksize;
		} else {
		rcu_read_unlock();
	}
#endif
	read_unlock(&tasklist_lock);

	lowmem_print(2, "oom: get memory %lu", *freed);
	return rem;
}

static struct notifier_block android_oom_notifier = {
	.notifier_call = android_oom_handler,
};
#endif /* CONFIG_SEC_OOM_KILLER */

static struct shrinker lowmem_shrinker = {
	.shrink = lowmem_shrink,
	.seeks = 32
};

static int __init lowmem_init(void)
{
	register_shrinker(&lowmem_shrinker);
	vmpressure_notifier_register(&lmk_vmpr_nb);
#ifdef CONFIG_SEC_OOM_KILLER
	register_oom_notifier(&android_oom_notifier);
#endif

	return 0;
}

static void __exit lowmem_exit(void)
{
	unregister_shrinker(&lowmem_shrinker);
}

#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_AUTODETECT_OOM_ADJ_VALUES
static short lowmem_oom_adj_to_oom_score_adj(short oom_adj)
{
	if (oom_adj == OOM_ADJUST_MAX)
		return OOM_SCORE_ADJ_MAX;
	else
		return (oom_adj * OOM_SCORE_ADJ_MAX) / -OOM_DISABLE;
}

static void lowmem_autodetect_oom_adj_values(void)
{
	int i;
	short oom_adj;
	short oom_score_adj;
	int array_size = ARRAY_SIZE(lowmem_adj);

	if (lowmem_adj_size < array_size)
		array_size = lowmem_adj_size;

	if (array_size <= 0)
		return;

	oom_adj = lowmem_adj[array_size - 1];
	if (oom_adj > OOM_ADJUST_MAX)
		return;

	oom_score_adj = lowmem_oom_adj_to_oom_score_adj(oom_adj);
	if (oom_score_adj <= OOM_ADJUST_MAX)
		return;

	lowmem_print(1, "lowmem_shrink: convert oom_adj to oom_score_adj:\n");
	for (i = 0; i < array_size; i++) {
		oom_adj = lowmem_adj[i];
		oom_score_adj = lowmem_oom_adj_to_oom_score_adj(oom_adj);
		lowmem_adj[i] = oom_score_adj;
		lowmem_print(1, "oom_adj %hd => oom_score_adj %hd\n",
			     oom_adj, oom_score_adj);
	}
}

static int lowmem_adj_array_set(const char *val, const struct kernel_param *kp)
{
	int ret;

	ret = param_array_ops.set(val, kp);

	/* HACK: Autodetect oom_adj values in lowmem_adj array */
	lowmem_autodetect_oom_adj_values();

	return ret;
}

static int lowmem_adj_array_get(char *buffer, const struct kernel_param *kp)
{
	return param_array_ops.get(buffer, kp);
}

static void lowmem_adj_array_free(void *arg)
{
	param_array_ops.free(arg);
}

static struct kernel_param_ops lowmem_adj_array_ops = {
	.set = lowmem_adj_array_set,
	.get = lowmem_adj_array_get,
	.free = lowmem_adj_array_free,
};

static const struct kparam_array __param_arr_adj = {
	.max = ARRAY_SIZE(lowmem_adj),
	.num = &lowmem_adj_size,
	.ops = &param_ops_short,
	.elemsize = sizeof(lowmem_adj[0]),
	.elem = lowmem_adj,
};
#endif

#ifdef CONFIG_ANDROID_LMK_ADJ_RBTREE
DEFINE_SPINLOCK(lmk_lock);
struct rb_root tasks_scoreadj = RB_ROOT;
/*
 * Makesure to invoke the function with holding sighand->siglock
 */
void add_2_adj_tree(struct task_struct *task)
{
	struct rb_node **link;
	struct rb_node *parent = NULL;
	struct signal_struct *sig_entry;
	s64 key = task->signal->oom_score_adj;

	/*
	 * Find the right place in the rbtree:
	 */
	spin_lock(&lmk_lock);
	link =  &tasks_scoreadj.rb_node;
	while (*link) {
		parent = *link;
		sig_entry = rb_entry(parent, struct signal_struct, adj_node);

		if (key < sig_entry->oom_score_adj)
			link = &parent->rb_right;
		else
			link = &parent->rb_left;
	}

	rb_link_node(&task->signal->adj_node, parent, link);
	rb_insert_color(&task->signal->adj_node, &tasks_scoreadj);
	spin_unlock(&lmk_lock);
}

/*
 * Makesure to invoke the function with holding sighand->siglock
 */
void delete_from_adj_tree(struct task_struct *task)
{
	spin_lock(&lmk_lock);
	if (!RB_EMPTY_NODE(&task->signal->adj_node)) {
		rb_erase(&task->signal->adj_node, &tasks_scoreadj);
		RB_CLEAR_NODE(&task->signal->adj_node);
	}
	spin_unlock(&lmk_lock);
}

static struct task_struct *pick_next_from_adj_tree(struct task_struct *task)
{
	struct rb_node *next;
	struct signal_struct *next_tsk_sig;

	spin_lock(&lmk_lock);
	next = rb_next(&task->signal->adj_node);
	spin_unlock(&lmk_lock);

	if (!next)
		return NULL;

	next_tsk_sig = rb_entry(next, struct signal_struct, adj_node);
	return next_tsk_sig->curr_target->group_leader;
}

static struct task_struct *pick_first_task(void)
{
	struct rb_node *left;
	struct signal_struct *first_tsk_sig;

	spin_lock(&lmk_lock);
	left = rb_first(&tasks_scoreadj);
	spin_unlock(&lmk_lock);

	if (!left)
		return NULL;

	first_tsk_sig = rb_entry(left, struct signal_struct, adj_node);
	return first_tsk_sig->curr_target->group_leader;
}
static struct task_struct *pick_last_task(void)
{
	struct rb_node *right;
	struct signal_struct *last_tsk_sig;

	spin_lock(&lmk_lock);
	right = rb_last(&tasks_scoreadj);
	spin_unlock(&lmk_lock);

	if (!right)
		return NULL;

	last_tsk_sig = rb_entry(right, struct signal_struct, adj_node);
	return last_tsk_sig->curr_target->group_leader;
}
#endif


module_param_named(cost, lowmem_shrinker.seeks, int, S_IRUGO | S_IWUSR);
#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_AUTODETECT_OOM_ADJ_VALUES
__module_param_call(MODULE_PARAM_PREFIX, adj,
		    &lowmem_adj_array_ops,
		    .arr = &__param_arr_adj,
		    S_IRUGO | S_IWUSR, 0644);
__MODULE_PARM_TYPE(adj, "array of short");
#else
module_param_array_named(adj, lowmem_adj, short, &lowmem_adj_size,
			 S_IRUGO | S_IWUSR);
#endif
module_param_array_named(minfree, lowmem_minfree, uint, &lowmem_minfree_size,
			 S_IRUGO | S_IWUSR);
module_param_named(debug_level, lowmem_debug_level, uint, S_IRUGO | S_IWUSR);
module_param_named(lmkcount, lowmem_lmkcount, uint, S_IRUGO);
module_param_named(lmk_fast_run, lmk_fast_run, int, S_IRUGO | S_IWUSR);
#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_DO_NOT_KILL_PROCESS
module_param_named(donotkill_proc, donotkill_proc.enabled, uint, S_IRUGO | S_IWUSR);
module_param_array_named(donotkill_proc_names, donotkill_proc.names, charp,
			 &donotkill_proc.names_count, S_IRUGO | S_IWUSR);
module_param_named(donotkill_sysproc, donotkill_sysproc.enabled, uint, S_IRUGO | S_IWUSR);
module_param_array_named(donotkill_sysproc_names, donotkill_sysproc.names, charp,
			 &donotkill_sysproc.names_count, S_IRUGO | S_IWUSR);
#endif

#ifdef OOM_COUNT_READ
module_param_named(oomcount, oom_count, uint, S_IRUGO);
#endif

module_init(lowmem_init);
module_exit(lowmem_exit);

MODULE_LICENSE("GPL");
