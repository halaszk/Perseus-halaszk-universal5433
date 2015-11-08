#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/fb.h>
#include <linux/pm_qos.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/sort.h>
#include <linux/reboot.h>
#include <linux/debugfs.h>
#include <linux/sysfs.h>
#include <linux/exynos-interface.h>

#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>

#include <mach/cpufreq.h>
#include <linux/suspend.h>

//#define DM_HOTPLUG_DEBUG
#ifdef DM_HOTPLUG_DEBUG
#define dm_dbg(__dev, format, args...) pr_info(__dev, format, ##args)
#else
#define dm_dbg(__dev, format, args...) ((void)0)
#endif

#if defined(CONFIG_SOC_EXYNOS5430)
#define NORMALMIN_FREQ	1000000
#else
#define NORMALMIN_FREQ	500000
#endif
#define POLLING_MSEC	100
#define DEFAULT_LOW_STAY_THRSHD	0

#define MIN_NUM_ONLINE_CPU	1
#define MAX_NUM_ONLINE_CPU	NR_CPUS

unsigned int min_num_cpu;
unsigned int max_num_cpu;

struct cpu_load_info {
	cputime64_t cpu_idle;
	cputime64_t cpu_iowait;
	cputime64_t cpu_wall;
	cputime64_t cpu_nice;
};

static DEFINE_PER_CPU(struct cpu_load_info, cur_cpu_info);
static DEFINE_MUTEX(dm_hotplug_lock);
static DEFINE_MUTEX(thread_lock);
static DEFINE_MUTEX(big_hotplug_lock);
static DEFINE_MUTEX(little_hotplug_in_lock);
#ifdef CONFIG_HOTPLUG_THREAD_STOP
static DEFINE_MUTEX(thread_manage_lock);
#endif

static struct task_struct *dm_hotplug_task;
#ifdef CONFIG_HOTPLUG_THREAD_STOP
static bool thread_start = false;
#endif
static unsigned int low_stay_threshold = DEFAULT_LOW_STAY_THRSHD;
static int cpu_util[NR_CPUS];
static unsigned int cur_load_freq = 0;
static bool lcd_is_on = true;
static bool forced_hotplug = false;
static bool in_low_power_mode = false;
static bool in_suspend_prepared = false;
static bool do_enable_hotplug = false;
static bool do_disable_hotplug = false;
#if defined(CONFIG_SCHED_HMP)
static bool do_hotplug_out = false;
static int big_hotpluged = 0;
static int little_hotplug_in = 0;
#define DEFAULT_NR_RUN_THRESHD	200
#define DEFAULT_NR_RUN_RANGE	2
static unsigned int nr_running_threshold = DEFAULT_NR_RUN_THRESHD;
static unsigned int nr_running_range = DEFAULT_NR_RUN_RANGE;
static unsigned int nr_running_count = 0;
static unsigned long cur_nr_running;
static bool little_in_by_nr_running = false;
#endif
#ifdef CONFIG_ARM_EXYNOS_MP_CPUFREQ
static unsigned int egl_min_freq;
static unsigned int kfc_max_freq;
#endif
int disable_dm_hotplug_before_suspend = 0;
int nr_sleep_prepare_cpus = CONFIG_EXYNOS5_DYNAMIC_CPU_HOTPLUG_SLEEP_PREPARE;

enum hotplug_cmd {
	CMD_NORMAL,
	CMD_LOW_POWER,
	CMD_BIG_IN,
	CMD_BIG_OUT,
	CMD_LITTLE_IN,
	CMD_LITTLE_ONE_IN,
	CMD_LITTLE_ONE_OUT,
	CMD_SLEEP_PREPARE,
};

static int on_run(void *data);
static int dynamic_hotplug(enum hotplug_cmd cmd);
static enum hotplug_cmd diagnose_condition(void);
static void calc_load(void);

static enum hotplug_cmd prev_cmd = CMD_NORMAL;
static enum hotplug_cmd exe_cmd;
static unsigned int delay = POLLING_MSEC;
static unsigned int out_delay = POLLING_MSEC;
static unsigned int in_delay = POLLING_MSEC;

#if defined(CONFIG_SCHED_HMP)
static struct workqueue_struct *hotplug_wq;
#endif
static struct workqueue_struct *force_hotplug_wq;
#ifdef CONFIG_HOTPLUG_THREAD_STOP
static struct workqueue_struct *thread_manage_wq;
#endif
static struct workqueue_struct *unblank_wq;

static int dm_hotplug_disable = 0;

static bool dualcore_blank = true;

static int exynos_dm_hotplug_disabled(void)
{
	return dm_hotplug_disable;
}
#ifdef CONFIG_ARGOS
void exynos_dm_hotplug_enable(void)
#else
static void exynos_dm_hotplug_enable(void)
#endif
{
	mutex_lock(&dm_hotplug_lock);
	if (!exynos_dm_hotplug_disabled()) {
		pr_warn("%s: dynamic hotplug already enabled\n",
				__func__);
		mutex_unlock(&dm_hotplug_lock);
		return;
	}
	dm_hotplug_disable--;
	if (!in_suspend_prepared)
		disable_dm_hotplug_before_suspend--;
	mutex_unlock(&dm_hotplug_lock);
}
#ifdef CONFIG_ARGOS
void exynos_dm_hotplug_disable(void)
#else
static void exynos_dm_hotplug_disable(void)
#endif
{
	mutex_lock(&dm_hotplug_lock);
	dm_hotplug_disable++;
	if (!in_suspend_prepared)
		disable_dm_hotplug_before_suspend++;
	mutex_unlock(&dm_hotplug_lock);
}

#ifdef CONFIG_PM
static ssize_t show_enable_dm_hotplug(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	int disabled = exynos_dm_hotplug_disabled();

	return snprintf(buf, 10, "%s\n", disabled ? "disabled" : "enabled");
}

static ssize_t store_enable_dm_hotplug(struct kobject *kobj, struct attribute *attr,
					const char *buf, size_t count)
{
	int enable_input;

	if (!sscanf(buf, "%d", &enable_input))
		return -EINVAL;

	if (enable_input > 3 || enable_input < 0) {
		pr_err("%s: invalid value (%d)\n", __func__, enable_input);
		return -EINVAL;
	}

	if (enable_input == 3) {
		pr_info("%s: disabling dualcore mode on screen-off\n", __func__);
		dualcore_blank = false;
	} else if (enable_input == 2) {
		pr_info("%s: enabling dualcore mode on screen-off\n", __func__);
		dualcore_blank = true;
	} else if (enable_input == 1) {
		do_enable_hotplug = true;
		if (exynos_dm_hotplug_disabled())
			exynos_dm_hotplug_enable();
		else
			pr_info("%s: dynamic hotplug already enabled\n",
					__func__);
#if defined(CONFIG_SCHED_HMP)
		if (big_hotpluged)
			dynamic_hotplug(CMD_BIG_OUT);
#endif
		do_enable_hotplug = false;
	} else {
		do_disable_hotplug = true;
		if (!dynamic_hotplug(CMD_NORMAL))
			prev_cmd = CMD_NORMAL;
		if (!exynos_dm_hotplug_disabled())
			exynos_dm_hotplug_disable();
		else
			pr_info("%s: dynamic hotplug already disabled\n",
					__func__);
		do_disable_hotplug = false;
	}

	return count;
}

#if defined(CONFIG_SCHED_HMP)
static ssize_t show_little_core1_hotplug_in(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	if (exynos_dm_hotplug_disabled())
		return snprintf(buf, PAGE_SIZE, "dynamic hotplug disabled\n");

	return snprintf(buf, PAGE_SIZE, "%s on low power mode\n",
			little_hotplug_in ? "hotplug-in" : "hotplug-out");
}

static ssize_t store_little_core1_hotplug_in(struct kobject *kobj,
				struct attribute *attr, const char *buf, size_t count)
{
	int input_little_hotplug_in;

	if (!sscanf(buf, "%1d", &input_little_hotplug_in))
		return -EINVAL;

	if (input_little_hotplug_in > 1 || input_little_hotplug_in < 0) {
		pr_err("%s: invalid value (%d)\n", __func__, input_little_hotplug_in);
		return -EINVAL;
	}

	little_core1_hotplug_in((bool)input_little_hotplug_in);

	return count;
}

static ssize_t show_hotplug_nr_running(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "current_nr_running = %lu, "
			"nr_running_threshold = %u, nr_running_range = %u\n",
			cur_nr_running, nr_running_threshold, nr_running_range);
}

static ssize_t store_hotplug_nr_running(struct kobject *kobj,
				struct attribute *attr, const char *buf, size_t count)
{
	int input_nr_running_thrshd;
	int input_nr_running_range;

	if (!sscanf(buf, "%5d %5d", &input_nr_running_thrshd, &input_nr_running_range))
		return -EINVAL;

	if (input_nr_running_thrshd <= 1 || input_nr_running_range < 1) {
		pr_err("%s: invalid values (thrshd = %d, range = %d)\n",
			__func__, input_nr_running_thrshd, input_nr_running_range);
		pr_err("%s: thrshd is should be over than 1,"
			" and range is should be over than 0\n", __func__);
		return -EINVAL;
	}

	nr_running_threshold = (unsigned int)input_nr_running_thrshd;
	nr_running_range = (unsigned int)input_nr_running_range;

	pr_info("%s: nr_running_threshold = %u, nr_running_range = %u\n",
		__func__, nr_running_threshold, nr_running_range);

	return count;
}
#endif

static ssize_t show_stay_threshold(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", low_stay_threshold);
}

static ssize_t store_stay_threshold(struct kobject *kobj, struct attribute *attr,
					const char *buf, size_t count)
{
	int input_threshold;

	if (!sscanf(buf, "%d", &input_threshold))
		return -EINVAL;

	if (input_threshold < 0) {
		pr_err("%s: invalid value (%d)\n", __func__, input_threshold);
		return -EINVAL;
	}

	low_stay_threshold = (unsigned int)input_threshold;

	return count;
}

static ssize_t show_dm_hotplug_delay(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "hotplug delay (out : %umsec, in : %umsec, cur : %umsec)\n",
				out_delay, in_delay, delay);
}

static ssize_t store_dm_hotplug_delay(struct kobject *kobj, struct attribute *attr,
					const char *buf, size_t count)
{
	int input_out_delay, input_in_delay;

	if (!sscanf(buf, "%d %d", &input_out_delay, &input_in_delay))
		return -EINVAL;

	if (input_out_delay < 0 || input_in_delay < 0) {
		pr_err("%s: invalid value (%d, %d)\n",
			__func__, input_out_delay, input_in_delay);
		return -EINVAL;
	}

	out_delay = (unsigned int)input_out_delay;
	in_delay = (unsigned int)input_in_delay;

	if (in_low_power_mode)
		delay = in_delay;
	else
		delay = out_delay;

	return count;
}


static ssize_t show_cpucore_table(struct kobject *kobj,
			     struct attribute *attr, char *buf)
{
	int i, num_cpu, count = 0;

	num_cpu = num_online_cpus();
	for (i = num_cpu; i > 0; i--)
		count += sprintf(&buf[count], "%d ", i);

	count += sprintf(&buf[count], "\n");
	return count;
}

static ssize_t show_cpucore_min_num_limit(struct kobject *kobj,
			     struct attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", min_num_cpu);
}

static ssize_t show_cpucore_max_num_limit(struct kobject *kobj,
			     struct attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", max_num_cpu);
}

static ssize_t store_cpucore_min_num_limit(struct kobject *kobj,
			struct attribute *attr, const char *buf, size_t count)
{
	int input;

	if (!sscanf(buf, "%u", &input))
		return -EINVAL;

	if (input < 0 || input > 7) {
		pr_err("Must keep input range 0 ~ 7\n");
		return -EINVAL;
	}

	pr_info("Not yet supported\n");

	min_num_cpu = input;

	return count;
}

static ssize_t store_cpucore_max_num_limit(struct kobject *kobj,
			struct attribute *attr, const char *buf, size_t count)
{
	int input, delta, cpu;

	if (!sscanf(buf, "%u", &input))
		return -EINVAL;

	if (input < 1 || input > 8) {
		pr_err("Must keep input range 1 ~ 8\n");
		return -EINVAL;
	}

	delta = input - num_online_cpus();

	if (delta > 0) {
		cpu = 1;
		while (delta) {
			if (!cpu_online(cpu)) {
				cpu_up(cpu);
				delta--;
			}
			cpu++;
		}
	} else if (delta < 0) {
		cpu = 7;
		while (delta) {
			if (cpu_online(cpu)) {
				cpu_down(cpu);
				delta++;
			}
			cpu--;
		}
	}

	max_num_cpu = input;

	return count;
}

static struct global_attr enable_dm_hotplug =
		__ATTR(enable_dm_hotplug, S_IRUGO | S_IWUSR,
			show_enable_dm_hotplug, store_enable_dm_hotplug);

#if defined(CONFIG_SCHED_HMP)
static struct global_attr little_core_hotplug_in =
		__ATTR(little_core_hotplug_in, S_IRUGO | S_IWUSR,
			show_little_core1_hotplug_in, store_little_core1_hotplug_in);

static struct global_attr hotplug_nr_running =
		__ATTR(hotplug_nr_running, S_IRUGO | S_IWUSR,
			show_hotplug_nr_running, store_hotplug_nr_running);
#endif

static struct global_attr dm_hotplug_stay_threshold =
		__ATTR(dm_hotplug_stay_threshold, S_IRUGO | S_IWUSR,
			show_stay_threshold, store_stay_threshold);

static struct global_attr dm_hotplug_delay =
		__ATTR(dm_hotplug_delay, S_IRUGO | S_IWUSR,
			show_dm_hotplug_delay, store_dm_hotplug_delay);
			
static struct sysfs_attr cpucore_table =
		__ATTR(cpucore_table, S_IRUGO,
			show_cpucore_table, NULL);
			
static struct sysfs_attr cpucore_min_num_limit =
		__ATTR(cpucore_min_num_limit, S_IRUGO | S_IWUSR,
			show_cpucore_min_num_limit,
			store_cpucore_min_num_limit);
			
static struct sysfs_attr cpucore_max_num_limit =
		__ATTR(cpucore_max_num_limit, S_IRUGO | S_IWUSR,
			show_cpucore_max_num_limit,
			store_cpucore_max_num_limit);

#endif

static inline u64 get_cpu_idle_time_jiffy(unsigned int cpu, u64 *wall)
{
	u64 idle_time;
	u64 cur_wall_time;
	u64 busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());

	busy_time  = kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_STEAL];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];

	idle_time = cur_wall_time - busy_time;
	if (wall)
		*wall = jiffies_to_usecs(cur_wall_time);

	return jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);
	else
		idle_time += get_cpu_iowait_time_us(cpu, wall);

	return idle_time;
}

static inline cputime64_t get_cpu_iowait_time(unsigned int cpu, cputime64_t *wall)
{
	u64 iowait_time = get_cpu_iowait_time_us(cpu, wall);

	if (iowait_time == -1ULL)
		return 0;

	return iowait_time;
}

#ifdef CONFIG_HOTPLUG_THREAD_STOP
static void thread_manage_work(struct work_struct *work)
{
	mutex_lock(&thread_manage_lock);
	if (thread_start) {
		dm_hotplug_task =
			kthread_create(on_run, NULL, "thread_hotplug");
		if (IS_ERR(dm_hotplug_task)) {
			pr_err("Failed in creation of thread.\n");
			return;
		}

		wake_up_process(dm_hotplug_task);
		
				if (!dynamic_hotplug(CMD_LOW_POWER))
			prev_cmd = CMD_LOW_POWER;
			
	} else {
		if (dm_hotplug_task) {
			kthread_stop(dm_hotplug_task);
			dm_hotplug_task = NULL;
			if (!dynamic_hotplug(CMD_NORMAL))
				prev_cmd = CMD_NORMAL;
		}
		dm_hotplug_disable = 1;
	}
	mutex_unlock(&thread_manage_lock);
}

static DECLARE_WORK(manage_work, thread_manage_work);
#endif

static void unblank_work_fn(struct work_struct *work)
{
	if (lcd_is_on)
		return;

	lcd_is_on = true;
	pr_info("LCD is on\n");

#ifdef CONFIG_HOTPLUG_THREAD_STOP
	if (thread_manage_wq) {
		if (work_pending(&manage_work))
			flush_work(&manage_work);
		thread_start = false;
		queue_work(thread_manage_wq, &manage_work);
	}
#endif
}
static DECLARE_WORK(unblank_work, unblank_work_fn);

void force_unblank(void)
{
	if (unblank_wq)
		queue_work(unblank_wq, &unblank_work);
}

static int fb_state_change(struct notifier_block *nb,
		unsigned long val, void *data)
{
	struct fb_event *evdata = data;
	struct fb_info *info = evdata->info;
	unsigned int blank;

	if (val != FB_EVENT_BLANK &&
		val != FB_R_EARLY_EVENT_BLANK)
		return 0;
	/*
	 * If FBNODE is not zero, it is not primary display(LCD)
	 * and don't need to process these scheduling.
	 */
	if (info->node)
		return NOTIFY_OK;

	blank = *(int *)evdata->data;

	switch (blank) {
	case FB_BLANK_POWERDOWN:
		lcd_is_on = false;
		pr_info("LCD is off\n");

#ifdef CONFIG_HOTPLUG_THREAD_STOP
		dm_hotplug_disable = 0;
		if (thread_manage_wq) {
			if (work_pending(&manage_work))
				flush_work(&manage_work);
			thread_start = true;
			queue_work(thread_manage_wq, &manage_work);
		}
#endif
		break;
	case FB_BLANK_UNBLANK:
		/*
		 * LCD blank CPU qos is set by exynos-ikcs-cpufreq
		 * This line of code release max limit when LCD is
		 * turned on.
		 */
		if (unblank_wq)
			queue_work(unblank_wq, &unblank_work);

		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block fb_block = {
	.notifier_call = fb_state_change,
};

static int __ref __cpu_hotplug(bool out_flag, enum hotplug_cmd cmd)
{
	int i = 0;
	int ret = 0;
#ifdef DM_HOTPLUG_DEBUG
	char cmddesc[25];

	switch (cmd) {
	case CMD_LOW_POWER:
		strcpy(cmddesc, "CMD_LOW_POWER");
		break;
	case CMD_LITTLE_ONE_OUT:
		strcpy(cmddesc, "CMD_LITTLE_ONE_OUT");
		break;
	case CMD_BIG_OUT:
		strcpy(cmddesc, "CMD_BIG_OUT");
		break;
	case CMD_SLEEP_PREPARE:
		strcpy(cmddesc, "CMD_SLEEP_PREPARE");
		break;
	case CMD_LITTLE_ONE_IN:
		strcpy(cmddesc, "CMD_LITTLE_ONE_IN");
		break;
	case CMD_BIG_IN:
		strcpy(cmddesc, "CMD_BIG_IN");
		break;
	case CMD_LITTLE_IN:
		strcpy(cmddesc, "CMD_LITTLE_IN");
		break;
	case CMD_NORMAL:
		strcpy(cmddesc, "CMD_NORMAL");
		break;
	}
#endif

	if (exynos_dm_hotplug_disabled())
		return 0;

#if defined(CONFIG_SCHED_HMP)
	if (out_flag) {
		if (do_disable_hotplug)
			goto blk_out;
			
				if (cmd == CMD_SLEEP_PREPARE) {
			dm_dbg("%s: 1, %s\n", __func__, cmddesc);
			for (i = max_num_cpu - 1; i >= NR_CA7; i--) {
                                if (cpu_online(i)) {
                                        ret = cpu_down(i);
                                        if (ret)
                                                goto blk_out;
                                }
			}
			dm_dbg("%s: 2, %s\n", __func__, cmddesc);
			for (i = 1; i < nr_sleep_prepare_cpus; i++) {
				if (!cpu_online(i)) {
					ret = cpu_up(i);
					if (ret)
						goto blk_out;
				}
			}
		}

		else if (cmd == CMD_BIG_OUT && !in_low_power_mode) {
			dm_dbg("%s: 3, %s\n", __func__, cmddesc);
			for (i = max_num_cpu - 1; i >= NR_CA7; i--) {
				if (cpu_online(i)) {
					ret = cpu_down(i);
					if (ret)
						goto blk_out;
				}
			}
		} else {
			if (cmd == CMD_LITTLE_ONE_OUT) {
				if (!in_low_power_mode)
					goto blk_out;

				dm_dbg("%s: 4, %s\n", __func__, cmddesc);
				for (i = NR_CA7 - 2; i > 0; i--) {
					if (cpu_online(i)) {
						ret = cpu_down(i);
						if (ret)
							goto blk_out;
					}
				}
			} else {
				dm_dbg("%s: 5, %s\n", __func__, cmddesc);

				for (i = max_num_cpu - 1; i > 0; i--) {
					if (cpu_online(i)) {
						ret = cpu_down(i);
						if (ret)
							goto blk_out;
					}
				}
			}
		}
	} else {
		if (in_suspend_prepared)
			goto blk_out;

		if (cmd == CMD_BIG_IN) {
			if (in_low_power_mode)
				goto blk_out;

			dm_dbg("%s: 6, %s\n", __func__, cmddesc);
			for (i = NR_CA7; i < max_num_cpu; i++) {
				if (!cpu_online(i)) {
					ret = cpu_up(i);
					if (ret)
						goto blk_out;
				}
			}
		} else {
			if (cmd == CMD_LITTLE_ONE_IN) {
				if (!lcd_is_on && dualcore_blank) {
					dm_dbg("%s: 7, %s\n", __func__, cmddesc);
					if (!cpu_online(1)) {
						ret = cpu_up(1);
						if (ret)
							goto blk_out;
					}
				} else {
					dm_dbg("%s: 8, %s\n", __func__, cmddesc);
					for (i = 1; i < NR_CA7 - 1; i++) {
						if (!cpu_online(i)) {
							ret = cpu_up(i);
							if (ret)
								goto blk_out;
						}
					}
				}
			} else if ((big_hotpluged && !do_disable_hotplug) ||
				(cmd == CMD_LITTLE_IN)) {
			if (!lcd_is_on && dualcore_blank) {
					dm_dbg("%s: 9, %s\n", __func__, cmddesc);
					if (!cpu_online(1)) {
						ret = cpu_up(1);
						if (ret)
							goto blk_out;
					}
				} else {
					dm_dbg("%s: 10, %s\n", __func__, cmddesc);
					for (i = 1; i < NR_CA7; i++) {
						if (!cpu_online(i)) {
							ret = cpu_up(i);
							if (ret)
								goto blk_out;
						}
					}
				}
			} else {
				if (lcd_is_on) {
					dm_dbg("%s: 11, %s\n", __func__, cmddesc);
					for (i = NR_CA7; i < max_num_cpu; i++) {
						if (do_hotplug_out)
							goto blk_out;

						if (!cpu_online(i)) {
							if (i == NR_CA7)
								set_hmp_boostpulse(100000);

							ret = cpu_up(i);
							if (ret)
								goto blk_out;
						}
					}
	
					dm_dbg("%s: 12, %s\n", __func__, cmddesc);
					for (i = 1; i < NR_CA7; i++) {
						if (!cpu_online(i)) {
							ret = cpu_up(i);
							if (ret)
								goto blk_out;
						}
					}
				} else {
					dm_dbg("%s: 13, %s\n", __func__, cmddesc);
					for (i = 1; i < max_num_cpu; i++) {
						if (do_hotplug_out && i >= NR_CA7)
							goto blk_out;

						if (!cpu_online(i)) {
							ret = cpu_up(i);
							if (ret)
								goto blk_out;
						}
					}
				}
			}
		}
	}
#else
	if (out_flag) {
		if (do_disable_hotplug)
			goto blk_out;

		dm_dbg("%s: 14, %s\n", __func__, cmddesc);
		for (i = max_num_cpu - 1; i > 0; i--) {
			if (cpu_online(i)) {
				ret = cpu_down(i);
				if (ret)
					goto blk_out;
			}
		}
	} else {
		if (in_suspend_prepared)
			goto blk_out;

		dm_dbg("%s: 15, %s\n", __func__, cmddesc);
		for (i = 1; i < max_num_cpu; i++) {
			if (!cpu_online(i)) {
				ret = cpu_up(i);
				if (ret)
					goto blk_out;
			}
		}
	}
#endif

blk_out:
	return ret;
}

static int dynamic_hotplug(enum hotplug_cmd cmd)
{
	int ret = 0;

	mutex_lock(&dm_hotplug_lock);

	switch (cmd) {
	case CMD_LOW_POWER:
		ret = __cpu_hotplug(true, cmd);
		in_low_power_mode = true;
		delay = in_delay;
		break;
	case CMD_LITTLE_ONE_OUT:
	case CMD_BIG_OUT:
	case CMD_SLEEP_PREPARE:
		ret = __cpu_hotplug(true, cmd);
		break;
	case CMD_LITTLE_ONE_IN:
	case CMD_BIG_IN:
		ret = __cpu_hotplug(false, cmd);
		break;
	case CMD_LITTLE_IN:
	case CMD_NORMAL:
		ret = __cpu_hotplug(false, cmd);
		in_low_power_mode = false;
		delay = out_delay;
		break;
	}

	mutex_unlock(&dm_hotplug_lock);

	return ret;
}

static bool force_out_flag;
static void force_dynamic_hotplug_work(struct work_struct *work)
{
	enum hotplug_cmd cmd;

	forced_hotplug = force_out_flag;

	calc_load();
	cmd = diagnose_condition();

	if (!dynamic_hotplug(cmd))
		prev_cmd = cmd;
}

static DECLARE_WORK(force_hotplug_work, force_dynamic_hotplug_work);

void force_dynamic_hotplug(bool out_flag, int delay_msec)
{
	if (force_hotplug_wq) {
		force_out_flag = out_flag;
		queue_work(force_hotplug_wq, &force_hotplug_work);
	}
}

#if defined(CONFIG_SCHED_HMP)
int big_cores_hotplug(bool out_flag)
{
	int ret = 0;

	mutex_lock(&big_hotplug_lock);

	if (out_flag) {
		do_hotplug_out = true;
		if (big_hotpluged) {
			big_hotpluged++;
			do_hotplug_out = false;
			goto out;
		}

		ret = dynamic_hotplug(CMD_BIG_OUT);
		if (!ret) {
			big_hotpluged++;
			do_hotplug_out = false;
		}
	} else {
		if (WARN_ON(big_hotpluged == 0)) {
			pr_err("%s: big cores already hotplug in\n",
					__func__);
			ret = -EINVAL;
			goto out;
		}

		if (big_hotpluged > 1) {
			big_hotpluged--;
			goto out;
		}

		ret = dynamic_hotplug(CMD_BIG_IN);
		if (!ret)
			big_hotpluged--;
	}

out:
	mutex_unlock(&big_hotplug_lock);

	return ret;
}

int little_core1_hotplug_in(bool in_flag)
{
	int ret = 0;

	mutex_lock(&little_hotplug_in_lock);

	if (in_flag) {
		if (little_hotplug_in) {
			little_hotplug_in++;
			goto out;
		}

		ret = dynamic_hotplug(CMD_LITTLE_ONE_IN);
		if (!ret)
			little_hotplug_in++;
	} else {
		if (WARN_ON(little_hotplug_in == 0)) {
			pr_err("%s: little core1 already hotplug out\n",
					__func__);
			ret = -EINVAL;
			goto out;
		}

		if (little_hotplug_in > 1) {
			little_hotplug_in--;
			goto out;
		}

		ret = dynamic_hotplug(CMD_LITTLE_ONE_OUT);
		if (!ret)
			little_hotplug_in--;
	}

out:
	mutex_unlock(&little_hotplug_in_lock);

	return ret;
}

static void event_hotplug_in_work(struct work_struct *work)
{
	if(!dynamic_hotplug(CMD_NORMAL))
		prev_cmd = CMD_NORMAL;
	else
		pr_err("%s: failed hotplug in\n", __func__);
}

static DECLARE_WORK(hotplug_in_work, event_hotplug_in_work);

void event_hotplug_in(void)
{
	if (hotplug_wq && lcd_is_on)
		queue_work(hotplug_wq, &hotplug_in_work);
}
#endif

static int exynos_dm_hotplug_notifier(struct notifier_block *notifier,
					unsigned long pm_event, void *v)
{
	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		mutex_lock(&thread_lock);
		in_suspend_prepared = true;
		if(nr_sleep_prepare_cpus > 1) {
			pr_info("%s, %d : dynamic_hotplug CMD_SLEEP_PREPARE\n", __func__, __LINE__);
			if (!dynamic_hotplug(CMD_SLEEP_PREPARE))
				prev_cmd = CMD_LOW_POWER;
		}
		else {
		if (!dynamic_hotplug(CMD_LOW_POWER))
			prev_cmd = CMD_LOW_POWER;
		}
		exynos_dm_hotplug_disable();
		if (dm_hotplug_task) {
			kthread_stop(dm_hotplug_task);
			dm_hotplug_task = NULL;
		}
		mutex_unlock(&thread_lock);
		break;

	case PM_POST_SUSPEND:
		mutex_lock(&thread_lock);
		exynos_dm_hotplug_enable();

		dm_hotplug_task =
			kthread_create(on_run, NULL, "thread_hotplug");
		if (IS_ERR(dm_hotplug_task)) {
			mutex_unlock(&thread_lock);
			pr_err("Failed in creation of thread.\n");
			return -EINVAL;
		}

		in_suspend_prepared = false;

		wake_up_process(dm_hotplug_task);
		mutex_unlock(&thread_lock);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block exynos_dm_hotplug_nb = {
	.notifier_call = exynos_dm_hotplug_notifier,
	.priority = 1,
};

static int exynos_dm_hotplut_reboot_notifier(struct notifier_block *this,
				unsigned long code, void *_cmd)
{
	switch (code) {
	case SYSTEM_POWER_OFF:
	case SYS_RESTART:
		mutex_lock(&thread_lock);
		exynos_dm_hotplug_disable();
		if (dm_hotplug_task) {
			kthread_stop(dm_hotplug_task);
			dm_hotplug_task = NULL;
		}
		mutex_unlock(&thread_lock);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block exynos_dm_hotplug_reboot_nb = {
	.notifier_call = exynos_dm_hotplut_reboot_notifier,
};

#ifdef CONFIG_SCHED_HMP
extern unsigned long avg_nr_running(void);
static void update_nr_running_count(void)
{
	cur_nr_running = (avg_nr_running() * 100) >> FSHIFT;

	if (cur_nr_running >= nr_running_threshold) {
		if (nr_running_count < nr_running_range)
			nr_running_count++;
	} else {
		if (nr_running_count > 0)
			nr_running_count--;
	}

	if (nr_running_count) {
		if (!little_in_by_nr_running) {
			little_core1_hotplug_in(true);
			little_in_by_nr_running = true;
		}
	} else {
		if (little_in_by_nr_running) {
			little_core1_hotplug_in(false);
			little_in_by_nr_running = false;
		}
	}
}
#endif

#ifdef CONFIG_ARM_EXYNOS_MP_CPUFREQ
extern bool cluster_on[CA_END];
#endif
static int low_stay = 0;

static enum hotplug_cmd diagnose_condition(void)
{
	enum hotplug_cmd ret;
	unsigned int normal_min_freq;
#ifdef CONFIG_ARM_EXYNOS_MP_CPUFREQ
	struct cpufreq_policy *policy;
	unsigned int egl_cur_freq;
#endif

#if defined(CONFIG_CPU_FREQ_GOV_INTERACTIVE)
	normal_min_freq = cpufreq_interactive_get_hispeed_freq(0);
	if (!normal_min_freq)
		normal_min_freq = NORMALMIN_FREQ;
#else
	normal_min_freq = NORMALMIN_FREQ;
#endif

#ifdef CONFIG_ARM_EXYNOS_MP_CPUFREQ
	policy = cpufreq_cpu_get(0);
	if (!policy) {
		kfc_max_freq = 0;
	} else {
		kfc_max_freq = policy->max;
		cpufreq_cpu_put(policy);
	}

	policy = cpufreq_cpu_get(NR_CA7);
	if (!policy) {
		egl_cur_freq = 0;
	} else {
		if (cluster_on[CA15])
			egl_cur_freq = policy->cur;
		else
			egl_cur_freq = 0;
		cpufreq_cpu_put(policy);
	}
#endif

#ifdef CONFIG_SCHED_HMP
	update_nr_running_count();
#endif

#if defined(CONFIG_ARM_EXYNOS_MP_CPUFREQ)
	ret = CMD_LITTLE_IN;

	if ((cur_load_freq >= kfc_max_freq) && lcd_is_on)
		ret = CMD_NORMAL;

	if ((cur_load_freq > normal_min_freq) ||
		(egl_cur_freq >= egl_min_freq) ||
		(pm_qos_request(PM_QOS_CPU_FREQ_MIN) >= egl_min_freq)) {
		if (in_low_power_mode)
			ret = CMD_LITTLE_IN;
#else
	ret = CMD_NORMAL;

	if (cur_load_freq > normal_min_freq) {
#endif
		low_stay = 0;
	} else if (cur_load_freq <= normal_min_freq &&
		low_stay <= low_stay_threshold) {
		low_stay++;
	}

	if (low_stay > low_stay_threshold &&
		(!lcd_is_on || forced_hotplug))
		ret = CMD_LOW_POWER;

#ifdef CONFIG_SCHED_HMP
	if (little_hotplug_in || little_in_by_nr_running) {
		int i;
		for (i = 1; i < NR_CA7; i++) {
			if (!cpu_online(i) && cur_load_freq <= normal_min_freq) {
				ret = CMD_LITTLE_ONE_IN;
				break;
			}
		}
	}
#endif

	return ret;
}

static void calc_load(void)
{
	struct cpufreq_policy *policy;
	unsigned int cpu_util_sum = 0;
	int cpu = 0;
	unsigned int i;

	policy = cpufreq_cpu_get(cpu);

	if (!policy) {
		pr_err("Invalid policy\n");
		return;
	}

	cur_load_freq = 0;

	for_each_cpu(i, policy->cpus) {
		struct cpu_load_info	*i_load_info;
		cputime64_t cur_wall_time, cur_idle_time, cur_iowait_time;
		unsigned int idle_time, wall_time, iowait_time;
		unsigned int load, load_freq;

		i_load_info = &per_cpu(cur_cpu_info, i);

		cur_idle_time = get_cpu_idle_time(i, &cur_wall_time);
		cur_iowait_time = get_cpu_iowait_time(i, &cur_wall_time);

		wall_time = (unsigned int)
			(cur_wall_time - i_load_info->cpu_wall);
		i_load_info->cpu_wall = cur_wall_time;

		idle_time = (unsigned int)
			(cur_idle_time - i_load_info->cpu_idle);
		i_load_info->cpu_idle = cur_idle_time;

		iowait_time = (unsigned int)
			(cur_iowait_time - i_load_info->cpu_iowait);
		i_load_info->cpu_iowait = cur_iowait_time;

		if (unlikely(!wall_time || wall_time < idle_time))
			continue;

		load = 100 * (wall_time - idle_time) / wall_time;
		cpu_util[i] = load;
		cpu_util_sum += load;

		load_freq = load * policy->cur;

		if (policy->cur > cur_load_freq)
			cur_load_freq = policy->cur;
	}

	cpufreq_cpu_put(policy);
	return;
}

static int on_run(void *data)
{
	int on_cpu = 0;
	int ret;

	struct cpumask thread_cpumask;

	cpumask_clear(&thread_cpumask);
	cpumask_set_cpu(on_cpu, &thread_cpumask);
	sched_setaffinity(0, &thread_cpumask);

	while (!kthread_should_stop()) {
		calc_load();
		exe_cmd = diagnose_condition();

#ifdef DM_HOTPLUG_DEBUG
		pr_info("frequency info : %d, prev_cmd %d, exe_cmd %d\n",
				cur_load_freq, prev_cmd, exe_cmd);
		pr_info("lcd is on : %d, low power mode = %d, dm_hotplug disable = %d\n",
				lcd_is_on, in_low_power_mode, exynos_dm_hotplug_disabled());
#if defined(CONFIG_SCHED_HMP)
		pr_info("big cores hotplug out : %d\n", big_hotpluged);
#endif
#endif
		if (exynos_dm_hotplug_disabled())
			goto sleep;

		if (prev_cmd != exe_cmd) {
			ret = dynamic_hotplug(exe_cmd);
			if (ret < 0) {
				if (ret == -EBUSY)
					goto sleep;
				else
					goto failed_out;
			}
		}

		prev_cmd = exe_cmd;

sleep:
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout_interruptible(msecs_to_jiffies(delay));
		set_current_state(TASK_RUNNING);
	}

	pr_info("stopped %s\n", dm_hotplug_task->comm);

	return 0;

failed_out:
	panic("%s: failed dynamic hotplug (exe_cmd %d)\n", __func__, exe_cmd);

	return ret;
}

bool is_big_hotpluged(void)
{
	return big_hotpluged ? true : false;
}

static struct dentry *cputime_debugfs;

static int cputime_debug_show(struct seq_file *s, void *unsued)
{
	seq_printf(s, "cputime %llu\n",
			(unsigned long long) cputime64_to_clock_t(get_jiffies_64()));

	return 0;
}

static int cputime_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, cputime_debug_show, inode->i_private);
}

const static struct file_operations cputime_fops = {
	.open		= cputime_debug_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init dm_cpu_hotplug_init(void)
{
	int ret = 0;
	min_num_cpu = 0;
	max_num_cpu = NR_CPUS;
#ifdef CONFIG_ARM_EXYNOS_MP_CPUFREQ
	struct cpufreq_policy *policy;
#endif

#ifndef CONFIG_HOTPLUG_THREAD_STOP
	dm_hotplug_task =
		kthread_create(on_run, NULL, "thread_hotplug");
	if (IS_ERR(dm_hotplug_task)) {
		pr_err("Failed in creation of thread.\n");
		return -EINVAL;
	}
#endif

	fb_register_client(&fb_block);

#ifdef CONFIG_PM
	ret = sysfs_create_file(power_kobj, &enable_dm_hotplug.attr);
	if (ret) {
		pr_err("%s: failed to create enable_dm_hotplug sysfs interface\n",
			__func__);
		goto err_enable_dm_hotplug;
	}

#if defined(CONFIG_SCHED_HMP)
	ret = sysfs_create_file(power_kobj, &little_core_hotplug_in.attr);
	if (ret) {
		pr_err("%s: failed to create little_core_hotplug_in sysfs interface\n",
			__func__);
		goto err_little_core_hotplug_in;
	}

	ret = sysfs_create_file(power_kobj, &hotplug_nr_running.attr);
	if (ret) {
		pr_err("%s: failed to create hotplug_nr_running sysfs interface\n",
			__func__);
		goto err_hotplug_nr_running;
	}
#endif

	ret = sysfs_create_file(power_kobj, &dm_hotplug_stay_threshold.attr);
	if (ret) {
		pr_err("%s: failed to create dm_hotplug_stay_threshold sysfs interface\n",
			__func__);
		goto err_dm_hotplug_stay_threshold;
	}

	ret = sysfs_create_file(power_kobj, &dm_hotplug_delay.attr);
	if (ret) {
		pr_err("%s: failed to create dm_hotplug_delay sysfs interface\n",
			__func__);
		goto err_dm_hotplug_delay;
	}
	
	ret = sysfs_create_file(power_kobj, &cpucore_table.attr);
	if (ret)
		goto err;

	ret = sysfs_create_file(power_kobj, &cpucore_min_num_limit.attr);
	if (ret)
		goto err;

	ret = sysfs_create_file(power_kobj, &cpucore_max_num_limit.attr);
	if (ret)
		goto err;
#endif

#ifdef CONFIG_ARM_EXYNOS_MP_CPUFREQ
	policy = cpufreq_cpu_get(NR_CA7);
	if (!policy) {
		pr_err("%s: invaled policy cpu%d\n", __func__, NR_CA7);
		ret = -ENODEV;
		goto err_policy;
	}

	egl_min_freq = policy->min;
	cpufreq_cpu_put(policy);
#endif

#if defined(CONFIG_SCHED_HMP)
	hotplug_wq = create_singlethread_workqueue("event-hotplug");
	if (!hotplug_wq) {
		ret = -ENOMEM;
		goto err_wq;
	}
#endif

	force_hotplug_wq = create_singlethread_workqueue("force-hotplug");
	if (!force_hotplug_wq) {
		ret = -ENOMEM;
		goto err_force_wq;
	}

#ifdef CONFIG_HOTPLUG_THREAD_STOP
	thread_manage_wq = create_singlethread_workqueue("thread-manage");
	if (!thread_manage_wq) {
		ret = -ENOMEM;
		goto err_thread_wq;
	}
#endif

	unblank_wq = create_singlethread_workqueue("dm-unblank");
	if (!unblank_wq) {
		ret = -ENOMEM;
		goto err_unblank_wq;
	}

	register_pm_notifier(&exynos_dm_hotplug_nb);
	register_reboot_notifier(&exynos_dm_hotplug_reboot_nb);

	cputime_debugfs =
		debugfs_create_file("cputime", S_IRUGO, NULL, NULL, &cputime_fops);
	if (IS_ERR_OR_NULL(cputime_debugfs)) {
		cputime_debugfs = NULL;
		pr_err("%s: debugfs_create_file() failed\n", __func__);
	}

#ifndef CONFIG_HOTPLUG_THREAD_STOP
	wake_up_process(dm_hotplug_task);
#endif

	return ret;
err_unblank_wq:
	destroy_workqueue(unblank_wq);
#ifdef CONFIG_HOTPLUG_THREAD_STOP
err_thread_wq:
	destroy_workqueue(force_hotplug_wq);
#endif
err_force_wq:
#if defined(CONFIG_SCHED_HMP)
	destroy_workqueue(hotplug_wq);
err_wq:
#endif
#ifdef CONFIG_ARM_EXYNOS_MP_CPUFREQ
err_policy:
#endif
#ifdef CONFIG_PM
	sysfs_remove_file(power_kobj, &dm_hotplug_delay.attr);
err_dm_hotplug_delay:
	sysfs_remove_file(power_kobj, &dm_hotplug_stay_threshold.attr);
err_dm_hotplug_stay_threshold:
#if defined(CONFIG_SCHED_HMP)
	sysfs_remove_file(power_kobj, &hotplug_nr_running.attr);
err_hotplug_nr_running:
	sysfs_remove_file(power_kobj, &little_core_hotplug_in.attr);
err_little_core_hotplug_in:
#endif
	sysfs_remove_file(power_kobj, &enable_dm_hotplug.attr);
err_enable_dm_hotplug:
#endif
err:
	pr_err("%s: failed to create sysfs interface\n", __func__);
	
	fb_unregister_client(&fb_block);
#ifndef CONFIG_HOTPLUG_THREAD_STOP
	kthread_stop(dm_hotplug_task);
#endif

	return ret;
}

late_initcall(dm_cpu_hotplug_init);
