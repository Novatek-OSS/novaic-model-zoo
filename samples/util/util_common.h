#ifndef _UTIL_COMMON_H_
#define _UTIL_COMMON_H_

/********************************************************************
    INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "vendor_ai.h"

/********************************************************************
    MACRO FUNCTION DEFINITIONS
********************************************************************/
#undef MAX
#undef MIN
#undef CLAMP
#undef DBGD
#undef DBGF
#undef DBGLF
#undef DBGLH

#define MAX(a, b)               ((a) > (b) ? (a) : (b))                             ///< Return the maximum of two values
#define MIN(a, b)               ((a) < (b) ? (a) : (b))                             ///< Return the minimum of two values
#define CLAMP(x, min, max)      MIN(max, MAX(x, min))                               ///< Clamp x to the range [min, max]
#define DBGD(x)                 printf("%s=%d\r\n", #x, (int)(x))                   ///< Show a debug string of variable name and integer value
#define DBGF(x)                 printf("%s=%f\r\n", #x, (float)(x))                 ///< Show a debug string of variable name and float value
#define DBGLF(x)                printf("%s=%.8lf\r\n", #x, (double)(x))             ///< Show a debug string of variable name and double value
#define DBGLH(x)                printf("%s=0x%08lX\r\n", #x, (unsigned long)(x))    ///< Show a debug string of variable name and 64-bit hexadecimal value

/********************************************************************
    TYPE DEFINITIONS
********************************************************************/
#ifndef UINTPTR
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
typedef UINT32          UINTPTR;            ///< for 32-bit systems
#endif
#endif

#if defined(CHIP_CNN30_IPC) || defined(CHIP_CNN30_XVR)
#define AI_BUF          VENDOR_AI3_BUF      ///< AI buffer of AI3 SDK
#define AI_NET_INFO     VENDOR_AI3_NET_INFO ///< AI network info of AI3 SDK
#else
#define AI_BUF          VENDOR_AI_BUF       ///< AI buffer of AI2 SDK
#define AI_NET_INFO     VENDOR_AI_NET_INFO  ///< AI network info of AI2 SDK
#endif

/**
    @name memory block
*/
typedef struct _MEM_PARM {
	UINTPTR             pa;                 ///< physical address of memory block
	UINTPTR             va;                 ///< virtual address of memory block
	UINT32              size;               ///< size of memory block in bytes
	UINT32              blk;                ///< memory block handle
} MEM_PARM;

#endif  /* _UTIL_COMMON_H_ */
