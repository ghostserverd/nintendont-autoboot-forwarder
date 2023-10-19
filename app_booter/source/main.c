/* Copyright 2011 Dimok
   This code is licensed to you under the terms of the GNU GPL, version 2;
   see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt  */
#include <gccore.h>
#include <stdio.h>
#include <stdarg.h>
#include "string.h"

#include "dolloader.h"
#include "sync.h"

#define EXECUTABLE_MEM_ADDR		((u8 *)0x92000000)
#define SYSTEM_ARGV				((u8 *)0x93300800)

void _main(void)
{
	entrypoint exeEntryPoint = load_dol_image(EXECUTABLE_MEM_ADDR);
	if(!exeEntryPoint)
		return;

	u8 *exeBuffer = (u8 *)(((u32)exeEntryPoint) + 8);
	memcpy(exeBuffer, SYSTEM_ARGV, sizeof(struct __argv));
	sync_before_exec(exeBuffer, sizeof(struct __argv));

	exeEntryPoint();
}
