# pragma once

#include <stdlib.h>

#include <gccore.h>

#define mdelay(x)
#define malloca(x, y) aligned_alloc(y, x)
#define ALIGNED(v) ATTRIBUTE_ALIGN(v)

