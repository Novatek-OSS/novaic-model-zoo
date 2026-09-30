/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <string.h>
#include <math.h>
#include "hd_type.h"
#include "kwrap/debug.h"
#include "util_img.h"
#include "util_model.h"

/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
#define DET_NUM_CLASS           80

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT nova_output_print_roi(NOVA_DET_BBOX *bbox, UINT32 bbox_ofs, INT32 obj_num, NOVA_MODEL *model_info)
{
	INT32 k;
	CHAR cls_name[LABEL_LEN];
	INT32 fw_num = (obj_num < 10) ? obj_num : 10;

	if (fw_num >= 1) {
		printf("Top %d results:\n", fw_num);
		printf("+----+--------------+--------+--------+--------+--------+--------+\n");
		printf("| ID |    Class     |  Conf  |  xmin  |  ymin  |  xmax  |  ymax  |\n");
		printf("+----+--------------+--------+--------+--------+--------+--------+\n");

		for (INT32 j = 0; j < fw_num && j < 10; ++j) {
			NOVA_DET_BBOX *box = (NOVA_DET_BBOX *)((UINTPTR)bbox + j * bbox_ofs);
			INT32 cls_id = box->cls_id;

			if (cls_id < 0 || cls_id >= DET_NUM_CLASS) {
				printf("invalid cls_id = %d\n", (int)cls_id);
				continue;
			}

			const CHAR *label = model_info->class_labels[cls_id];
			for (k = 0; k < LABEL_LEN - 1; ++k) {
				if (label[k] == '\0' ||
					label[k] == '\r' ||
					label[k] == '\n') {
					break;
				}
				cls_name[k] = label[k];
			}
			cls_name[k] = '\0';
			int name_len = (int)strlen(cls_name);
			if (name_len > 12) {
				name_len = 12;
			}
			int left_space = (12 - name_len) / 2;
			int right_space = 12 - name_len - left_space;

			printf(
				"| %2d | %*s%.*s%*s |  %3.1f%% | %5d  | %5d  | %5d  | %5d  |\n",
				(int)cls_id, left_space, "", name_len, cls_name, right_space, "",
				(double)(box->score * 100), (int)floorf(box->xmin),
				(int)floorf(box->ymin), (int)ceilf(box->xmax), (int)ceilf(box->ymax)
			);
		}
		printf("+----+--------------+--------+--------+--------+--------+--------+\n");
	} else {
		printf(">>>>>>>>>>>> no object detected\r\n");
	}

	return HD_OK;
}
