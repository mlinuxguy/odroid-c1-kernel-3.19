/*
 * arch/arm/mach-meson8b/board-meson8b-odroidc.c
 *
 * Copyright (C) 2011-2013 Amlogic, Inc.
 * Copyright (C) 2014 Hardkernel,. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/memblock.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/amlogic/of_lm.h>
#include <linux/reboot.h>
#include <plat/irq.h>
#include <plat/lm.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/system_info.h>
#include <mach/io.h>
#ifdef CONFIG_SMP
#include <mach/smp.h>
#endif
#include <linux/syscore_ops.h>
#include <mach/am_regs.h>

#include <linux/amlogic/vmapi.h>

extern void meson_common_restart(enum reboot_mode mode,const char *cmd);
static void meson_map_board_io(void);
extern unsigned long long aml_reserved_start;
extern unsigned long long aml_reserved_end;
extern void __init meson_timer_init(void);

static __init void meson8b_reserve(void)
{
        /*
         * Reserved memory for hotplug:
         *   start address: PHYS_OFFSET, size 0x4000,
         *   toprevent other getting logical address 0xc0000000 and
         *   flushing valid data on "zero address"
         */
        memblock_reserve(PHYS_OFFSET,__pa(swapper_pg_dir) - PHYS_OFFSET);
}

__initdata struct map_desc meson_board_io_desc[1];

static __init void meson_map_board_io(void)
{
        meson_board_io_desc[0].virtual = PAGE_ALIGN(__phys_to_virt(aml_reserved_start));
        meson_board_io_desc[0].pfn = __phys_to_pfn(aml_reserved_start);
        meson_board_io_desc[0].length = aml_reserved_end
                - aml_reserved_start + 1;
/* fix        meson_board_io_desc[0].type = MT_MEMORY_NONCACHED; */
        meson_board_io_desc[0].type = MT_MEMORY_RWX_NONCACHED;
        iotable_init(meson_board_io_desc,ARRAY_SIZE(meson_board_io_desc));
}

static void __init meson_map_io(void)
{
        meson_map_default_io();
        meson_map_board_io();
}

static struct of_device_id m8_of_platform_bus_ids[] = {
        {
                .compatible = "simple-bus",
        },
        {},
};
static struct of_device_id m8_of_lm_bus_ids[] = {
        {
                .compatible = "logicmodule-bus",
        },
        {},
};

static __init void meson_init_machine_devicetree(void)
{
        struct device *parent;

        parent = get_device(&platform_bus);

        of_platform_populate(NULL,m8_of_platform_bus_ids,NULL,parent);
#ifdef CONFIG_OF_LM
        of_lm_populate(NULL,m8_of_lm_bus_ids,NULL,NULL);
#endif
}

int meson_cache_of_init(void);

static __init void meson_init_early(void)
{
        int rev;

        meson_cpu_version_init();
        /*
         * Mali or some USB devices allocate their coherent buffers from atomic
         * context. Increase size of atomic coherent pool to make sure such
         * the allocations won't fail.
         */
        init_dma_coherent_pool_size(SZ_4M);

        rev = get_meson_cpu_version(MESON_CPU_VERSION_LVL_MAJOR);
        rev <<= 24;
        system_serial_high = rev;
        rev = get_meson_cpu_version(MESON_CPU_VERSION_LVL_MINOR);
        system_rev = rev;
}

static void __init meson_init_irq(void)
{
        meson_init_gic_irq();
}

static const char *m8_common_board_compat[] __initdata = {
        "AMLOGIC,8726_M8B",
        NULL,
};

DT_MACHINE_START(AML8726_M8, "ODROIDC")
        .reserve        = meson8b_reserve,
        .smp            = smp_ops(meson_smp_ops),
        .map_io         = meson_map_io,
        .init_early     = meson_init_early,
        .init_irq       = meson_init_irq,
        .init_time      = meson_timer_init,
        .init_machine   = meson_init_machine_devicetree,
        .restart        = meson_common_restart,
        .dt_compat      = m8_common_board_compat,
MACHINE_END
