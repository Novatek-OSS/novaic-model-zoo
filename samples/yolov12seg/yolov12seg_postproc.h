#ifndef _YOLOV12SEG_POSTPROC_H_
#define _YOLOV12SEG_POSTPROC_H_

/********************************************************************
    INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "util_model.h"

/********************************************************************
    MACRO CONSTANT DEFINITIONS
********************************************************************/
#define MASK_COEFF_NUM          (32)                            ///< number of mask coefficients
#define NET_INPUT_WIDTH         (640)                           ///< network input width in pixels
#define NET_INPUT_HEIGHT        (640)                           ///< network input height in pixels
#define PROTO_HEIGHT            (NET_INPUT_WIDTH/4)             ///< prototype map height (1/4 of input)
#define PROTO_WIDTH             (NET_INPUT_HEIGHT/4)            ///< prototype map width (1/4 of input)
#define MAU_MAT_W_H             (PROTO_HEIGHT * PROTO_WIDTH)    ///< prototype map total elements (W x H)
#define ANCHOR_NUM              (8400)                          ///< number of anchors per sample
#define PROTO_CHANNELS          (32)                            ///< number of prototype channels
#define NUM_CLASSES             (80)                            ///< number of object classes
#define MASK_THRESHOLD          (128)                           ///< mask binarization threshold (0 - 255 range)
#define NMS_THRESHOLD           (0.5f)                          ///< IoU threshold for NMS
#define CONF_THRESHOLD          (0.6f)                          ///< minimum confidence threshold for detection
#define MAX_DETECT_NUM          (300)                           ///< maximum number of detected objects

/********************************************************************
    TYPE DEFINITION
********************************************************************/
typedef struct _YOLO_BBOX {
	// NOVA_DET_BBOX fields
	FLOAT               xmin;                           ///< bounding box left coordinate
	FLOAT               ymin;                           ///< bounding box top coordinate
	FLOAT               xmax;                           ///< bounding box right coordinate
	FLOAT               ymax;                           ///< bounding box bottom coordinate
	FLOAT               score;                          ///< detection confidence score
	INT32               cls_id;                         ///< predicted class ID

	// Extension fields
	FLOAT               x;                              ///< center x coordinate
	FLOAT               y;                              ///< center y coordinate
	FLOAT               w;                              ///< bounding box width
	FLOAT               h;                              ///< bounding box height
	INT32               anchor_ind;                     ///< anchor index used for this detection
	FLOAT               mask_coeff[MASK_COEFF_NUM];     ///< mask coefficient vector
} YOLO_BBOX;

/********************************************************************
    EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
UINT32 nova_yolov12seg_postproc(NOVA_MODEL *model_info, NOVA_INPUT *in_info, const CHAR *out_filename);

#endif  /* _YOLOV12SEG_POSTPROC_H_ */
