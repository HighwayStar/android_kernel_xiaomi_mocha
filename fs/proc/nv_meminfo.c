/*
 *
 *  Copyright (c) 2014, NVIDIA CORPORATION.  All rights reserved.
 *
 *  nv_meminfo.c is used to display Nvidia specific internal meminfo counters
 *
 */

#include <linux/fs.h>
#include <linux/hugetlb.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/mmzone.h>
#include <linux/proc_fs.h>
#include <linux/quicklist.h>
#include <linux/seq_file.h>
#include <linux/swap.h>
#include <linux/vmstat.h>
#include <linux/atomic.h>
#include <linux/vmalloc.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include "internal.h"

#if defined(CONFIG_TEGRA_NVMAP)
#include <linux/nvmap.h>
#endif

unsigned long nv_reclaim_counter();
unsigned long nv_kswapd_counter();
static int nv_meminfo_proc_show(struct seq_file *m, void *v)
{
	/*
	 * Tagged format, for easy grepping and expansion.
	 */
	seq_printf(m,
			   "Reclaim Counter: %8lu\n"
			   "Kswapd Counter: %8lu\n"
		,
	    nv_reclaim_counter(),
		nv_kswapd_counter()
	);


	return 0;
}

static int nv_meminfo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, nv_meminfo_proc_show, NULL);
}

static const struct file_operations nv_meminfo_proc_fops = {
	.open		= nv_meminfo_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init nv_proc_meminfo_init(void)
{
	proc_create("nv_meminfo", 0, NULL, &nv_meminfo_proc_fops);
	return 0;
}
module_init(nv_proc_meminfo_init);

