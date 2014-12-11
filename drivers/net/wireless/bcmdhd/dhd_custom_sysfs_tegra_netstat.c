/*
 * drivers/net/wireless/bcmdhd/dhd_custom_sysfs_tegra_netstat.c
 *
 * NVIDIA Tegra Sysfs for BCMDHD driver
 *
 * Copyright (C) 2014 NVIDIA Corporation. All rights reserved.
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

#include "dhd_custom_sysfs_tegra.h"
#include "wlioctl.h"
#include "wldev_common.h"

/* 13+1+13+1+2 */
#define MAX_NETSTAT_PKT_SZ 26
static void
netstat_work_func(struct work_struct *work);

static unsigned int netstat_rate_ms = 10 * 1000;
static DECLARE_DELAYED_WORK(netstat_work, netstat_work_func);

static void
netstat_work_func(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	int fp = -1;
	char buf[1024];


	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = sys_open("/proc/net/tcp", O_RDONLY, 0);
	if (fp <= -1) {
		printk(KERN_ERR "could not open file\n");
		goto fail;
	}

	/* trigger show */
	while (sys_read(fp, buf, 1) == 1);
	sys_close(fp);

	fp = -1;
	fp = sys_open("/proc/net/udp", O_RDONLY, 0);
	if (fp <= -1) {
		printk(KERN_ERR "could not open file\n");
		goto fail;
	}

	/* trigger show */
	while (sys_read(fp, buf, 1) == 1);
	sys_close(fp);
	/* schedule next netstat */
fail:
	set_fs(old_fs);
	schedule_delayed_work(&netstat_work,
			msecs_to_jiffies(netstat_rate_ms));

}

void
tegra_sysfs_histogram_netstat_work_start(void)
{
//	pr_info("%s\n", __func__);
	if (netstat_rate_ms > 0)
		schedule_delayed_work(&netstat_work,
			msecs_to_jiffies(netstat_rate_ms));
}

void
netstat_save(char tag, struct sock *sp, int bucket)
{
	char data[64];
	struct inet_sock *inet = inet_sk(sp);
	__be32 dest = inet->inet_daddr;
	__be32 src  = inet->inet_rcv_saddr;
	__u16 destp       = ntohs(inet->inet_dport);
	__u16 srcp        = ntohs(inet->inet_sport);
	sprintf(data, "%08x%04x%08x%04x%02x", src, srcp,
			dest, destp, sp->sk_state);
	tcpdump_pkt_save(tag, "",
			__func__,
			__LINE__,
			data,
			MAX_NETSTAT_PKT_SZ,
			0);
}

void
netstat_tcp_save(char tag, struct sock *sp, int bucket)
{
	char data[64];
	struct inet_sock *inet = inet_sk(sp);
	const struct tcp_sock *tp = tcp_sk(sp);
	__be32 dest = inet->inet_daddr;
	__be32 src  = inet->inet_rcv_saddr;
	__u16 destp       = ntohs(inet->inet_dport);
	__u16 srcp        = ntohs(inet->inet_sport);
	sprintf(data, "%08x%04x%08x%04x%02x", src, srcp,
		dest, destp, sp->sk_state);
	tcpdump_pkt_save(tag, "",
			__func__,
			__LINE__,
			data,
			MAX_NETSTAT_PKT_SZ,
			0);
}

void
netstat_tcp_syn_save(char tag, struct sock *sp, struct request_sock *req, int bucket)
{
	char data[64];
	struct inet_sock *inet = inet_sk(sp);
	const struct inet_request_sock *ireq = inet_rsk(req);
	__be32 dest = ireq->rmt_addr;
	__be32 src  = ireq->loc_addr;
	__u16 destp       = ntohs(inet_sk(sp)->inet_dport);
	__u16 srcp        = ntohs(ireq->rmt_port);
	sprintf(data, "%08x%04x%08x%04x%02x", src, srcp,
			dest, destp, TCP_SYN_RECV);
	tcpdump_pkt_save(tag, "",
			__func__,
			__LINE__,
			data,
			MAX_NETSTAT_PKT_SZ,
			0);
}

void
netstat_tcp_wait_save(char tag, const struct inet_timewait_sock *sp, int bucket)
{
	char data[64];
	__be32 dest, src;
	__u16 destp, srcp;

	dest  = sp->tw_daddr;
	src   = sp->tw_rcv_saddr;
	destp = ntohs(sp->tw_dport);
	srcp  = ntohs(sp->tw_sport);

	sprintf(data, "%08x%04x%08x%04x%02x", src, srcp,
			dest, destp, sp->tw_substate);
	tcpdump_pkt_save(tag, "",
			__func__,
			__LINE__,
			data,
			MAX_NETSTAT_PKT_SZ,
			0);
}

void
tegra_sysfs_histogram_netstat_work_stop(void)
{
	cancel_delayed_work(&netstat_work);
	flush_scheduled_work();
}


ssize_t
tegra_sysfs_histogram_netstat_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	static int i;

	if (!i) {
		i++;
		strcpy(buf, "dummy netstat!");
		return strlen(buf);
	} else {
		i = 0;
		return 0;
	}
}

ssize_t
tegra_sysfs_histogram_netstat_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	int err;
	unsigned int uint;

	if (strncmp(buf, "enable", 6) == 0) {
		pr_info("%s: starting netstat delayed work...\n", __func__);
		tegra_sysfs_histogram_netstat_work_start();
	} else if (strncmp(buf, "disable", 7) == 0) {
		pr_info("%s: stopping netstat delayed work...\n", __func__);
		tegra_sysfs_histogram_netstat_work_stop();
	} else if (strncmp(buf, "rate ", 5) == 0) {
		err = kstrtouint(buf + 5, 0, &uint);
		if (err < 0) {
			pr_err("%s: invalid netstat rate (ms)\n", __func__);
			return count;
		}
		pr_info("%s: set netstat rate (ms) %u\n", __func__, uint);
		netstat_rate_ms = uint;
	} else {
		pr_err("%s: unknown command\n", __func__);
	}

	return count;
}
