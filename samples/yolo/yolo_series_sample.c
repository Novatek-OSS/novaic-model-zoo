/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <string.h>
#include "util_img.h"
#include "util_model.h"
#include "yolo_postproc.h"

/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
#define MODEL_PATH          "nvt_model.bin"
#define LABEL_PATH          "coco_labels.txt"
#define INPUT_PATH          "bus.jpg"

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
YOLO_VER_LIST nova_yolo_set_version(const CHAR *version_str)
{
	YOLO_VER_LIST ver = YOLO_VER_INVALID;

	if (!version_str) {
		return YOLO_VER_INVALID;
	}

	if (strcmp(version_str, "yolov5") == 0) {
		ver = YOLOV5;
	} else if (strcmp(version_str, "yolov7")  == 0) {
		ver = YOLOV7;
	} else if (strcmp(version_str, "yolov8")  == 0) {
		ver = YOLOV8;
	} else if (strcmp(version_str, "yolox")   == 0) {
		ver = YOLOX;
	} else if (strcmp(version_str, "yolov10") == 0) {
		ver = YOLOV10;
	} else if (strcmp(version_str, "yolov11") == 0) {
		ver = YOLOV11;
	} else if (strcmp(version_str, "yolov12") == 0) {
		ver = YOLOV12;
	} else if (strcmp(version_str, "yolov13") == 0) {
		ver = YOLOV13;
	} else if (strcmp(version_str, "yolo26")  == 0) {
		ver = YOLO26;
	}

	if (ver == YOLO_VER_INVALID) {
		printf("yolo model version \"%s\" not supported, please check\n",
				version_str);
		return YOLO_VER_INVALID;
	}

	return ver;
}

int main(int argc, char *argv[])
{
	CHAR yolo_ver_str[32];
	UINT8 idx = 1;
	NOVA_MODEL model_info = {
		.model_path = MODEL_PATH,
		.label_path = LABEL_PATH,
		.prof_inference = FALSE,  // Whether to print inference time
		.prof_postproc = FALSE    // Whether to print postprocess time
	};
	NOVA_IMG img_info = {
		.fmt = NOVA_IMG_JPG,
		.path = INPUT_PATH
	};
	NOVA_INPUT input_info = {0};

	if (argc < 3) {
		printf("Usage: %s <jpg_name> <yolo_version>\n", argv[0]);
		return -1;
	}

	if (argc > idx) {
		snprintf(img_info.path, sizeof(img_info.path), "%s", argv[idx++]);
	}
	if (argc > idx) {
		snprintf(yolo_ver_str, sizeof(yolo_ver_str), "%s", argv[idx++]);
	}

	YOLO_VER_LIST yolo_ver = nova_yolo_set_version(yolo_ver_str);

	if (yolo_ver == YOLOX) {
		input_info.letterbox_mode = NOVA_LETTERBOX_TOP_LEFT;
	} else {
		input_info.letterbox_mode = NOVA_LETTERBOX_CENTER;
	}

	nova_model_init(&model_info);
	nova_img_init(&img_info);
	nova_input_init(&input_info, &model_info, &img_info);

	nova_model_inference(&model_info, &input_info);
	nova_yolo_postproc(&model_info, &input_info, yolo_ver);

	nova_input_uninit(&input_info);
	nova_img_uninit(&img_info);
	nova_model_uninit(&model_info);

	return 0;
}
