// Copyright 2008-2009  Segher Boessenkool  <segher@kernel.crashing.org>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <gccore.h>

void *memset(void *b, u8 c, u32 len)
{
	for(u8 *ptr = (u8 *)b; len; --len)
		*ptr++ = c;

	return b;
}

void *memcpy(void *dst, const void *src, u32 len)
{
	u8 *dst8 = (u8 *)dst;
	u8 *src8 = (u8 *)src;

	while(len)
	{
		*dst8++ = *src8++;
		--len;
	}
	return dst;
}
