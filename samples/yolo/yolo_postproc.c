#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "vendor_ai.h"
#include <math.h>
#include <stdlib.h>
#include "yolo_postproc.h"
#include "util_model.h"
#include <sys/time.h>
double exp_math(double x)
{
#if FAST_SIGMOID_EN
	union {
		UINT32 i;
		float f;
	} v;
	v.i = (1 << 23) * (1.4426950409 * x + 126.93490512f);
	return v.f;
#else
	return exp(x);
#endif
}
FLOAT clip(FLOAT n, FLOAT lower, FLOAT upper)
{
	return MAX(lower, MIN(n, upper));
}
int cmp_detection(const void *a, const void *b)
{
	float diff = ((Detection *)b)->score - ((Detection *)a)->score;
	if (diff > 0) {
		return 1;
	} else if (diff < 0) {
		return -1;
	} else {
		return 0;
	}
}

void nova_yolox_transform(INT32 num_kept, INT32 ih, INT32 iw, INT32 input_h, INT32 input_w, NOVA_DET_BBOX *yolo_final_bbox)
{
	FLOAT scale = MIN((float)input_w / (float)iw, (float)input_h / (float)ih);

	for (int i = 0; i < num_kept; ++i) {
		yolo_final_bbox[i].xmin = clip(yolo_final_bbox[i].xmin / scale, 0.0f, iw - 1);
		yolo_final_bbox[i].ymin = clip(yolo_final_bbox[i].ymin / scale, 0.0f, ih - 1);
		yolo_final_bbox[i].xmax = clip(yolo_final_bbox[i].xmax / scale, 0.0f, iw - 1);
		yolo_final_bbox[i].ymax = clip(yolo_final_bbox[i].ymax / scale, 0.0f, ih - 1);
	}
}
FLOAT sigmoid(float x)
{
	FLOAT result;
	result = (float)(1. / (1. + exp_math(-x)));
	return result;
}
FLOAT sigmoid_inverse(FLOAT score)
{
	score = (-logf(1.0 / VAL_CONF_TH - 1.0));
	return score;
}
VOID score_sigmoid(FLOAT *score, FLOAT conf_thre)
{
	FLOAT sigmoid_score = sigmoid_inverse(conf_thre);
	for (UINT32 i = 0; i < ANCHOR_NUM; i++) {
		for (UINT32 j = 0; j < 80; j++) {
			if (score[j * ANCHOR_NUM + i] > sigmoid_score) {
				score[j * ANCHOR_NUM + i] = sigmoid(score[j * ANCHOR_NUM + i]);
			} else {
				score[j * ANCHOR_NUM + i] = 0;
			}
		}
	}
}
VOID topk_indices(FLOAT *arr, INT32 n, INT32 k, INT32 *out_indices)
{
	ScoreIndex *temp = (ScoreIndex *)malloc(n * sizeof(ScoreIndex));
	for (INT32 i = 0; i < n; i++) {
		temp[i].score = arr[i];
		temp[i].index = i;
	}
	for (INT32 i = 0; i < k; i++) {
		INT32 max_idx = i;
		for (INT32 j = i + 1; j < n; j++) {
			if (temp[j].score > temp[max_idx].score) {
				max_idx = j;
			}
		}
		ScoreIndex t = temp[i];
		temp[i] = temp[max_idx];
		temp[max_idx] = t;
		out_indices[i] = temp[i].index;
	}
	free(temp);
}
INT32 find_max_idx1(FLOAT *layer_data_float)
{
	FLOAT max = layer_data_float[0];
	INT32 max_idx = 0;
	INT32 i;

	for (i = 0; i < NUM_CLASS; i++) {
		if (layer_data_float[i] > max) {
			max = layer_data_float[i];
			max_idx = i;
		}
	}
	return max_idx;
}
void nova_transpose_to_hwc(float *in_addr, float *out_addr, int C, int H, int W)
{
	for (int h = 0; h < H; ++h) {
		for (int w = 0; w < W; ++w) {
			int out_row = h * W + w;
			for (int c = 0; c < C; ++c) {
				int in_idx = c * H * W + h * W + w;
				int out_idx = out_row * C + c;
				out_addr[out_idx] = in_addr[in_idx];
			}
		}
	}
}
void quick_sort(NOVA_DET_BBOX *yolo_post_bbox, INT32 left, INT32 right)
{
	NOVA_DET_BBOX *bbox = yolo_post_bbox;
	if (left >= right) {
		return;
	}

	INT32 l = left;
	INT32 r = right;
	FLOAT key_xmin = bbox[left].xmin;
	FLOAT key_ymin = bbox[left].ymin;
	FLOAT key_xmax = bbox[left].xmax;
	FLOAT key_ymax = bbox[left].ymax;
	FLOAT key_score = bbox[left].score;
	FLOAT key_cid = bbox[left].cls_id;

	while (l < r) {
		while ((l < r) && (key_score >= bbox[r].score)) {
			r--;
		}

		if (l < r) {
			bbox[l].xmin = bbox[r].xmin;
			bbox[l].ymin = bbox[r].ymin;
			bbox[l].xmax = bbox[r].xmax;
			bbox[l].ymax = bbox[r].ymax;
			bbox[l].score = bbox[r].score;
			bbox[l].cls_id = bbox[r].cls_id;
		}

		while ((l < r) && (key_score <= bbox[l].score)) {
			l++;
		}

		if (l < r) {
			bbox[r].xmin = bbox[l].xmin;
			bbox[r].ymin = bbox[l].ymin;
			bbox[r].xmax = bbox[l].xmax;
			bbox[r].ymax = bbox[l].ymax;
			bbox[r].score = bbox[l].score;
			bbox[r].cls_id = bbox[l].cls_id;
			r--;
		}
	}
	bbox[l].xmin = key_xmin;
	bbox[l].ymin = key_ymin;
	bbox[l].xmax = key_xmax;
	bbox[l].ymax = key_ymax;
	bbox[l].score = key_score;
	bbox[l].cls_id = key_cid;
	if (left < (l - 1)) {
		quick_sort(yolo_post_bbox, left, (l - 1));
	}
	if ((l + 1) < right) {
		quick_sort(yolo_post_bbox, (l + 1), right);
	}
}
#if defined(CHIP_CNN30_IPC) || defined(CHIP_CNN30_XVR)
void nova_transpose_yolov10(VENDOR_AI3_BUF *p_outbuf, FLOAT *layer_float, FLOAT *layer_float_post)
{
	FLOAT *in_addr = NULL;
	FLOAT *out_addr = NULL;
	in_addr = layer_float;
	out_addr = layer_float_post;

	UINT32 batch = p_outbuf->batch_num;
	UINT32 channels = p_outbuf->channel;
	UINT32 height = p_outbuf->height;

	INT8 order[3] = {0, 2, 1};

	UINT32 in_shape[3], out_shape[3];
	UINT32 in_ofs_old[3], in_ofs[3];
	UINT32 in_c_ofs, in_y_ofs, in_n_ofs;
	FLOAT *nin = NULL, *cin = NULL, *yin = NULL;
	FLOAT *nout = NULL, *cout = NULL, *yout = NULL;
	UINT32 i, n, c, y;

	in_shape[0] = batch;
	in_shape[1] = channels;
	in_shape[2] = height;

	in_ofs_old[0] = channels * height;
	in_ofs_old[1] = height;
	in_ofs_old[2] = 1;

	for (i = 0; i < 3; i++) {
		out_shape[i] = in_shape[order[i]];
		in_ofs[i] = in_ofs_old[order[i]];
	}

	batch = out_shape[0];
	channels = out_shape[1];
	height = out_shape[2];

	in_n_ofs = in_ofs[0];
	in_c_ofs = in_ofs[1];
	in_y_ofs = in_ofs[2];

	nin = in_addr;
	nout = out_addr;
	for (n = 0; n < batch; n++) {
		cin = nin;
		cout = nout;
		for (c = 0; c < channels; c++) {
			yin = cin;
			yout = cout;
			for (y = 0; y < height; y++) {
				*(FLOAT *)yout = *(FLOAT *)yin;
				yin += in_y_ofs;
				yout += 1;
			}
			cin += in_c_ofs;
			cout += height;
		}
		nin += in_n_ofs;
		nout += channels * height;
	}
}
#endif
INT32 nova_yolo_nms(NOVA_DET_BBOX *yolo_final_bbox, NOVA_DET_BBOX *yolo_post_bbox, FLOAT nms_thre, INT32 *bbox_num)
{
	NOVA_DET_BBOX *bbx = yolo_post_bbox;
	NOVA_DET_BBOX *final_bbx = yolo_final_bbox;
	INT32 num = bbox_num[0];
	INT32 i, j, out_num = 0;
	FLOAT tmp_w, tmp_h;
	FLOAT left, right, top, bottom, width, height, u_area, iou;
	if (num == 0) {
		return 0;
	}
	if (num > 1) {
		quick_sort(bbx, 0, num - 1);
	}

	FLOAT *area = (FLOAT *)malloc(sizeof(FLOAT) * num);

	for (i = 0; i < num; ++i) {
		bbx[i].xmax += 4096 * bbx[i].cls_id;
		bbx[i].xmin += 4096 * bbx[i].cls_id;
		bbx[i].ymax += 4096 * bbx[i].cls_id;
		bbx[i].ymin += 4096 * bbx[i].cls_id;
	}

	for (i = 0; i < num; ++i) {
		tmp_w = bbx[i].xmax - bbx[i].xmin + 1;
		tmp_h = bbx[i].ymax - bbx[i].ymin + 1;
		area[i] = tmp_w * tmp_h;
	}

	for (i = 0; i < num; ++i) {
		if (bbx[i].score == -1.0) {
			continue;
		}
		for (j = i + 1; j < num; ++j) {
			if (bbx[j].score == -1.0) {
				continue;
			}
			left = MAX(bbx[i].xmin, bbx[j].xmin);
			right = MIN(bbx[i].xmax, bbx[j].xmax);
			top = MAX(bbx[i].ymin, bbx[j].ymin);
			bottom = MIN(bbx[i].ymax, bbx[j].ymax);
			width = MAX(right - left + 1, 0.f);
			height = MAX(bottom - top + 1, 0.f);
			u_area = height * width;
			iou = (u_area) / (area[i] + area[j] - u_area);
			if (iou >= nms_thre) {
				bbx[j].score = -1.0;
				area[j] = -1.0;
			}
		}
	}
	for (i = 0; i < num; ++i) {
		if (bbx[i].score == -1.0) {
			continue;
		}
		final_bbx[out_num].xmin = bbx[i].xmin - 4096 * bbx[i].cls_id;
		final_bbx[out_num].ymin = bbx[i].ymin - 4096 * bbx[i].cls_id;
		final_bbx[out_num].xmax = bbx[i].xmax - 4096 * bbx[i].cls_id;
		final_bbx[out_num].ymax = bbx[i].ymax - 4096 * bbx[i].cls_id;
		final_bbx[out_num].score = bbx[i].score;
		final_bbx[out_num].cls_id = bbx[i].cls_id;
		out_num++;
	}
	free(area);
	return out_num;
}

void nova_transform(INT32 num_kept, INT32 ih, INT32 iw, INT32 input_h, INT32 input_w, NOVA_DET_BBOX *yolo_final_bbox, BOOL is_padding)
{
	FLOAT scale;
	NOVA_DET_BBOX *bbox = yolo_final_bbox;
	INT32 nh, nw, dh, dw;
	INT32 i;
	if (is_padding) {
		scale = MIN((float)(input_w) / (float)(iw), (float)(input_h) / (float)(ih));
		nh = (INT32)(scale * (float)(ih));
		nw = (INT32)(scale * (float)(iw));
		dh = (input_h - nh) / 2;
		dw = (input_w - nw) / 2;

		for (i = 0; i < num_kept; ++i) {
			bbox[i].xmin = clip((bbox[i].xmin - dw) / scale, 0.0f, iw - 1);
			bbox[i].ymin = clip((bbox[i].ymin - dh) / scale, 0.0f, ih - 1);
			bbox[i].xmax = clip((bbox[i].xmax - dw) / scale, 0.0f, iw - 1);
			bbox[i].ymax = clip((bbox[i].ymax - dh) / scale, 0.0f, ih - 1);
		}
	} else {
		for (i = 0; i < num_kept; ++i) {
			bbox[i].xmin = clip(bbox[i].xmin * iw / input_w, 0.0f, iw - 1);
			bbox[i].ymin = clip(bbox[i].ymin * ih / input_h, 0.0f, ih - 1);
			bbox[i].xmax = clip(bbox[i].xmax * iw / input_w, 0.0f, iw - 1);
			bbox[i].ymax = clip(bbox[i].ymax * ih / input_h, 0.0f, ih - 1);
		}
	}
}

void nova_decode_center_size_to_bbox(NOVA_DET_BBOX *p, UINT32 i, FLOAT score, FLOAT *bbox, UINT32 cls)
{
	FLOAT cx, cy, w_b, h_b;
	cx = bbox[0 * ANCHOR_NUM + i];
	cy = bbox[1 * ANCHOR_NUM + i];
	w_b = bbox[2 * ANCHOR_NUM + i];
	h_b = bbox[3 * ANCHOR_NUM + i];
	p->xmin = cx - w_b / 2;
	p->ymin = cy - h_b / 2;
	p->xmax = cx + w_b / 2;
	p->ymax = cy + h_b / 2;
	p->score = score;
	p->cls_id = cls;
}

UINT32 yolo_decode_anchor_predictions(FLOAT *score, FLOAT *bbox, NOVA_DET_BBOX *p_predicts, FLOAT conf_thre)
{
	INT32 predicts_ind = 0;
	FLOAT sigmoid_score = 0.0f;
	sigmoid_score = sigmoid_inverse(sigmoid_score);
	for (UINT32 i = 0; i < ANCHOR_NUM; i++) {
		// find max score and cls
		FLOAT max_score = 0;
		UINT32 cls = 0;
		for (UINT32 j = 0; j < 80; j++) {
			// with sigmoid
			if (score[j * ANCHOR_NUM + i] > (sigmoid_score)) {
				FLOAT prob = sigmoid(score[j * ANCHOR_NUM + i]);
				if (prob > max_score) {
					max_score = prob;
					cls = j;
				}
			}
		}
		if (max_score > conf_thre) {
			NOVA_DET_BBOX predict = {0};
			nova_decode_center_size_to_bbox(&predict, i, max_score, bbox, cls);
			p_predicts[predicts_ind] = predict;
			predicts_ind++;
		}
	}
	return predicts_ind;
}

INT32 nova_postprocess_anchorfree(NOVA_DET_BBOX *yolo_final_bbox, NOVA_DET_BBOX *yolo_post_bbox, FLOAT **layer_float_post, FLOAT conf_thre, FLOAT nms_thre)
{
	INT32 bbox_num[1] = {0};
	INT32 out_num = 0;
	FLOAT *score = layer_float_post[0];
	FLOAT *bbox = layer_float_post[1];
	bbox_num[0] = yolo_decode_anchor_predictions(score, bbox, yolo_post_bbox, conf_thre);
	out_num = nova_yolo_nms(yolo_final_bbox, yolo_post_bbox, nms_thre, bbox_num);
	return out_num;
}
#if defined(CHIP_CNN30_IPC) || defined(CHIP_CNN30_XVR)
INT32 nova_yolox_decode_predictions(float **layer_float_post, NOVA_DET_BBOX *p_predicts, float conf_thre, int *Strides)
{
	int shapes[3] = {YOLO_L0_SIZE, YOLO_L1_SIZE, YOLO_L2_SIZE};
	int counts[3] = {YOLO_L0_COUNT, YOLO_L1_COUNT, YOLO_L2_COUNT};

	Detection dets[ANCHOR_NUM];
	int det_count = 0;

	for (int l = 0; l < 3; ++l) {
		int w = shapes[l];
		int stride = Strides[l];

		float *reg = layer_float_post[3 * l + 0];  // reg
		float *obj = layer_float_post[3 * l + 1];  // obj
		float *cls = layer_float_post[3 * l + 2];  // cls

		int total = counts[l];

		for (int idx = 0; idx < total; ++idx) {
			int gy = idx / w;
			int gx = idx % w;

			int reg_base = 4 * (gy * w + gx);   // [0,1,2,3]
			int obj_base = (gy * w + gx);       // [0]
			int cls_base = 80 * (gy * w + gx);  // [0~79]

			float x = reg[reg_base + 0];
			float y = reg[reg_base + 1];
			float w_box = reg[reg_base + 2];
			float h_box = reg[reg_base + 3];

			float obj_score = sigmoid(obj[obj_base]);
			if (obj_score <= 0.001f) {
				continue;
			}

			int best_class = -1;
			float best_class_score = 0.0f;

			for (int c = 0; c < 80; ++c) {
				float class_score = sigmoid(cls[cls_base + c]);
				if (class_score <= 0.001f) {
					continue;
				}

				float total_score = obj_score * class_score;
				if (total_score > best_class_score) {
					best_class_score = total_score;
					best_class = c;
				}
			}

			if (best_class_score < conf_thre) {
				continue;
			}

			float box_cx = (x + gx) * stride;
			float box_cy = (y + gy) * stride;
			float box_w = expf(w_box) * stride;
			float box_h = expf(h_box) * stride;

			dets[det_count].xc = box_cx;
			dets[det_count].yc = box_cy;
			dets[det_count].w = box_w;
			dets[det_count].h = box_h;
			dets[det_count].score = best_class_score;
			dets[det_count].class_id = best_class;
			det_count++;
		}
	}

	qsort(dets, det_count, sizeof(Detection), cmp_detection);

	int out_count = 0;
	for (int i = 0; i < det_count; ++i) {
		if (dets[i].score > 0.001f) {
			float cx = dets[i].xc, cy = dets[i].yc;
			float w = dets[i].w, h = dets[i].h;

			p_predicts[out_count].xmin = cx - w / 2.0f;
			p_predicts[out_count].ymin = cy - h / 2.0f;
			p_predicts[out_count].xmax = cx + w / 2.0f;
			p_predicts[out_count].ymax = cy + h / 2.0f;
			p_predicts[out_count].score = dets[i].score;
			p_predicts[out_count].cls_id   = dets[i].class_id;
			out_count++;
		}
	}

	return out_count;
}
INT32 nova_postprocess_yolox(NOVA_DET_BBOX *yolo_final_bbox, NOVA_DET_BBOX *yolo_post_bbox, FLOAT **layer_float_post, INT32 *Strides, FLOAT conf_thre, FLOAT nms_thre)
{
	INT32 bbox_num[1] = {0};
	INT32 out_num = 0;

	bbox_num[0] = nova_yolox_decode_predictions(layer_float_post, yolo_post_bbox, conf_thre, Strides);
	out_num = nova_yolo_nms(yolo_final_bbox, yolo_post_bbox, nms_thre, bbox_num);

	return out_num;
}
INT32 nova_postprocess_yolov10(NOVA_DET_BBOX *yolo_final_bbox, NOVA_DET_BBOX *yolo_post_bbox, FLOAT **layer_float_post, FLOAT **conf_loc_layer_float, VENDOR_AI3_BUF *layer_buffer, FLOAT conf_thre, FLOAT nms_thresh)
{
	INT32 bbox_num[1] = {0};
	NOVA_DET_BBOX *predict_bbox = yolo_post_bbox;
	for (UINT32 i = 0; i < 2; i++) {
		nova_transpose_yolov10(&(layer_buffer[i]), conf_loc_layer_float[i], layer_float_post[i]);
	}
	score_sigmoid(layer_float_post[0], conf_thre);
	FLOAT *max_scores = (FLOAT *)malloc(ANCHOR_NUM * sizeof(FLOAT));
	INT32 *max_labels = (INT32 *)malloc(ANCHOR_NUM * sizeof(INT32));
	UINT32 final_anchor = 0;
	for (INT32 i = 0; i < ANCHOR_NUM; i++) {
		FLOAT max_score = conf_thre;
		INT32 max_label = 0;
		UINT32 anchor_num = 0;

		for (INT32 c = 0; c < NUM_CLASS; c++) {
			FLOAT s = layer_float_post[0][i * NUM_CLASS + c];
			if (s > max_score) {
				max_score = s;
				max_label = c;
				anchor_num = 1;
			}
		}
		if (anchor_num) {
			final_anchor++;
		}
		max_scores[i] = max_score;
		max_labels[i] = max_label;
	}

	bbox_num[0] = MIN(final_anchor, MAX_DET);
	INT32 *topk_indices_arr = (INT32 *)malloc(bbox_num[0] * sizeof(INT32));

	topk_indices(max_scores, ANCHOR_NUM, bbox_num[0], topk_indices_arr);

	for (INT32 k = 0; k < bbox_num[0]; k++) {
		INT32 idx = topk_indices_arr[k];
		predict_bbox[k].xmin  = layer_float_post[1][idx * 4 + 0];
		predict_bbox[k].ymin  = layer_float_post[1][idx * 4 + 1];
		predict_bbox[k].xmax  = layer_float_post[1][idx * 4 + 2];
		predict_bbox[k].ymax  = layer_float_post[1][idx * 4 + 3];
		predict_bbox[k].score = max_scores[idx];
		predict_bbox[k].cls_id = max_labels[idx];
	}
	free(max_scores);
	free(max_labels);
	free(topk_indices_arr);
	int out_num = nova_yolo_nms(yolo_final_bbox, yolo_post_bbox, nms_thresh, bbox_num);
	return out_num;
}
#endif
void nova_postprocess_float(NOVA_DET_BBOX *yolov5s_post_bbox, INT32 input_w, INT32 input_h, INT32 height, INT32 width, INT32 idx, FLOAT conf_thre, FLOAT *layer_data_float, INT32 *strides, YOLO_Anchor *anchor, INT32 *bbox_num, UINT32 model_mode)
{
	NOVA_DET_BBOX tmp_bbox;
	NOVA_DET_BBOX *predict_bbox = yolov5s_post_bbox;
	FLOAT cx, cy, w_b, h_b;
	FLOAT score = 0;
	INT32 cls_id;
	FLOAT *ptr = layer_data_float;
	FLOAT *cls_ptr = NULL;
	INT32 a;

	FLOAT h = 0.0f, w = 0.0f;
	FLOAT h_max, w_max;
	FLOAT h_min = -0.5f, w_min = -0.5f;
	h_max = (FLOAT)(height) - 1.5f;
	w_max = (FLOAT)(width) - 1.5f;

	INT32 bbox_ind = bbox_num[0];

	for (a = 0; a < 3; ++a) {
		for (h = h_min; h <= h_max; h = h + 1.0f) {
			for (w = w_min; w <= w_max; w = w + 1.0f) {

				cls_ptr = ptr + 5;
				cls_id = find_max_idx1(cls_ptr);

				score = ptr[4] * cls_ptr[cls_id];

				if (score >= conf_thre) {
					cx = (ptr[0] * 2.0f + w) * (FLOAT)(strides[idx]);
					cy = (ptr[1] * 2.0f + h) * (FLOAT)(strides[idx]);
					w_b = pow((ptr[2] * 2.0f), 2.0f) * anchor[idx * 3 + a].width;
					h_b = pow((ptr[3] * 2.0f), 2.0f) * anchor[idx * 3 + a].height;

					tmp_bbox.xmin  = clip(cx - w_b / 2.0f, 0.0f, (FLOAT)(input_w - 1));
					tmp_bbox.ymin  = clip(cy - h_b / 2.0f, 0.0f, (FLOAT)(input_h - 1));
					tmp_bbox.xmax  = clip(cx + w_b / 2.0f, 0.0f, (FLOAT)(input_w - 1));
					tmp_bbox.ymax  = clip(cy + h_b / 2.0f, 0.0f, (FLOAT)(input_h - 1));
					tmp_bbox.score  = score;
					tmp_bbox.cls_id    = cls_id;

					predict_bbox[bbox_ind] = tmp_bbox;
					bbox_ind++;

					if (bbox_ind > PREDICT_NUMBERS) {
						printf("err: bbox_ind:%d should < PREDICT_NUMBERS:%d\n",
							   bbox_ind, PREDICT_NUMBERS);
					}
				}
				ptr += 5 + NUM_CLASS;
			}
		}
	}

	bbox_num[0] = bbox_ind;
}
INT32 nova_postprocess_anchorbase(NOVA_DET_BBOX *yolov5s_final_bbox, NOVA_DET_BBOX *yolov5s_post_bbox, FLOAT **layer_float_post, YOLO_Anchor *anchors, INT32 *Strides, FLOAT conf_thre, FLOAT nms_thre, UINT32 model_mode)
{
	UINT32 i;
	INT32  stride, width, height;
	INT32  input_w = INPUT_W;
	INT32  input_h = INPUT_H;
	INT32  bbox_num[1] = {0};
	INT32  out_num = 0;
	FLOAT *cur_output_float = NULL;
	for (i = 0; i < 3; i++) {
		stride = Strides[i];
		width  = (input_w + stride - 1) / stride;
		height = (input_h + stride - 1) / stride;
		cur_output_float = layer_float_post[i];
		nova_postprocess_float(yolov5s_post_bbox, input_w, input_w, height, width, i, conf_thre, cur_output_float, Strides, anchors, bbox_num, model_mode);
	}
	out_num = nova_yolo_nms(yolov5s_final_bbox, yolov5s_post_bbox, nms_thre, bbox_num);
	return out_num;
}
HD_RESULT nova_yolo_postproc(NOVA_MODEL *model_info, NOVA_INPUT *in_info, UINT32 yolo_ver)
{
	HD_RESULT ret = HD_OK;
	INT32 num_kept = 0;
	UINT32 i;
	FLOAT nms_thresh;
	FLOAT conf_thresh;
	struct timeval tstart_postproc, tend_postproc;
	UINT32 time_prerf_postproc = 0;
	UINT32 size_predicts_init = PREDICT_NUMBERS * sizeof(NOVA_DET_BBOX) + 32;
	UINT32 size_final_pred = FINAL_PRED_NUMBERS * sizeof(NOVA_DET_BBOX) + 32;
	NOVA_DET_BBOX *yolo_post_bbox = (NOVA_DET_BBOX *)malloc(size_predicts_init);
	NOVA_DET_BBOX *yolo_final_bbox = (NOVA_DET_BBOX *)malloc(size_final_pred);
	memset(yolo_post_bbox, 0, size_predicts_init);
	memset(yolo_final_bbox, 0, size_final_pred);
	NET_PROC *p_net = &model_info->proc_info;
	FLOAT **layer_float = (FLOAT **)malloc(p_net->net_info.out_buf_cnt * sizeof(FLOAT *));

#if defined(CHIP_CNN30_IPC) || defined(CHIP_CNN30_XVR)
	VENDOR_AI3_BUF ai_buf = {0};
	for (i = 0; i < p_net->net_info.out_buf_cnt; i++) {
		ret = vendor_ai3_net_get(p_net->proc_id, p_net->net_info.out_path_list[i], &ai_buf);
		if (HD_OK != ret) {
			printf("proc_id(%u) get out buf fail, i(%d), out_path(0x%x)\n", p_net->proc_id, i, p_net->net_info.out_path_list[i]);
			return HD_ERR_FAIL;
		}
		UINT32 length = ai_buf.width * ai_buf.height * ai_buf.channel * ai_buf.batch_num;
		if (yolo_ver == YOLOX || yolo_ver == YOLOV10) {
			layer_float[i] = (FLOAT *)malloc(sizeof(FLOAT) * length);
		} else {
			layer_float[i] = NULL;
		}
	}
#endif

	if (model_info->prof_postproc) {
		gettimeofday(&tstart_postproc, NULL);
	}

	if (yolo_ver == YOLOV5 || yolo_ver == YOLOV7) {
		YOLO_Anchor anchors[9];
		if (yolo_ver == YOLOV5) {
			YOLO_Anchor v5_anchors[9] = {{10, 13}, {16, 30}, {33, 23}, {30, 61}, {62, 45}, {59, 119}, {116, 90}, {156, 198}, {373, 326}};
			memcpy(anchors, v5_anchors, sizeof(v5_anchors));
		} else {
			YOLO_Anchor v7_anchors[9] = {{12, 16}, {19, 36}, {40, 28}, {36, 75}, {76, 55}, {72, 146}, {142, 110}, {192, 243}, {459, 401}};
			memcpy(anchors, v7_anchors, sizeof(v7_anchors));
		}

		INT32 Strides[3] = {8, 16, 32};
		if (VAL_MODE) {
			nms_thresh = VAL_NMS_TH_V5_V7_v11_v12_v13;
			conf_thresh = VAL_CONF_TH;
		} else {
			nms_thresh = INFER_NMS_TH;
			conf_thresh = INFER_CONF_TH;
		}

		num_kept = nova_postprocess_anchorbase(yolo_final_bbox, yolo_post_bbox, p_net->float_out, anchors, Strides, conf_thresh, nms_thresh, yolo_ver);

		if (num_kept == 0) {
			printf("%s\n", "Couldn't find any detections");
		} else {
			nova_transform(num_kept, in_info->letterbox.orig_h, in_info->letterbox.orig_w, INPUT_H, INPUT_W, yolo_final_bbox, IsPadding);
		}
	} else if (yolo_ver == YOLOV8 || yolo_ver == YOLOV11 || yolo_ver == YOLO26) {
		if (VAL_MODE) {
			if (yolo_ver == YOLOV8 || yolo_ver == YOLO26) {
				nms_thresh = VAL_NMS_TH_V8_X_v10_26;
				conf_thresh = VAL_CONF_TH;
			} else {
				nms_thresh = VAL_NMS_TH_V5_V7_v11_v12_v13;
				conf_thresh = VAL_CONF_TH;
			}
		} else {
			nms_thresh = INFER_NMS_TH;
			conf_thresh = INFER_CONF_TH;
		}

		num_kept = nova_postprocess_anchorfree(yolo_final_bbox, yolo_post_bbox, p_net->float_out, conf_thresh, nms_thresh);
		if (num_kept == 0) {
			printf("%s\n", "Couldn't find any detections");
		} else {
			nova_transform(num_kept, in_info->letterbox.orig_h, in_info->letterbox.orig_w, INPUT_H, INPUT_W, yolo_final_bbox, IsPadding);
		}
	}
#if defined(CHIP_CNN30_IPC) || defined(CHIP_CNN30_XVR)
	else if (yolo_ver == YOLOV12 || yolo_ver == YOLOV13) {
		if (VAL_MODE) {
			nms_thresh = VAL_NMS_TH_V5_V7_v11_v12_v13;
			conf_thresh = VAL_CONF_TH;
		} else {
			nms_thresh = INFER_NMS_TH;
			conf_thresh = INFER_CONF_TH;
		}
		num_kept = nova_postprocess_anchorfree(yolo_final_bbox, yolo_post_bbox, p_net->float_out, conf_thresh, nms_thresh);
		if (num_kept == 0) {
			printf("%s\n", "Couldn't find any detections");
		} else {
			nova_transform(num_kept, in_info->letterbox.orig_h, in_info->letterbox.orig_w, INPUT_H, INPUT_W, yolo_final_bbox, IsPadding);
		}
	} else if (yolo_ver == YOLOX) {
		INT32 Strides[3] = {8, 16, 32};
		if (VAL_MODE) {
			nms_thresh = VAL_NMS_TH_V8_X_v10_26;
			conf_thresh = VAL_CONF_TH;
		} else {
			nms_thresh = INFER_NMS_TH;
			conf_thresh = INFER_CONF_TH;
		}

		INT32 H_W[3] = {80, 40, 20};
		INT32 C[3]   = {4, 1, 80};

		for (UINT32 i = 0; i < 9; i++) {
			int layer = i / 3;   // 0,1,2
			int branch = i % 3;  // 0 reg, 1 obj, 2 cl
			nova_transpose_to_hwc(p_net->float_out[i], layer_float[i], C[branch], H_W[layer], H_W[layer]);
		}

		num_kept = nova_postprocess_yolox(yolo_final_bbox, yolo_post_bbox, layer_float, Strides, conf_thresh, nms_thresh);
		if (num_kept == 0) {
			printf("%s\n", "Couldn't find any detections");
		} else {
			nova_yolox_transform(num_kept, in_info->letterbox.orig_h, in_info->letterbox.orig_w, INPUT_H, INPUT_W, yolo_final_bbox);
		}
	} else if (yolo_ver == YOLOV10) {
		if (VAL_MODE) {
			nms_thresh = VAL_NMS_TH_V8_X_v10_26;
			conf_thresh = VAL_CONF_TH;
		} else {
			nms_thresh = INFER_NMS_TH;
			conf_thresh = INFER_CONF_TH;
		}
		num_kept = nova_postprocess_yolov10(yolo_final_bbox, yolo_post_bbox, layer_float, p_net->float_out, p_net->out_buf, conf_thresh, nms_thresh);
		nova_transform(num_kept, in_info->letterbox.orig_h, in_info->letterbox.orig_w, INPUT_H, INPUT_W, yolo_final_bbox, IsPadding);
	}
#endif
	else {
		printf("please check your model mode\n");
	}

	if (model_info->prof_postproc) {
		gettimeofday(&tend_postproc, NULL);
		time_prerf_postproc = (tend_postproc.tv_sec - tstart_postproc.tv_sec) * 1000000 + (tend_postproc.tv_usec - tstart_postproc.tv_usec);
		printf("postprocess time: %u (us) \n", time_prerf_postproc);
	}

	ret = nova_output_print_roi(yolo_final_bbox, sizeof(NOVA_DET_BBOX), num_kept, model_info);

	free(yolo_post_bbox);
	free(yolo_final_bbox);
	for (i = 0; i < p_net->net_info.out_buf_cnt; i++) {
		free(layer_float[i]);
	}
	free(layer_float);
	sleep(1); // avoid interleaved console output with other threads
	return ret;
}