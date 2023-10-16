/* Copyright 2011 Dimok
   This code is licensed to you under the terms of the GNU GPL, version 2;
   see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt  */
#include <gccore.h>
#include <stdio.h>
#include <stdarg.h>
#include "string.h"

#include "dolloader.h"
#include "sync.h"

#define EXECUTABLE_MEM_ADDR		0x92000000
#define SYSTEM_ARGV				((struct __argv *)0x93300800)

void _main(void)
{
	void *exeBuffer = (void *)EXECUTABLE_MEM_ADDR;
	u32 exeEntryPointAddress = load_dol_image(exeBuffer);
	entrypoint exeEntryPoint = (entrypoint)exeEntryPointAddress;

	if(!exeEntryPoint)
		return;

	if(SYSTEM_ARGV->argvMagic == ARGV_MAGIC)
	{
		exeBuffer = (void *) (exeEntryPointAddress + 8);
		memcpy(exeBuffer, SYSTEM_ARGV, sizeof(struct __argv));
		sync_before_exec(exeBuffer, sizeof(struct __argv));
	}

	exeEntryPoint();
}
