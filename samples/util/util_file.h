#ifndef _UTIL_FILE_H_
#define _UTIL_FILE_H_

/********************************************************************
    INCLUDE FILES
********************************************************************/
#include "hd_type.h"

/********************************************************************
    EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
HD_RESULT nova_file_readbin (UINTPTR addr, UINT32 size, const CHAR *filename);
HD_RESULT nova_file_writebin(UINTPTR addr, UINT32 size, const CHAR *filename);
INT32     nova_file_loadbin (UINTPTR addr, const CHAR *filename);
HD_RESULT nova_file_readtxt (UINTPTR addr, UINT32 line_len, UINT32 line_num, const CHAR *filename);

#endif  /* _UTIL_FILE_H_ */
