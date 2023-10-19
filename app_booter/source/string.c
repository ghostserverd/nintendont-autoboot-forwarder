// Copyright 2008-2009  Segher Boessenkool  <segher@kernel.crashing.org>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <gccore.h>

void *memset(u8 *b, u8 c, u32 len)
{
	while(len)
	{
		*b++ = c;
		--len;
	}

	return b;
}

void *memcpy(u8 *dst, const u8 *src, u32 len)
{
	while(len)
	{
		*dst++ = *src++;
		--len;
	}
	return dst;
}
