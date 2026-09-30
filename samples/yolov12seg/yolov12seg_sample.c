/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "util_img.h"
#include "util_model.h"
#include "yolov12seg_postproc.h"

/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
#define MODEL_PATH          "nvt_model.bin"
#define LABEL_PATH          "coco_labels.txt"
#define INPUT_PATH          "bus.jpg"
#define MASK_OUTPUT_PATH    "mask.bin"

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	NOVA_MODEL model_info = {
		.model_path = MODEL_PATH,
		.label_path = LABEL_PATH
	};
	NOVA_IMG img_info = {
		.fmt = NOVA_IMG_JPG,
		.path = INPUT_PATH
	};
	NOVA_INPUT input_info = {
		.letterbox_mode = NOVA_LETTERBOX_CENTER
	};

	if (argc > 2) {
		printf("Usage: %s <JPEG file path>\n", argv[0]);
		return -1;
	} else {
		if (argc == 2) {
			snprintf(img_info.path, PATH_LEN, argv[1]);
		}
	}

	nova_model_init(&model_info);
	nova_img_init(&img_info);
	nova_input_init(&input_info, &model_info, &img_info);

	nova_model_inference(&model_info, &input_info);

	nova_yolov12seg_postproc(&model_info, &input_info, MASK_OUTPUT_PATH);

	nova_input_uninit(&input_info);
	nova_img_uninit(&img_info);
	nova_model_uninit(&model_info);

	return 0;
}
