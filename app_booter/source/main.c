/* Copyright 2011 Dimok
   This code is licensed to you under the terms of the GNU GPL, version 2;
   see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt  */
#include <gccore.h>
#include <stdio.h>
#include <stdarg.h>
#include "string.h"

#include "sync.h"

typedef void (*entrypoint) (void);

typedef struct {
	u32 text_pos[7];
	u32 data_pos[11];
	void *text_start[7];
	void *data_start[11];
	u32 text_size[7];
	u32 data_size[11];
	void *bss_start;
	u32 bss_size;
	entrypoint entry_point;
} dolheader;

#define DOLHEADER				((const dolheader *)0x92000000)
#define SYSTEM_ARGV				((u8 *)0x93300800)

void _main(void)
{
	for(u32 i = 0; i < 11; i++)
	{
		if(i < 7 && DOLHEADER->text_size[i])
		{
			memcpy(DOLHEADER->text_start[i], ((const void *)DOLHEADER) + DOLHEADER->text_pos[i], DOLHEADER->text_size[i]);
			sync_before_exec(DOLHEADER->text_start[i], DOLHEADER->text_size[i]);
		}

		if(DOLHEADER->data_size[i])
		{
			memcpy(DOLHEADER->data_start[i], ((const void *)DOLHEADER) + DOLHEADER->data_pos[i], DOLHEADER->data_size[i]);
			sync_before_exec(DOLHEADER->data_start[i], DOLHEADER->data_size[i]);
		}
	}

	u8 *exeBuffer = (u8 *)(((u32)DOLHEADER->entry_point) + 8);
	memcpy(exeBuffer, SYSTEM_ARGV, sizeof(struct __argv));
	sync_before_exec(exeBuffer, sizeof(struct __argv));

	DOLHEADER->entry_point();
}
