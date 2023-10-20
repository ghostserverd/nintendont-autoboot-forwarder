/* Copyright 2011 Dimok
   This code is licensed to you under the terms of the GNU GPL, version 2;
   see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt  */
#include <gccore.h>
#include <stdio.h>
#include <stdarg.h>
#include "../../source/common.h"
#include "string.h"

#include "sync.h"

typedef void (*entrypoint) (void);

typedef struct {
	u32 pos[7 + 11];
	void *start[7 + 11];
	u32 size[7 + 11];
	void *bss_start;
	u32 bss_size;
	entrypoint entry_point;
} dolheader;

#define DOLHEADER	((const dolheader *)EXECUTE_ADDR)

void _main(void)
{
	for(u32 i = 0; i < 7 + 11; ++i)
		if(DOLHEADER->size[i])
		{
			memcpy(DOLHEADER->start[i], EXECUTE_ADDR + DOLHEADER->pos[i], DOLHEADER->size[i]);
			sync_before_exec(DOLHEADER->start[i], DOLHEADER->size[i]);
		}

	memcpy((u8 *)(((u32)DOLHEADER->entry_point) + 8), (const u8 *)ARGS_ADDR, sizeof(struct __argv));
	sync_before_exec((u8 *)(((u32)DOLHEADER->entry_point) + 8), sizeof(struct __argv));
	DOLHEADER->entry_point();
}
