/*
 * GK20A Debug functionality
 *
 * Copyright (C) 2011-2015 NVIDIA CORPORATION.  All rights reserved.
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

#ifndef _DEBUG_GK20A_H_
#define _DEBUG_GK20A_H_


struct gk20a_debug_output {
	void (*fn)(void *ctx, const char *str, size_t len);
	void *ctx;
	char buf[256];
};

void gk20a_debug_output(struct gk20a_debug_output *o,
					const char *fmt, ...);
void gk20a_debug_dump(struct platform_device *pdev);
int gk20a_gr_debug_dump(struct platform_device *pdev);
void gk20a_debug_init(struct platform_device *pdev);

#endif
