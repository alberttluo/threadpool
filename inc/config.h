#ifndef CONFIG_H
#define CONFIG_H

#include <assert.h>

#define DEBUG (1)

#if defined(DEBUG) && DEBUG > 0
 #define DEBUG_PRINT(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)

 #define DEBUG_ASSERT(cond) assert(cond)
#else
 #define DEBUG_PRINT(fmt, args...)
#endif


#endif
