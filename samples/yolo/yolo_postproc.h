/********************************************************************
    INCLUDE FILES
********************************************************************/
#include "util_model.h"

/********************************************************************
    MACRO CONSTANT DEFINITIONS
********************************************************************/
#define PREDICT_NUMBERS         20000//20000  4000
#define FINAL_PRED_NUMBERS      10000//10000
#define INPUT_W                 640
#define INPUT_H                 640
#define OUTPUT_BBOX_NUM         25200
#define NUM_CLASS               80
#define IsPadding               1
#define MAX_DET                 300
#define ANCHOR_NUM              8400
#define FAST_SIGMOID_EN         0
#define VAL_MODE                0

#define YOLO_L0_SIZE   80
#define YOLO_L1_SIZE   40
#define YOLO_L2_SIZE   20
#define YOLO_L0_COUNT  (YOLO_L0_SIZE * YOLO_L0_SIZE)
#define YOLO_L1_COUNT  (YOLO_L1_SIZE * YOLO_L1_SIZE)
#define YOLO_L2_COUNT  (YOLO_L2_SIZE * YOLO_L2_SIZE)

#define VAL_NMS_TH_V5_V7_v11_v12_v13   0.6
#define VAL_NMS_TH_V8_X_v10_26         0.7
#define VAL_CONF_TH                    0.001
#define INFER_NMS_TH                   0.45
#define INFER_CONF_TH                  0.25

/********************************************************************
    TYPE DEFINITIONS
********************************************************************/
typedef enum _YOLO_VER_LIST {
	YOLOV5       =  5,
	YOLOV7       =  7,
	YOLOV8       =  8,
	YOLOX        =  9,
	YOLOV10      = 10,
	YOLOV11      = 11,
	YOLOV12      = 12,
	YOLOV13      = 13,
	YOLO26       = 26,
	ENUM_DUMMY4WORD(YOLO_VER_LIST),
	YOLO_VER_MAX,
	YOLO_VER_INVALID = -1
} YOLO_VER_LIST;

typedef struct _Detection {
	FLOAT xc, yc, w, h;
	FLOAT score;
	INT32 class_id;
} Detection;

typedef struct _GFX_REGION {
	INT32 offset_w;
	INT32 offset_h;
	INT32 image_w;
	INT32 image_h;
} GFX_REGION;

typedef struct _YOLO_Anchor {
	FLOAT width;
	FLOAT height;
} YOLO_Anchor;

typedef struct _ScoreIndex {
	FLOAT score;
	INT32 index;
} ScoreIndex;

/********************************************************************
    EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
HD_RESULT nova_yolo_postproc(NOVA_MODEL *model_info, NOVA_INPUT *in_info, UINT32 yolo_ver);
