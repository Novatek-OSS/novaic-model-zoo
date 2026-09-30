#ifndef _UTIL_MODEL_H_
#define _UTIL_MODEL_H_

/********************************************************************
    INCLUDE FILES
********************************************************************/
#include "hdal.h"
#include "vendor_ai.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "util_img.h"
#if defined(CHIP_CNN25_XVR)
#include "vendor_common.h"
#endif
#if defined(_NVT_NVR_SDK_)
#include <comm/nvtmem_if.h>
#include <sys/ioctl.h>
#endif

/********************************************************************
    MACRO CONSTANT DEFINITIONS
********************************************************************/
#define NOVA_PRINT_AI_VER       0       ///< print AI SDK version
#define NOVA_PRINT_MODEL_INFO   0       ///< print model information

#define PATH_LEN                256     ///< maximal length of file path
#define LABEL_LEN               256     ///< maximal length of class label
#define MAX_CLASS_NUMBER        1000    ///< maximum number of classes
#define TOP_N                   5       ///< number of top scoring results

/********************************************************************
    MACRO FUNCTION DEFINITIONS
********************************************************************/
#define SWAP(a, b, t)           (t) = (a); (a) = (b); (b) = (t) ///< swap two values using a temporary variable

/********************************************************************
    TYPE DEFINITIONS
********************************************************************/
/**
    @name tensor shape
*/
typedef struct _AI_NET_SHAPE {
	INT32               num;            ///< Caffe blob: batch size
	INT32               channels;       ///< Caffe blob: number of channels
	INT32               height;         ///< Caffe blob: height
	INT32               width;          ///< Caffe blob: width
	INT32               lineofs;        ///< lineoffset: distance between two lines (rows); unit: byte
} AI_NET_SHAPE;

/**
    @name output class
*/
typedef struct _AI_NET_OUTPUT_CLASS {
	INT32               no;             ///< class number
	FLOAT               score;          ///< class score
} AI_NET_OUTPUT_CLASS;

/**
    @name parameters of accuracy calculation
*/
typedef struct _AI_NET_ACCURACY_PARM {
	UINTPTR                 in_addr;    ///< [in]  address of input data
	AI_NET_OUTPUT_CLASS    *classes;    ///< [out] top scoring class list
	AI_NET_SHAPE            shape;      ///< [in]  input/output data dimensions (height/width/lineofs is not used)
	INT32                   top_n;      ///< [in/out] number of top scoring classes
	INT32                  *class_idx;  ///< [in]  address of class index buffer
} AI_NET_ACCURACY_PARM;

/**
    @name AI network processing information
*/
typedef struct _NET_PROC {
	CHAR                model_filename[PATH_LEN];   ///< model file name path
	UINT32              binsize;                    ///< size of model binary
	UINT32              proc_id;                    ///< process id of AI engine
	MEM_PARM            io_mem;                     ///< memory parameter for input/output buffer
	MEM_PARM            proc_mem;                   ///< memory parameter for processing buffer
	MEM_PARM            intl_mem;                   ///< memory parameter for internal buffer
	MEM_PARM           *in_mem;                     ///< pointer to input memory
	MEM_PARM           *out_mem;                    ///< pointer to output memory
	AI_BUF             *in_buf;                     ///< pointer to input AI buffer
	AI_BUF             *out_buf;                    ///< pointer to output AI buffer
	AI_NET_INFO         net_info;                   ///< AI network information
	FLOAT             **float_out;                  ///< pointer to float output data array
#if defined(CHIP_CNN25_IPC) || defined(CHIP_CNN25_XVR)
	UINT32             *in_path_list;               ///< pointer to input path id list
	UINT32             *out_path_list;              ///< pointer to output path id list
#endif
} NET_PROC;

/**
    @name model information
*/
typedef struct _NOVA_MODEL {
	CHAR                model_path[PATH_LEN];                       ///< file path of model binary
	CHAR                label_path[PATH_LEN];                       ///< file path of class label file
	CHAR                class_labels[MAX_CLASS_NUMBER][LABEL_LEN];  ///< array of class label strings
	NET_PROC            proc_info;                                  ///< AI network processing information
	BOOL                prof_inference;                             ///< enable inference profiling flag
	BOOL                prof_postproc;                              ///< enable post-processing profiling flag
} NOVA_MODEL;

/**
    @name letterbox padding mode
*/
typedef enum {
	NOVA_LETTERBOX_NONE = 0,            ///< no letterbox padding
	NOVA_LETTERBOX_CENTER,              ///< used by YOLO series
	NOVA_LETTERBOX_TOP_LEFT,            ///< used by YOLOX
} NOVA_LETTERBOX_MODE;                  ///< letterbox padding mode

/**
    @name letterbox information
*/
typedef struct _NOVA_LETTERBOX {
	FLOAT               scale;          ///< scaling factor (minimum ratio of model input size to original image size)
	UINT32              offset_w;       ///< number of left padding pixels
	UINT32              offset_h;       ///< number of top padding pixels
	UINT32              orig_w;         ///< original image width
	UINT32              orig_h;         ///< original image height
} NOVA_LETTERBOX;

/**
    @name input information
*/
typedef struct _NOVA_INPUT {
	AI_BUF             *img_buf;        ///< pointer to input image AI buffer
	MEM_PARM            img_mem;        ///< memory parameter for image buffer
	MEM_PARM            temp_mem;       ///< memory parameter for temporary buffer
	MEM_PARM            scale_img_mem;  ///< memory parameter for scaled image buffer
	NOVA_LETTERBOX_MODE letterbox_mode; ///< letterbox padding mode selection
	NOVA_LETTERBOX      letterbox;      ///< letterbox padding parameters
} NOVA_INPUT;

/**
    @name detection bounding-box information
*/
typedef struct _NOVA_DET_BBOX {
	FLOAT               xmin;           ///< x coordinate of the top-left corner
	FLOAT               ymin;           ///< y coordinate of the top-left corner
	FLOAT               xmax;           ///< x coordinate of the bottom-right corner
	FLOAT               ymax;           ///< y coordinate of the bottom-right corner
	FLOAT               score;          ///< detection confidence score
	INT32               cls_id;         ///< detected class id
} NOVA_DET_BBOX;

/********************************************************************
    EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

HD_RESULT nova_model_init(NOVA_MODEL *model_info);
HD_RESULT nova_model_uninit(NOVA_MODEL *model_info);
HD_RESULT nova_model_inference(NOVA_MODEL *model_info, NOVA_INPUT *in_info);

HD_RESULT nova_set_image_buf(MEM_PARM *img_mem, HD_DIM img_dim, HD_VIDEO_PXLFMT img_fmt, AI_BUF *img_buf);
HD_RESULT nova_mem_alloc(MEM_PARM *mem_parm, CHAR *name, UINT32 size);
HD_RESULT nova_mem_free(MEM_PARM *mem_parm);
HD_RESULT ai_net_accuracy_process(AI_NET_ACCURACY_PARM *p_parm);

HD_RESULT nova_input_init(NOVA_INPUT *in_info, NOVA_MODEL *model_info, NOVA_IMG *img_info);
HD_RESULT nova_input_uninit(NOVA_INPUT *in_info);
HD_RESULT nova_output_print_roi(NOVA_DET_BBOX *bbox, UINT32 bbox_ofs, INT32 obj_num, NOVA_MODEL *model_info);

#ifdef __cplusplus
}
#endif
#endif  /* _UTIL_MODEL_H_ */
