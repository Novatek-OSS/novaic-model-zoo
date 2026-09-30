#ifndef _UTIL_IMG_H_
#define _UTIL_IMG_H_

/********************************************************************
    INCLUDE FILES
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "jpeglib.h"
#include <setjmp.h>
#include "util_common.h"

/********************************************************************
    MACRO CONSTANT DEFINITIONS
********************************************************************/
#define PATH_LEN                256         ///< maximal length of file path

/********************************************************************
    TYPE DEFINITIONS
********************************************************************/
/**
    @name image format type
*/
typedef enum _NOVA_IMG_FMT {
	NOVA_IMG_JPG = 0,                       ///< JPEG image format
} NOVA_IMG_FMT;                             ///< image format enumeration

/**
    @name JPEG error manager
*/
struct my_error_mgr {
	struct jpeg_error_mgr   pub;            ///< base JPEG error manager (must be first)
	jmp_buf                 setjmp_buffer;  ///< setjmp buffer for error recovery
};

/**
    @name pointer type of my_error_mgr
*/
typedef struct my_error_mgr *my_error_ptr;  ///< pointer to JPEG error manager

/**
    @name JPEG decoder context
*/
typedef struct _NOVA_JPEG {
	struct jpeg_decompress_struct *j_info;  ///< pointer to libjpeg decompress info structure
	struct my_error_mgr j_err;              ///< JPEG error manager instance
	FILE                *j_file;            ///< pointer to the opened JPEG file
} NOVA_JPEG;                                ///< JPEG decoder context structure

/**
    @name image info
*/
typedef struct _NOVA_IMG {
	NOVA_IMG_FMT        fmt;                ///< image format type
	CHAR                path[PATH_LEN];     ///< file path of the image
	VOID                *info;              ///< pointer to format-specific decoder context
} NOVA_IMG;                                 ///< image information structure

/********************************************************************
    FUNCTION DECLARATIONS
********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

HD_RESULT nova_img_init(NOVA_IMG *img_info);
HD_RESULT nova_img_uninit(NOVA_IMG *img_info);

int nova_jpeg_init(NOVA_JPEG *jpeg, const char *jpeg_name);
int nova_jpeg_uninit(NOVA_JPEG *jpeg);
int nova_jpeg_get_width(NOVA_JPEG *jpeg);
int nova_jpeg_get_height(NOVA_JPEG *jpeg);
int nova_jpeg_get_colorspace(NOVA_JPEG *jpeg);
int nova_jpeg_get_components(NOVA_JPEG *jpeg);
int nova_jpeg_get_tempbuf_size(NOVA_JPEG *jpeg, HD_VIDEO_PXLFMT dst_fmt);
int nova_jpeg_readimg(NOVA_JPEG *jpeg, HD_VIDEO_PXLFMT dst_fmt, AI_BUF *img_buf, MEM_PARM *img_mem, MEM_PARM *temp_mem);

#ifdef __cplusplus
}
#endif

#endif  /* _UTIL_IMG_H_ */
