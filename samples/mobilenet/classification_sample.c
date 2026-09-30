/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util_img.h"
#include "util_model.h"

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/
static VOID nova_cls_model_postproc(NOVA_MODEL *model_info)
{
	FLOAT *p_out = model_info->proc_info.float_out[0];
	AI_BUF *p_outbuf = &model_info->proc_info.out_buf[0];

	INT32 *p_idx = (INT32 *)malloc(p_outbuf->channel * sizeof(INT32));
	AI_NET_OUTPUT_CLASS *p_classes = (AI_NET_OUTPUT_CLASS *)malloc(p_outbuf->batch_num * TOP_N * sizeof(AI_NET_OUTPUT_CLASS));
	AI_NET_SHAPE shape = {p_outbuf->batch_num, p_outbuf->channel, 1, 1, 1};
	AI_NET_ACCURACY_PARM parm = {0};
	parm.in_addr   = (UINTPTR)p_out;
	parm.classes   = p_classes;
	parm.shape     = shape;
	parm.top_n     = TOP_N;
	parm.class_idx = p_idx;
	ai_net_accuracy_process(&parm);

	printf("\nClassification Results:\r\n");
	printf("+------+-------------+-------+--------------------------------------------+\n");
	printf("|  ID  |    Score    |  No.  |                   Categories               |\n");
	printf("+------+-------------+-------+--------------------------------------------+\n");
	for (INT32 i = 0; i < parm.top_n; i++) {
		CHAR cate[43];
		strncpy(cate, model_info->class_labels[parm.classes[i].no], 42);
		cate[42] = '\0';
		printf("|  %2d  |  %.7f  |  %3d  | %-42s |\r\n",
				i + 1,
				parm.classes[i].score,
				parm.classes[i].no,
				cate);
	}
	printf("+------+-------------+-------+--------------------------------------------+\n");
	free(p_idx);
	free(p_classes);

	sleep(1); // avoid interleaved console output with other threads
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	NOVA_MODEL model_info = {
		.model_path = "nvt_model.bin",
		.label_path = "imagenet_labels.txt"
	};
	NOVA_IMG img_info = {
		.fmt = NOVA_IMG_JPG,
		.path = "ILSVRC2012_val_00000002.JPEG"
	};
	NOVA_INPUT input_info = {0};

	if (argc > 2) {
		printf("Usage: %s <JPEG file path>\n", argv[0]);
		return -1;
	} else {
		if (argc == 2) {
			snprintf(img_info.path, PATH_LEN, argv[1]);
		}
	}

	printf("model path: %s\n", model_info.model_path);
	printf("image path: %s\n", img_info.path);

	nova_model_init(&model_info);
	nova_img_init(&img_info);
	nova_input_init(&input_info, &model_info, &img_info);

	nova_model_inference(&model_info, &input_info);
	nova_cls_model_postproc(&model_info);

	nova_input_uninit(&input_info);
	nova_img_uninit(&img_info);
	nova_model_uninit(&model_info);

	return 0;
}
