/*
 * arch/arm64/mach-tegra/include/mach/nvdumper.h
 *
 * Copyright (c) 2011-2014, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef __MACH_TEGRA_NVDUMPER_H
#define __MACH_TEGRA_NVDUMPER_H

struct mode_regs_t {
	unsigned long sp_svc;
	unsigned long lr_svc;
	unsigned long spsr_svc;
	/* 0x17 */
	unsigned long sp_abt;
	unsigned long lr_abt;
	unsigned long spsr_abt;
	/* 0x1b */
	unsigned long sp_und;
	unsigned long lr_und;
	unsigned long spsr_und;
	/* 0x12 */
	unsigned long sp_irq;
	unsigned long lr_irq;
	unsigned long spsr_irq;
	/* 0x11 */
	unsigned long r8_fiq;
	unsigned long r9_fiq;
	unsigned long r10_fiq;
	unsigned long r11_fiq;
	unsigned long r12_fiq;
	unsigned long sp_fiq;
	unsigned long lr_fiq;
	unsigned long spsr_fiq;
	/* 0x1f: sys/usr */
	unsigned long r8_usr;
	unsigned long r9_usr;
	unsigned long r10_usr;
	unsigned long r11_usr;
	unsigned long r12_usr;
	unsigned long sp_usr;
	unsigned long lr_usr;
};

struct cp15_regs_t {
	unsigned int SCTLR;
	unsigned int TTBR0;
	unsigned int TTBR1;
	unsigned int TTBCR;
	unsigned int DACR;
	unsigned int DFSR;
	unsigned int IFSR;
	unsigned int ADFSR;
	unsigned int AIFSR;
	unsigned int DFAR;
	unsigned int IFAR;
	unsigned int PAR;
	unsigned int TLPLDR;
	unsigned int PRRR;
	unsigned int NRRR;
	unsigned int CONTEXTIDR;
	unsigned int TPIDRURW;
	unsigned int TPIDRURO;
	unsigned int TPIDRPRW;
	unsigned int CFGBASEADDREG;
};

struct nvdumper_cpu_data_t {
	bool is_online;
	struct pt_regs pt_regs;
	struct mode_regs_t mode_regs;
	struct cp15_regs_t cp15_regs;
	struct task_struct *current_task;
};


int nvdumper_regdump_init(void);
void nvdumper_regdump_exit(void);
void nvdumper_crash_setup_regs(void);
void nvdumper_print_data(void);

#endif
