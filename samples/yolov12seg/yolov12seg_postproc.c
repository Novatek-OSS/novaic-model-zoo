/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <string.h>
#include <limits.h>
#include <math.h>
#include <sys/stat.h>
#include "yolov12seg_postproc.h"

/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
#define YOLOV12SEG_DBG  0               ///< print debug info

// global variables
// ClassMask class_masks[NUM_CLASSES];  // mask maps for NUM_CLASSES
UINT8 merged_mask[PROTO_HEIGHT][PROTO_WIDTH];  // final class ID (0~79 or 255)
UINT8 max_conf[PROTO_HEIGHT][PROTO_WIDTH];     // max confidence of each pixel (0~255)
YOLO_BBOX yolo_predict[MAX_DETECT_NUM];
// network post-processing function
float sigmoid_table[] = { 0.500000,0.502500,0.505000,0.507499,0.509999,0.512497,0.514996,0.517493,0.519989,
    0.522485,0.524979,0.527472,0.529964,0.532454,0.534943,0.537430,0.539915,0.542398,0.544879,0.547358,0.549834,
    0.552308,0.554779,0.557248,0.559714,0.562177,0.564636,0.567093,0.569546,0.571996,0.574443,0.576885,0.579324,
    0.581759,0.584191,0.586618,0.589040,0.591459,0.593873,0.596283,0.598688,0.601088,0.603483,0.605874,0.608259,
    0.610639,0.613014,0.615384,0.617748,0.620106,0.622459,0.624806,0.627148,0.629483,0.631812,0.634136,0.636453,
    0.638763,0.641067,0.643365,0.645656,0.647941,0.650219,0.652489,0.654753,0.657010,0.659260,0.661503,0.663739,
    0.665967,0.668188,0.670401,0.672607,0.674805,0.676996,0.679179,0.681354,0.683521,0.685680,0.687831,0.689974,
    0.692110,0.694236,0.696355,0.698465,0.700567,0.702661,0.704746,0.706822,0.708890,0.710949,0.713000,0.715042,
    0.717075,0.719100,0.721115,0.723122,0.725120,0.727108,0.729088,0.731059,0.733020,0.734973,0.736916,0.738850,
    0.740775,0.742691,0.744597,0.746494,0.748382,0.750260,0.752129,0.753989,0.755839,0.757680,0.759511,0.761333,
    0.763145,0.764948,0.766741,0.768525,0.770299,0.772064,0.773819,0.775564,0.777300,0.779026,0.780743,0.782450,
    0.784147,0.785835,0.787513,0.789182,0.790841,0.792490,0.794130,0.795760,0.797380,0.798991,0.800592,0.802184,
    0.803766,0.805338,0.806901,0.808455,0.809998,0.811533,0.813057,0.814573,0.816078,0.817575,0.819061,0.820538,
    0.822006,0.823465,0.824914,0.826353,0.827784,0.829204,0.830616,0.832018,0.833411,0.834795,0.836170,0.837535,
    0.838891,0.840238,0.841576,0.842905,0.844224,0.845535,0.846836,0.848129,0.849412,0.850687,0.851953,0.853210,
    0.854458,0.855697,0.856927,0.858149,0.859362,0.860566,0.861762,0.862949,0.864127,0.865297,0.866458,0.867611,
    0.868756,0.869892,0.871019,0.872138,0.873249,0.874352,0.875447,0.876533,0.877611,0.878681,0.879743,0.880797,
    0.881843,0.882881,0.883911,0.884933,0.885948,0.886954,0.887953,0.888944,0.889927,0.890903,0.891871,0.892832,
    0.893785,0.894731,0.895669,0.896600,0.897523,0.898439,0.899348,0.900250,0.901144,0.902031,0.902911,0.903784,
    0.904651,0.905510,0.906362,0.907207,0.908045,0.908877,0.909702,0.910520,0.911331,0.912136,0.912934,0.913726,
    0.914511,0.915289,0.916062,0.916827,0.917587,0.918340,0.919087,0.919827,0.920561,0.921290,0.922012,0.922728,
    0.923438,0.924142,0.924840,0.925532,0.926218,0.926899,0.927574,0.928242,0.928906,0.929563,0.930215,0.930862,
    0.931502,0.932138,0.932768,0.933392,0.934011,0.934625,0.935233,0.935836,0.936434,0.937027,0.937614,0.938197,
    0.938774,0.939346,0.939913,0.940476,0.941033,0.941585,0.942133,0.942676,0.943214,0.943747,0.944276,0.944799,
    0.945319,0.945833,0.946343,0.946849,0.947350,0.947846,0.948339,0.948826,0.949310,0.949789,0.950264,0.950734,
    0.951200,0.951662,0.952120,0.952574,0.953024,0.953470,0.953911,0.954349,0.954783,0.955212,0.955638,0.956060,
    0.956478,0.956893,0.957303,0.957710,0.958113,0.958513,0.958909,0.959301,0.959690,0.960075,0.960456,0.960834,
    0.961209,0.961580,0.961948,0.962312,0.962673,0.963031,0.963385,0.963736,0.964084,0.964429,0.964770,0.965109,
    0.965444,0.965776,0.966105,0.966431,0.966754,0.967074,0.967391,0.967705,0.968016,0.968324,0.968629,0.968931,
    0.969231,0.969528,0.969822,0.970113,0.970402,0.970688,0.970971,0.971251,0.971529,0.971805,0.972077,0.972348,
    0.972615,0.972880,0.973143,0.973403,0.973661,0.973916,0.974169,0.974419,0.974667,0.974913,0.975156,0.975398,
    0.975636,0.975873,0.976107,0.976339,0.976569,0.976797,0.977023,0.977246,0.977467,0.977687,0.977904,0.978119,
    0.978332,0.978543,0.978752,0.978959,0.979164,0.979367,0.979568,0.979767,0.979964,0.980160,0.980353,0.980545,
    0.980735,0.980923,0.981109,0.981293,0.981476,0.981657,0.981836,0.982014,0.982190,0.982364,0.982536,0.982707,
    0.982876,0.983043,0.983209,0.983374,0.983536,0.983697,0.983857,0.984015,0.984172,0.984327,0.984480,0.984632,
    0.984783,0.984932,0.985080,0.985226,0.985371,0.985514,0.985656,0.985797,0.985936,0.986074,0.986211,0.986346,
    0.986480,0.986613,0.986745,0.986875,0.987004,0.987131,0.987258,0.987383,0.987507,0.987630,0.987751,0.987872,
    0.987991,0.988109,0.988226,0.988342,0.988456,0.988570,0.988682,0.988794,0.988904,0.989013,0.989121,0.989228,
    0.989334,0.989439,0.989543,0.989646,0.989748,0.989849,0.989949,0.990048,0.990146,0.990243,0.990339,0.990435,
    0.990529,0.990622,0.990715,0.990806,0.990897,0.990987,0.991076,0.991164,0.991251,0.991337,0.991423,0.991507,
    0.991591,0.991674,0.991756,0.991837,0.991918,0.991998,0.992077,0.992155,0.992232,0.992309,0.992385,0.992460,
    0.992535,0.992608,0.992681,0.992754,0.992825,0.992896,0.992966,0.993036,0.993105,0.993173,0.993240 };
static float sigmoid_tbl(float x)
{
	float result;
	int result_index;
	if (x > 4.9) {
		result = 0.9935;
	} else if (x < -4.9) {
		result = 0.0065;
	} else {
		if (x >= 0) {
			result_index = x * 100;
			result = sigmoid_table[result_index];
		} else {
			result_index = abs(x) * 100;
			result = 1 - sigmoid_table[result_index];
		}
	}
	return result;
}
// get pre-selected bounding boxes
static UINT32 get_pre_choose_box(FLOAT *s, FLOAT *b, FLOAT *c, YOLO_BBOX *p_predicts)
{
	int predicts_ind = 0;

	for (UINT32 i = 0; i < ANCHOR_NUM; i++) {
		UINT8 label = 0;
		FLOAT max_score = 0.0f;

		// 1. find max class confidence
		for (UINT8 j = 0; j < NUM_CLASSES; j++) {
			FLOAT current_score = s[j * ANCHOR_NUM + i];
			if (current_score > max_score) {
				max_score = current_score;
				label = j;
			}
		}

		// 2. confidence threshold filtering
		if (max_score > CONF_THRESHOLD) {
			YOLO_BBOX predict = {0};

			// 3. decode bounding box: multiply by input size
			// bbox shape is [4, 8400], absolute coordinates, indices are 0*ANCHOR_NUM+i, 1*ANCHOR_NUM+i, 2*ANCHOR_NUM+i, 3*ANCHOR_NUM+i
			predict.x = b[0 * ANCHOR_NUM + i];  // x center
			predict.y = b[1 * ANCHOR_NUM + i];  // y center
			predict.w = b[2 * ANCHOR_NUM + i];  // width
			predict.h = b[3 * ANCHOR_NUM + i];  // height

			// ensure coordinates within boundaries
			if (predict.x < 0.0f) {
				predict.x = 0.0f;
			}
			if (predict.x > NET_INPUT_WIDTH) {
				predict.x = (FLOAT)NET_INPUT_WIDTH;
			}
			if (predict.y < 0.0f) {
				predict.y = 0.0f;
			}
			if (predict.y > NET_INPUT_HEIGHT) {
				predict.y = (FLOAT)NET_INPUT_HEIGHT;
			}
			if (predict.w < 1.0f) {
				predict.w = 1.0f;
			}
			if (predict.h < 1.0f) {
				predict.h = 1.0f;
			}

			// convert to xyxy format
			predict.xmin = predict.x - predict.w / 2;
			predict.ymin = predict.y - predict.h / 2;
			predict.xmax = predict.x + predict.w / 2;
			predict.ymax = predict.y + predict.h / 2;

			// bounding box clamping
			if (predict.xmin < 0.0f) {
				predict.xmin = 0.0f;
			}
			if (predict.xmin > NET_INPUT_WIDTH) {
				predict.xmin = (FLOAT)NET_INPUT_WIDTH;
			}
			if (predict.ymin < 0.0f) {
				predict.ymin = 0.0f;
			}
			if (predict.ymin > NET_INPUT_HEIGHT) {
				predict.ymin = (FLOAT)NET_INPUT_HEIGHT;
			}
			if (predict.xmax < 0.0f) {
				predict.xmax = 0.0f;
			}
			if (predict.xmax > NET_INPUT_WIDTH) {
				predict.xmax = (FLOAT)NET_INPUT_WIDTH;
			}
			if (predict.ymax < 0.0f) {
				predict.ymax = 0.0f;
			}
			if (predict.ymax > NET_INPUT_HEIGHT) {
				predict.ymax = (FLOAT)NET_INPUT_HEIGHT;
			}

			// 4. cid/score/anchor-index
			predict.anchor_ind = i;
			predict.score = max_score;
			predict.cls_id = label;

			// 5. copy mask coefficients
			for (int m = 0; m < PROTO_CHANNELS; m++) {
				// mask_coeff shape is [32, 8400]
				int mask_offset = m * ANCHOR_NUM + i;
				predict.mask_coeff[m] = c[mask_offset];
			}

			p_predicts[predicts_ind] = predict;
			predicts_ind++;

			if (predicts_ind > MAX_DETECT_NUM - 1) {
				printf("over limit detect num: %d\r\n", MAX_DETECT_NUM);
				return predicts_ind;
			}
		}
	}
	return predicts_ind;
}
// comparison function for qsort
static int comp(const void *a, const void *b)
{
	YOLO_BBOX *bbox_a = (YOLO_BBOX *)a;
	YOLO_BBOX *bbox_b = (YOLO_BBOX *)b;

	if (bbox_a->score < bbox_b->score) {
		return 1;
	}
	if (bbox_a->score > bbox_b->score) {
		return -1;
	}
	return 0;
}
// calculate overlap length of two intervals
static float overlap(float x1, float w1, float x2, float w2)
{
	float l1 = x1 - w1 / 2;
	float l2 = x2 - w2 / 2;
	float left = (l1 > l2) ? l1 : l2;
	float r1 = x1 + w1 / 2;
	float r2 = x2 + w2 / 2;
	float right = (r1 < r2) ? r1 : r2;
	return (right > left) ? (right - left) : 0;
}
// calculate intersection area of two boxes
static float box_intersection(YOLO_BBOX a, YOLO_BBOX b)
{
	float w = overlap(a.x, a.w, b.x, b.w);
	float h = overlap(a.y, a.h, b.y, b.h);
	if (w <= 0 || h <= 0) {
		return 0;
	}
	return w * h;
}
// calculate union area of two boxes
static float box_union(YOLO_BBOX a, YOLO_BBOX b)
{
	float i = box_intersection(a, b);
	float u = a.w * a.h + b.w * b.h - i;
	return u;
}
// calculate IoU
static float box_iou(YOLO_BBOX a, YOLO_BBOX b)
{
	float union_area = box_union(a, b);
	if (union_area <= 0) {
		return 0;
	}
	return box_intersection(a, b) / union_area;
}
// NMS implementation
static UINT32 nms(YOLO_BBOX *obj, UINT32 objnum, FLOAT iou_th, INT16 scorebias)
{
	UINT32 i, j, k = 0;

	for (i = 0; i < objnum; i++) {
		// skip suppressed boxes
		if (obj[i].score < scorebias) {
			continue;
		}
		for (j = i + 1; j < objnum; j++) {
			// skip suppressed boxes
			if (obj[j].score < scorebias) {
				continue;
			}
			// calculate IoU
			float iou = box_iou(obj[i], obj[j]);
			// if IoU exceeds threshold, suppress lower-confidence box
			if (iou > iou_th) {
				if (obj[i].score >= obj[j].score) {
					obj[j].score = -1;  // suppress box j
				} else {
					obj[i].score = -1;  // suppress box i
					break;  // box i suppressed, break inner loop
				}
			}
		}
		// if box i not suppressed, keep it
		if (obj[i].score >= scorebias) {
			obj[k++] = obj[i];
		}
	}
	return k;
}
// initialize merged mask
static void init_merged_mask(void)
{
	for (int y = 0; y < PROTO_HEIGHT; y++) {
		for (int x = 0; x < PROTO_WIDTH; x++) {
			merged_mask[y][x] = 255;  // background
			max_conf[y][x] = 0;
		}
	}
}
// merge 80 class binary masks into one and save to bin file
void save_merged_mask_to_bin(const char *filename, const NOVA_LETTERBOX *lb_param)
{
	int orig_h = lb_param->orig_h;
	int orig_w = lb_param->orig_w;
	float scale = lb_param->scale;
	int offset_w = lb_param->offset_w;
	int offset_h = lb_param->offset_h;

	// allocate mask buffer in original image size
	UINT8 *mask_full = (UINT8 *)malloc(orig_h * orig_w * sizeof(UINT8));
	if (!mask_full) {
		printf("malloc failed for mask_full\n");
		return;
	}

	// iterate each pixel of original image, nearest-neighbor interpolation from feature map
	for (int i = 0; i < orig_h; i++) {
		for (int j = 0; j < orig_w; j++) {
			// map original image (j,i) to 640x640 coordinate space
			float x_640 = offset_w + j * scale;
			float y_640 = offset_h + i * scale;

			// map to feature map (160x160) coordinates
			float feat_x = x_640 / 4.0f;
			float feat_y = y_640 / 4.0f;

			// round to nearest neighbor
			int fx = (int)roundf(feat_x);
			int fy = (int)roundf(feat_y);

			// boundary check (prevent out-of-bounds)
			if (fx < 0) {
				fx = 0;
			}
			if (fx >= PROTO_WIDTH) {
				fx = PROTO_WIDTH - 1;
			}
			if (fy < 0) {
				fy = 0;
			}
			if (fy >= PROTO_HEIGHT) {
				fy = PROTO_HEIGHT - 1;
			}

			// read from merged_mask
			mask_full[i * orig_w + j] = merged_mask[fy][fx];
		}
	}

	// write to bin file
	char dir[PATH_MAX];
	strncpy(dir, filename, sizeof(dir) - 1);
	dir[sizeof(dir) - 1] = '\0';

	char *p = strrchr(dir, '/');
	if (p) {
		*p = '\0';
		if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
			perror("mkdir");
		}
	}

	FILE *fp = fopen(filename, "wb");
	if (!fp) {
		printf("failed open: %s\n", filename);
		free(mask_full);
		return;
	}
	size_t written = fwrite(mask_full, sizeof(UINT8), orig_h * orig_w, fp);
	if (written != (size_t)(orig_h * orig_w)) {
		printf("write mask failed: wrote %zu, expected %d\n", written, orig_h * orig_w);
	}
	fclose(fp);

	free(mask_full);
	printf("save merged mask to %s, size %dx%d\n", filename, orig_w, orig_h);
}

// segmentation post-processing
void process_segmentation_masks(YOLO_BBOX *predictions, UINT32 num_predictions, float *proto)
{
	// 1. initialize merged mask
	init_merged_mask();

	// 2. process each detection box
	for (UINT32 n = 0; n < num_predictions; n++) {
		int class_id = predictions[n].cls_id;

		// compute bounding box coordinates in 80x80 feature map
		int x1 = (int)(predictions[n].xmin * PROTO_WIDTH / NET_INPUT_WIDTH);
		int y1 = (int)(predictions[n].ymin * PROTO_HEIGHT / NET_INPUT_HEIGHT);
		int x2 = (int)(predictions[n].xmax * PROTO_WIDTH / NET_INPUT_WIDTH);
		int y2 = (int)(predictions[n].ymax * PROTO_HEIGHT / NET_INPUT_HEIGHT);

		// boundary check
		x1 = (x1 < 0) ? 0 : ((x1 >= PROTO_WIDTH) ? PROTO_WIDTH - 1 : x1);
		y1 = (y1 < 0) ? 0 : ((y1 >= PROTO_HEIGHT) ? PROTO_HEIGHT - 1 : y1);
		x2 = (x2 < 0) ? 0 : ((x2 >= PROTO_WIDTH) ? PROTO_WIDTH - 1 : x2);
		y2 = (y2 < 0) ? 0 : ((y2 >= PROTO_HEIGHT) ? PROTO_HEIGHT - 1 : y2);

		// ensure x2 >= x1, y2 >= y1
		if (x2 < x1) {
			int temp = x1;
			x1 = x2;
			x2 = temp;
		}
		if (y2 < y1) {
			int temp = y1;
			y1 = y2;
			y2 = temp;
		}

		// compute mask for pixels inside bounding box
		for (int y = y1; y <= y2; y++) {
			for (int x = x1; x <= x2; x++) {
				float mask_value = 0.0f;
				for (int c = 0; c < PROTO_CHANNELS; c++) {
					int idx = y * PROTO_WIDTH * PROTO_CHANNELS + x * PROTO_CHANNELS + c;
					mask_value += predictions[n].mask_coeff[c] * proto[idx];
				}
				mask_value = sigmoid_tbl(mask_value);
				UINT8 val = (UINT8)(mask_value * 255);

				// assign based on confidence
				if (val > MASK_THRESHOLD && val > max_conf[y][x]) {
					max_conf[y][x] = val;
					merged_mask[y][x] = class_id;
				}
			}
		}
	}
}

#if YOLOV12SEG_DBG
void debug_data(FLOAT *score, FLOAT *bbox, FLOAT *mask_coeff, FLOAT *proto)
{
	// print first few score values
	printf("Debug score[0-4]: ");
	for (int i = 0; i < 5; i++) {
		printf("%.6f ", score[i]);
	}
	printf("\n");

	// print bbox range
	FLOAT bbox_min = 1e9, bbox_max = -1e9;
	for (int i = 0; i < 4 * ANCHOR_NUM; i++) {
		if (bbox[i] < bbox_min) {
			bbox_min = bbox[i];
		}
		if (bbox[i] > bbox_max) {
			bbox_max = bbox[i];
		}
	}
	printf("bbox range: [%.6f, %.6f]\n", bbox_min, bbox_max);

	// print score range
	FLOAT score_min = 1e9, score_max = -1e9;
	for (int i = 0; i < NUM_CLASSES * ANCHOR_NUM; i++) {
		if (score[i] < score_min) {
			score_min = score[i];
		}
		if (score[i] > score_max) {
			score_max = score[i];
		}
	}
	printf("score range: [%.6f, %.6f]\n", score_min, score_max);
}
#endif // YOLOV12SEG_DBG

// coordinate remap
void scale_coords_to_original(float *x1, float *y1, float *x2, float *y2, const NOVA_LETTERBOX *param)
{
	// remap from model coordinate space (640x640) back to original image coordinates
	*x1 = (*x1 - param->offset_w) / param->scale;
	*y1 = (*y1 - param->offset_h) / param->scale;
	*x2 = (*x2 - param->offset_w) / param->scale;
	*y2 = (*y2 - param->offset_h) / param->scale;

	// clamp to original image boundaries
	*x1 = fmaxf(0.0f, fminf(*x1, (float)(param->orig_w - 1)));
	*y1 = fmaxf(0.0f, fminf(*y1, (float)(param->orig_h - 1)));
	*x2 = fmaxf(0.0f, fminf(*x2, (float)(param->orig_w - 1)));
	*y2 = fmaxf(0.0f, fminf(*y2, (float)(param->orig_h - 1)));
}
// post-processing flow
/* dump 1x160x160 mask bin; mask corresponds to image after letterboxing
 * pixel=0-79 is class ID, pixel=255 is background
 */
UINT32 nova_yolov12seg_postproc(NOVA_MODEL *model_info, NOVA_INPUT *in_info, const CHAR *out_filename)
{
	NET_PROC *p_net = &model_info->proc_info;
	NOVA_LETTERBOX *lb_param = &in_info->letterbox;

	memset(yolo_predict, 0, MAX_DETECT_NUM * sizeof(YOLO_BBOX));
	UINT32 predicts_ind = 0;                            // note: output indices may differ across models
	FLOAT *bbox = (FLOAT *)(p_net->float_out[0]);       // p_outbuf_float[0]; [1,4,8400]
	FLOAT *score = (FLOAT *)(p_net->float_out[1]);      // p_outbuf_float[1]; [1,80,8400]
	FLOAT *mask_coeff = (FLOAT *)(p_net->float_out[2]); // p_outbuf_float[2]; [1,32,8400]
	FLOAT *proto = (FLOAT *)(p_net->float_out[3]);      // p_outbuf_float[3]; [1,80,80,32] transpose original data from [1,32,80,80] to [1,80,80,32]
#if YOLOV12SEG_DBG
	debug_data(score, bbox, mask_coeff, proto);
#endif
	// 1. pre-filtering: confidence threshold and bounding box decoding
	predicts_ind = get_pre_choose_box(score, bbox, mask_coeff, yolo_predict);
	if (predicts_ind == 0) {
		return 0;
	}
	// 2. sort by confidence
	qsort(yolo_predict, predicts_ind, sizeof(YOLO_BBOX), comp);
	// 3. apply NMS
	UINT32 num_kept = nms(yolo_predict, predicts_ind, NMS_THRESHOLD, 0);
	// 4. limit maximum detection count
	num_kept = (num_kept < MAX_DETECT_NUM) ? num_kept : MAX_DETECT_NUM;

	// 5. segmentation post-processing
	process_segmentation_masks(yolo_predict, num_kept, proto);
	// 6. print and save results
	for (UINT32 n = 0; n < num_kept; n++) {
		scale_coords_to_original(&yolo_predict[n].xmin, &yolo_predict[n].ymin,
								 &yolo_predict[n].xmax, &yolo_predict[n].ymax,
								 lb_param);
	}
	nova_output_print_roi((NOVA_DET_BBOX *)yolo_predict, sizeof(YOLO_BBOX), num_kept, model_info);
	save_merged_mask_to_bin(out_filename, lb_param);
	return num_kept;
}
