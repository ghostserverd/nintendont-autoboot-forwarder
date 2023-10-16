# pragma once

#include <ogc/ipc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef int64_t s64;
typedef uint64_t u64;
typedef int32_t s32;
typedef uint32_t u32;
typedef int16_t s16;
typedef uint16_t u16;
typedef int8_t s8;
typedef uint8_t u8;

#define mdelay(x)
#define malloca(x, y) aligned_alloc(y, x)
#define ALIGNED(x) __attribute__((__aligned__(x)))
