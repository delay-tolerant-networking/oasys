#include "../debug/Debug.h"

STATIC_ASSERT(1 == 1, Should_Pass, __LINE__);
STATIC_ASSERT(false,  Should_Die,  __LINE__);
