/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <string.h>
#include "util_img.h"
#include "util_model.h"

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/
METHODDEF(void)
my_error_exit(j_common_ptr cinfo)
{
	my_error_ptr myerr = (my_error_ptr)cinfo->err;
	(*cinfo->err->output_message)(cinfo);
	longjmp(myerr->setjmp_buffer, 1);
}

static void nova_rgb2yuv_float(UINT8 r, UINT8 g, UINT8 b, float *y, float *u, float *v)
{
	*y = 0.301 * r + 0.585 * g + 0.114 * b;
	*u = -0.168 * r - 0.332 * g + 0.500 * b + 128;
	*v = 0.499 * r - 0.419 * g - 0.080 * b + 128;
}

static void nova_jpeg_yonly2yuv(NOVA_JPEG *jpeg, HD_VIDEO_PXLFMT dst_fmt, AI_BUF *img_buf, UINT8 *p_tmp)
{
	struct jpeg_decompress_struct *cinfo = jpeg->j_info;
	INT32 j_width = nova_jpeg_get_width(jpeg);
	INT32 j_height = nova_jpeg_get_height(jpeg);

	UINT8 *p_line = (UINT8 *)p_tmp;
	UINT8 uv = 128;

	UINT32 width = img_buf->width;
	UINT32 height = img_buf->height;
	UINT32 line_offset = img_buf->line_ofs;
	UINT8 *p_y = (UINT8 *)img_buf->va;
	UINT8 *p_uv = p_y + height * line_offset;

	UINT8 *p_y_exe = p_y;
	while (cinfo->output_scanline < cinfo->output_height) {
		(void)jpeg_read_scanlines(cinfo, &p_line, 1);
		memcpy(p_y_exe, p_line, j_width);
		// width = ALIGN_CEIL(j_width, 2), if j_width is odd, width is 1 greater than j_width
		p_y_exe[width] = p_y_exe[j_width];
		p_y_exe += line_offset;
	}
	if (j_height % 2) {
		UINT8 *p_y_1 = p_y + j_height * line_offset;
		memcpy(p_y_exe, p_y_1, line_offset);
	}
	memset(p_uv, uv, height * line_offset / 2);
}

static void nova_jpeg_rgb2yuv(NOVA_JPEG *jpeg, HD_VIDEO_PXLFMT dst_fmt, AI_BUF *img_buf, UINT8 *p_tmp)
{
	struct jpeg_decompress_struct *cinfo = jpeg->j_info;
	INT32 j_width = nova_jpeg_get_width(jpeg);
	INT32 j_height = nova_jpeg_get_height(jpeg);
	INT32 j_components = nova_jpeg_get_components(jpeg);

	UINT8 *p_line_0 = (UINT8 *)p_tmp;
	UINT8 *p_line_1 = p_line_0 + j_width * j_components;

	UINT8 r00, g00, b00, r01, g01, b01, r10, g10, b10, r11, g11, b11;
	float y00, u00, v00, y01, u01, v01, y10, u10, v10, y11, u11, v11;

	UINT32 height = img_buf->height;
	UINT32 line_offset = img_buf->line_ofs;
	UINT8 *p_y_0 = (UINT8 *)img_buf->va;
	UINT8 *p_y_1 = p_y_0 + line_offset;
	UINT8 *p_uv = p_y_0 + height * line_offset;
	UINT8 *p_u = NULL, *p_v = NULL;
	if (dst_fmt == HD_VIDEO_PXLFMT_YUV420) {
#if defined(CHIP_CNN25_IPC) || defined(CHIP_CNN30_IPC) || defined(CHIP_CNN30_XVR) || defined(CHIP_NPU)
		p_u = p_uv;
		p_v = p_u + 1;
#elif defined(CHIP_CNN25_XVR)
		p_v = p_uv;
		p_u = p_v + 1;
#endif
	} else {
		printf("ERR: nova_jpeg_rgb2yuv not support dst fmt %d\n", dst_fmt);
		return;
	}
	unsigned int j = 0;
	// rounds j_width down to the nearest multiple of 2, because 2 groups of RGB in parallel
	unsigned int j_loop = (j_width & ~1) * j_components;
	// rounds j_height down to the nearest multiple of 2, because 2 lines in parallel
	j_height = (j_height & ~1);
	while (cinfo->output_scanline < (UINT32)j_height) {

		(void)jpeg_read_scanlines(cinfo, &p_line_0, 1);
		(void)jpeg_read_scanlines(cinfo, &p_line_1, 1);

		UINT8 *p_y_0_exe = p_y_0;
		UINT8 *p_y_1_exe = p_y_1;
		UINT8 *p_u_exe = p_u;
		UINT8 *p_v_exe = p_v;

		for (j = 0; j < j_loop; j += j_components * 2) {

			r00 = p_line_0[j];		g00 = p_line_0[j + 1];	b00 = p_line_0[j + 2];
			r01 = p_line_0[j + 3];	g01 = p_line_0[j + 4];	b01 = p_line_0[j + 5];
			r10 = p_line_1[j];		g10 = p_line_1[j + 1];	b10 = p_line_1[j + 2];
			r11 = p_line_1[j + 3];	g11 = p_line_1[j + 4];	b11 = p_line_1[j + 5];

			nova_rgb2yuv_float(r00, g00, b00, &y00, &u00, &v00);
			nova_rgb2yuv_float(r01, g01, b01, &y01, &u01, &v01);
			nova_rgb2yuv_float(r10, g10, b10, &y10, &u10, &v10);
			nova_rgb2yuv_float(r11, g11, b11, &y11, &u11, &v11);

			float u = (u00 + u01 + u10 + u11) * 0.25f;
			float v = (v00 + v01 + v10 + v11) * 0.25f;

			*p_y_0_exe++ = (UINT8)CLAMP(y00, 0, 255);
			*p_y_0_exe++ = (UINT8)CLAMP(y01, 0, 255);

			*p_y_1_exe++ = (UINT8)CLAMP(y10, 0, 255);
			*p_y_1_exe++ = (UINT8)CLAMP(y11, 0, 255);

			*p_u_exe = (UINT8)CLAMP(u, 0, 255);		p_u_exe += 2;
			*p_v_exe = (UINT8)CLAMP(v, 0, 255);		p_v_exe += 2;
		}
		// one last group of RGB in case of odd width
		if (j_width % 2) {

			r00 = p_line_0[j];	g00 = p_line_0[j + 1];	b00 = p_line_0[j + 2];
			r10 = p_line_1[j];	g10 = p_line_1[j + 1];	b10 = p_line_1[j + 2];

			nova_rgb2yuv_float(r00, g00, b00, &y00, &u00, &v00);
			nova_rgb2yuv_float(r10, g10, b10, &y10, &u10, &v10);

			float u = (u00 + u10) * 0.5f;
			float v = (v00 + v10) * 0.5f;

			*p_y_0_exe++ = (UINT8)CLAMP(y00, 0, 255);
			*p_y_1_exe++ = (UINT8)CLAMP(y10, 0, 255);
			*p_y_0_exe   = (UINT8)CLAMP(y00, 0, 255);
			*p_y_1_exe   = (UINT8)CLAMP(y10, 0, 255);

			*p_u_exe = (UINT8)CLAMP(u, 0, 255);
			*p_v_exe = (UINT8)CLAMP(v, 0, 255);
		}
		p_y_0 += line_offset * 2;
		p_y_1 += line_offset * 2;
		p_u   += line_offset;
		p_v   += line_offset;
	}
	// last height in case of odd height
	while (cinfo->output_scanline < cinfo->output_height) {

		(void)jpeg_read_scanlines(cinfo, &p_line_0, 1);

		UINT8 *p_y_0_exe = p_y_0;
		UINT8 *p_y_1_exe = p_y_1;
		UINT8 *p_u_exe = p_u;
		UINT8 *p_v_exe = p_v;

		for (j = 0; j < j_loop; j += j_components * 2) {

			r00 = p_line_0[j];		g00 = p_line_0[j + 1];	b00 = p_line_0[j + 2];
			r01 = p_line_0[j + 3];	g01 = p_line_0[j + 4];	b01 = p_line_0[j + 5];

			nova_rgb2yuv_float(r00, g00, b00, &y00, &u00, &v00);
			nova_rgb2yuv_float(r01, g01, b01, &y01, &u01, &v01);

			float u = (u00 + u01) * 0.5f;
			float v = (v00 + v01) * 0.5f;

			*p_y_0_exe++ = (UINT8)CLAMP(y00, 0, 255);
			*p_y_0_exe++ = (UINT8)CLAMP(y01, 0, 255);

			*p_y_1_exe++ = (UINT8)CLAMP(y00, 0, 255);
			*p_y_1_exe++ = (UINT8)CLAMP(y01, 0, 255);

			*p_u_exe = (UINT8)CLAMP(u, 0, 255);		p_u_exe += 2;
			*p_v_exe = (UINT8)CLAMP(v, 0, 255);		p_v_exe += 2;
		}
		if (j_width % 2) {

			r00 = p_line_0[j];	g00 = p_line_0[j + 1];	b00 = p_line_0[j + 2];
			r10 = p_line_1[j];	g10 = p_line_1[j + 1];	b10 = p_line_1[j + 2];

			nova_rgb2yuv_float(r00, g00, b00, &y00, &u00, &v00);
			nova_rgb2yuv_float(r10, g10, b10, &y10, &u10, &v10);

			float u = (u00 + u10) * 0.5f;
			float v = (v00 + v10) * 0.5f;

			*p_y_0_exe++ = (UINT8)CLAMP(y00, 0, 255);
			*p_y_1_exe++ = (UINT8)CLAMP(y10, 0, 255);
			*p_y_0_exe   = (UINT8)CLAMP(y00, 0, 255);
			*p_y_1_exe   = (UINT8)CLAMP(y10, 0, 255);

			*p_u_exe = (UINT8)CLAMP(u, 0, 255);
			*p_v_exe = (UINT8)CLAMP(v, 0, 255);
		}
	}
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
int nova_jpeg_get_width(NOVA_JPEG *jpeg)
{
	if (jpeg->j_info == NULL) {
		printf("ERR: nova_jpeg_get_width failed: cinfo is 0x%08lX\n", (unsigned long)jpeg->j_info);
		return -1;
	}
	return jpeg->j_info->output_width;
}

int nova_jpeg_get_height(NOVA_JPEG *jpeg)
{
	if (jpeg->j_info == NULL) {
		printf("ERR: nova_jpeg_get_height failed: cinfo is 0x%08lX\n", (unsigned long)jpeg->j_info);
		return -1;
	}
	return jpeg->j_info->output_height;
}

int nova_jpeg_get_colorspace(NOVA_JPEG *jpeg)
{
	if (jpeg->j_info == NULL) {
		printf("ERR: nova_jpeg_get_colorspace failed: cinfo is 0x%08lX\n", (unsigned long)jpeg->j_info);
		return -1;
	}
	return jpeg->j_info->out_color_space;
}

int nova_jpeg_get_components(NOVA_JPEG *jpeg)
{
	if (jpeg->j_info == NULL) {
		printf("ERR: nova_jpeg_get_components failed: cinfo is 0x%08lX\n", (unsigned long)jpeg->j_info);
		return -1;
	}
	return jpeg->j_info->output_components;
}

int nova_jpeg_get_tempbuf_size(NOVA_JPEG *jpeg, HD_VIDEO_PXLFMT dst_fmt)
{
	int tempbuf_size = 0;
	int width = nova_jpeg_get_width(jpeg);
	int color = nova_jpeg_get_colorspace(jpeg);
	int components = nova_jpeg_get_components(jpeg);
	if (color != JCS_GRAYSCALE && color != JCS_RGB) {
		printf("ERR: nova_jpeg_get_tempbuf_size failed: src color %d not supported\n", color);
		return -1;
	}

	switch (dst_fmt) {
	case HD_VIDEO_PXLFMT_YUV420:
		tempbuf_size = sizeof(UINT8) * components * width * 2;
		break;
	default:
		printf("ERR: nova_jpeg_get_tempbuf_size failed: dst fmt %d not supported\n", dst_fmt);
		tempbuf_size = -1;
		break;
	}
	return tempbuf_size;
}

int nova_jpeg_readimg(NOVA_JPEG *jpeg, HD_VIDEO_PXLFMT dst_fmt, AI_BUF *img_buf, MEM_PARM *img_mem, MEM_PARM *temp_mem)
{
	if (jpeg == NULL) {
		printf("ERR: nova_jpeg_readimg failed: NOVA_JPEG is 0x%08lX\n", (unsigned long)jpeg);
		return -1;
	}
	if (dst_fmt != HD_VIDEO_PXLFMT_YUV420) {
		printf("ERR: nova_jpeg_readimg failed: dst fmt %u not supported\n", dst_fmt);
		return -1;
	}

	INT32 tempbuf_size = nova_jpeg_get_tempbuf_size(jpeg, dst_fmt);
	if (tempbuf_size < 0) {
		return -1;
	} else if (tempbuf_size > 0) {
		nova_mem_alloc(temp_mem, "temp_buf", ALIGN_CEIL_64(tempbuf_size));
	}

	INT32 j_width = nova_jpeg_get_width(jpeg);
	INT32 j_height = nova_jpeg_get_height(jpeg);
	INT32 j_color = nova_jpeg_get_colorspace(jpeg);
	HD_DIM j_dim = {j_width, j_height};
	nova_set_image_buf(img_mem, j_dim, dst_fmt, img_buf);

	switch (j_color) {
	case JCS_GRAYSCALE:
		nova_jpeg_yonly2yuv(jpeg, dst_fmt, img_buf, (UINT8 *)temp_mem->va);
		break;
	case JCS_RGB:
		nova_jpeg_rgb2yuv(jpeg, dst_fmt, img_buf, (UINT8 *)temp_mem->va);
		break;
	default:
		printf("ERR: nova_jpeg_readimg not support jpeg colorspace %d\n", j_color);
		return -1;
	}
	return 0;
}

int nova_jpeg_init(NOVA_JPEG *jpeg, const char *jpeg_path)
{
	if (jpeg_path == NULL) {
		printf("ERR: nova_jpeg_init failed: jpeg_path is 0x%08lX\n", (unsigned long)jpeg_path);
		return -1;
	}

	jpeg->j_info = (struct jpeg_decompress_struct *)malloc(sizeof(struct jpeg_decompress_struct));
	jpeg->j_info->err = jpeg_std_error(&(jpeg->j_err.pub));
	jpeg->j_err.pub.error_exit = my_error_exit;
	if (setjmp(jpeg->j_err.setjmp_buffer)) {
		printf("ERR: nova_jpeg_init failed: setjmp failed\n");
		jpeg_destroy_decompress(jpeg->j_info);
		if (jpeg->j_info) {
			free(jpeg->j_info);
		}
		return -1;
	}

	jpeg->j_file = fopen(jpeg_path, "rb");
	if (jpeg->j_file == NULL) {
		printf("ERR: nova_jpeg_init failed: open %s failed\n", jpeg_path);
		if (jpeg->j_info) {
			free(jpeg->j_info);
		}
		return -1;
	}
	jpeg_create_decompress(jpeg->j_info);
	jpeg_stdio_src(jpeg->j_info, jpeg->j_file);
	(void)jpeg_read_header(jpeg->j_info, TRUE);
	(void)jpeg_start_decompress(jpeg->j_info);

	return 0;
}

int nova_jpeg_uninit(NOVA_JPEG *jpeg)
{
	if (jpeg->j_info == NULL) {
		printf("ERR: nova_jpeg_uninit failed: cinfo is 0x%08lX\n", (unsigned long)jpeg->j_info);
		return -1;
	}
	(void)jpeg_finish_decompress(jpeg->j_info);
	jpeg_destroy_decompress(jpeg->j_info);

	if (jpeg->j_file) {
		fclose(jpeg->j_file);
	}
	jpeg->j_file = NULL;

	if (jpeg->j_info) {
		free(jpeg->j_info);
	}
	jpeg->j_info = NULL;
	return 0;
}

HD_RESULT nova_img_init(NOVA_IMG *img_info)
{
	if (!img_info) {
		return HD_ERR_INV;
	}

	if (img_info->fmt == NOVA_IMG_JPG) {
		NOVA_JPEG *jpeg = (NOVA_JPEG *)calloc(1, sizeof(NOVA_JPEG));
		img_info->info = jpeg;
		return (nova_jpeg_init(jpeg, img_info->path) == 0) ? HD_OK : HD_ERR_FAIL;
	} else {
		return HD_ERR_NOT_SUPPORT;
	}
}

HD_RESULT nova_img_uninit(NOVA_IMG *img_info)
{
	if (!img_info) {
		return HD_ERR_INV;
	}

	if (img_info->fmt == NOVA_IMG_JPG) {
		if (img_info->info) {
			nova_jpeg_uninit(img_info->info);
			free(img_info->info);
			img_info->info = NULL;
		}
	} else {
		return HD_ERR_NOT_SUPPORT;
	}
	return HD_OK;
}
