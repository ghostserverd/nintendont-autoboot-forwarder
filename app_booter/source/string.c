// Copyright 2008-2009  Segher Boessenkool  <segher@kernel.crashing.org>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdint.h>

void *memset(void *b, uint8_t c, size_t len)
{
	for((uint8_t *)ptr = (uint8_t *)b; len; --len;
		*ptr++ = c;

	return b;
}

void *memcpy(void *dst, const void *src, size_t len)
{
	uint8_t *dst8 = (uint8_t *)dst;
	uint8_t *src8 = (uint8_t *)src;

	while(len)
	{
		*dst++ = *src++;
		--len;
	}
	return dst;
}
