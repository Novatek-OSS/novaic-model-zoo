/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <string.h>
#include "hd_type.h"
#include "kwrap/debug.h"
#include "util_img.h"
#include "util_model.h"

/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/

#define GFX_W_ALIGN             4                   ///< width align of GFX engine limitation
#define GFX_H_ALIGN             2                   ///< height align of GFX engine limitation

/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/
typedef struct _GFX_REGION {
	INT32 offset_w;
	INT32 offset_h;
	INT32 image_w;
	INT32 image_h;
} GFX_REGION;

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/
HD_RESULT nova_resize_yuv420sp(HD_GFX_IMG_BUF *dst_img, HD_GFX_IMG_BUF *src_img, HD_GFX_SCALE_QUALITY method)
{
	HD_RESULT ret = HD_OK;
	HD_GFX_SCALE param;
	memset(&param, 0, sizeof(HD_GFX_SCALE));

	param.src_img.dim.w = src_img->dim.w;
	param.src_img.dim.h = src_img->dim.h;
	param.src_img.format = HD_VIDEO_PXLFMT_YUV420;
	param.src_img.p_phy_addr[0] = src_img->p_phy_addr[0];
	param.src_img.p_phy_addr[1] = src_img->p_phy_addr[1];
	param.src_img.lineoffset[0] = src_img->lineoffset[0];
	param.src_img.lineoffset[1] = src_img->lineoffset[1];
	param.dst_img.dim.w = dst_img->dim.w;
	param.dst_img.dim.h = dst_img->dim.h;
	param.dst_img.format = HD_VIDEO_PXLFMT_YUV420;
	param.dst_img.p_phy_addr[0] = dst_img->p_phy_addr[0];
	param.dst_img.p_phy_addr[1] = dst_img->p_phy_addr[1];
	param.dst_img.lineoffset[0] = dst_img->lineoffset[0];
	param.dst_img.lineoffset[1] = dst_img->lineoffset[1];

	param.src_region.x = 0;
	param.src_region.y = 0;
	param.src_region.w = src_img->dim.w;
	param.src_region.h = src_img->dim.h;
	param.dst_region.x = 0;
	param.dst_region.y = 0;
	param.dst_region.w = dst_img->dim.w;
	param.dst_region.h = dst_img->dim.h;

	param.quality = method;

	ret = hd_gfx_scale(&param);
	if (ret != HD_OK) {
		printf("hd_gfx_scale fail=%d\n", ret);
	}

	return ret;
}

HD_RESULT nova_resize_yuv420sp_xvr(HD_GFX_IMG_BUF *dst_img, HD_GFX_IMG_BUF *src_img, HD_GFX_SCALE_QUALITY method, GFX_REGION *gfx_image)
{
	HD_RESULT ret = HD_OK;
	HD_GFX_SCALE param;
	memset(&param, 0, sizeof(HD_GFX_SCALE));

	param.src_img.dim.w            = src_img->dim.w;
	param.src_img.dim.h            = src_img->dim.h;
	param.src_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.src_img.p_phy_addr[0]    = src_img->p_phy_addr[0];
	param.src_img.p_phy_addr[1]    = src_img->p_phy_addr[1];
	param.src_img.lineoffset[0]    = src_img->lineoffset[0];
	param.src_img.lineoffset[1]    = src_img->lineoffset[1];
	param.dst_img.dim.w            = dst_img->dim.w;
	param.dst_img.dim.h            = dst_img->dim.h;
	param.dst_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.dst_img.p_phy_addr[0]    = dst_img->p_phy_addr[0];
	param.dst_img.p_phy_addr[1]    = dst_img->p_phy_addr[1];
	param.dst_img.lineoffset[0]    = dst_img->lineoffset[0];
	param.dst_img.lineoffset[1]    = dst_img->lineoffset[1];

	param.src_region.x             = 0;
	param.src_region.y             = 0;
	param.src_region.w             = src_img->dim.w;
	param.src_region.h             = src_img->dim.h;

	if (gfx_image->image_h > gfx_image->image_w) {
		param.dst_region.x         = gfx_image->offset_w;
		param.dst_region.y         = 0;
		param.dst_region.w         = ALIGN_CEIL_4(gfx_image->image_w);
		param.dst_region.h         = dst_img->dim.h;
	} else {
		param.dst_region.x         = 0;
		param.dst_region.y         = gfx_image->offset_h;
		param.dst_region.w         = dst_img->dim.w;
		param.dst_region.h         = ALIGN_CEIL_4(gfx_image->image_h);

	}
	param.quality                  = method;

	ret = hd_gfx_scale(&param);
	if (ret != HD_OK) {
		printf("hd_gfx_scale fail=%d\n", ret);
	}

	return ret;
}

HD_RESULT nova_input_letterbox(NOVA_INPUT *in_info, NOVA_MODEL *model_info, NOVA_IMG *img_info)
{
	HD_RESULT ret = HD_OK;
	UINT32 img_w = in_info->img_buf->width;
	UINT32 img_h = in_info->img_buf->height;
	UINT32 net_in_w = model_info->proc_info.in_buf[0].width;
	UINT32 net_in_h = model_info->proc_info.in_buf[0].height;
	UINT32 align_img_w = ALIGN_CEIL(img_w, GFX_W_ALIGN);
	UINT32 align_img_h = ALIGN_CEIL(img_h, GFX_H_ALIGN);
	UINT32 align_net_in_w = ALIGN_CEIL(net_in_w, GFX_W_ALIGN);
	UINT32 align_net_in_h = ALIGN_CEIL(net_in_h, GFX_H_ALIGN);
	UINT64 img_line_ofs = in_info->img_buf->line_ofs;

	MEM_PARM *scale_out_buf = &in_info->scale_img_mem;
	// printf("net_in_w = %d, net_in_h = %d\r\n",net_in_w,net_in_h);
	ret = nova_mem_alloc(scale_out_buf, "scale_out_buf", (UINT32)(net_in_w * net_in_h * 3));
	if (ret != HD_OK) {
		printf("nova_mem_alloc scale_out_buf fail=%d\n", ret);
		return HD_ERR_FAIL;
	}

	HD_GFX_IMG_BUF gfx_in = {0};
	gfx_in.dim.w = align_img_w;
	gfx_in.dim.h = align_img_h;

	// YUV420SP
	gfx_in.format = HD_VIDEO_PXLFMT_YUV420;
	gfx_in.lineoffset[0] = img_line_ofs;
	gfx_in.lineoffset[1] = img_line_ofs;

	gfx_in.p_phy_addr[0] = in_info->img_buf->pa;
	gfx_in.p_phy_addr[1] = in_info->img_buf->pa + img_line_ofs * img_h;

	// cal letter box
	FLOAT scale = MIN((FLOAT)(net_in_w) / (FLOAT)(align_img_w), (FLOAT)(net_in_h) / (FLOAT)(align_img_h));
	UINT32 letter_out_w = ALIGN_CEIL((UINT32)(scale * (FLOAT)(align_img_w)), GFX_W_ALIGN);
	UINT32 letter_out_h = ALIGN_CEIL((UINT32)(scale * (FLOAT)(align_img_h)), GFX_H_ALIGN);

	UINT32 letter_offset_w;
	UINT32 letter_offset_h;
	if (in_info->letterbox_mode == NOVA_LETTERBOX_TOP_LEFT) {
		letter_offset_h = 0;
		letter_offset_w = 0;
	} else { // NOVA_LETTERBOX_CENTER
		letter_offset_w = ALIGN_CEIL((net_in_w - letter_out_w) / 2, GFX_W_ALIGN);
		letter_offset_h = ALIGN_CEIL((net_in_h - letter_out_h) / 2, GFX_H_ALIGN);
	}

	NOVA_JPEG *jpeg = img_info->info;
	INT32 img_orig_w = nova_jpeg_get_width(jpeg);
	INT32 img_orig_h = nova_jpeg_get_height(jpeg);

	// letter box parameters
	NOVA_LETTERBOX *letterbox = &in_info->letterbox;
	letterbox->scale = scale;
	letterbox->offset_w = letter_offset_w;
	letterbox->offset_h = letter_offset_h;
	letterbox->orig_w = img_orig_w;
	letterbox->orig_h = img_orig_h; // use original value, not the ALIGN_CEIL value

	// NV12 grey(Y114,U128,V128) input padding
	// When JPEG width is not a multiple of 4, nova_set_image_buf allocated
	// extra bytes per row. Fill with grey values so GFX scaler reads correct data.
	UINT32 pad_w = align_img_w - img_w;  // extra padding bytes per Y-row
	if (pad_w > 0) {
		UINT32 y;
		UINT8 *pad_ptr = NULL;
		// Pad Y plane right side: img_h rows, each with pad_w bytes after valid data
		for (y = 0; y < img_h; y++) {
			pad_ptr = (UINT8 *)(in_info->img_buf->va) + y * img_line_ofs + img_w;
			memset(pad_ptr, 114, pad_w);
		}
		// Pad UV plane right side
		UINTPTR uv_plane_start = in_info->img_buf->va + img_line_ofs * img_h;
		for (y = 0; y < img_h / 2; y++) {
			pad_ptr = (UINT8 *)(uv_plane_start) + y * img_line_ofs + img_w;
			memset(pad_ptr, 128, pad_w);
		}
	}

	// NV12 grey(Y114,U128,V128) letterbox preset
	memset((VOID *)(scale_out_buf->va), 114, align_net_in_w * align_net_in_h);
	memset((VOID *)(scale_out_buf->va + (align_net_in_w * align_net_in_h)), 128, align_net_in_w * align_net_in_h * 1 / 2);

	hd_common_mem_flush_cache((VOID *)in_info->img_buf->va, (UINT32)(in_info->img_buf->size));
	hd_common_mem_flush_cache((VOID *)scale_out_buf->va, (UINT32)(align_net_in_w * align_net_in_h * 3 / 2));

	// scale by gfx
	HD_GFX_IMG_BUF gfx_out = {0};
#if defined(CHIP_CNN25_XVR) || defined(CHIP_CNN30_XVR)
	gfx_out.dim.w = align_net_in_w;
	gfx_out.dim.h = align_net_in_h;
	gfx_out.format = HD_VIDEO_PXLFMT_YUV420;
	gfx_out.lineoffset[0] = align_net_in_w;
	gfx_out.lineoffset[1] = align_net_in_w;
	gfx_out.p_phy_addr[0] = scale_out_buf->pa;
	gfx_out.p_phy_addr[1] = gfx_out.p_phy_addr[0] + align_net_in_w * align_net_in_h;
#else
	gfx_out.dim.w = letter_out_w;
	gfx_out.dim.h = letter_out_h;
	gfx_out.format = HD_VIDEO_PXLFMT_YUV420;
	gfx_out.lineoffset[0] = align_net_in_w;
	gfx_out.lineoffset[1] = align_net_in_w;
	gfx_out.p_phy_addr[0] = ALIGN_CEIL(scale_out_buf->pa + letter_offset_w + letter_out_w * letter_offset_h, GFX_W_ALIGN);
	gfx_out.p_phy_addr[1] = ALIGN_CEIL(scale_out_buf->pa + letter_offset_w + letter_out_w * (letter_offset_h / 2) + align_net_in_w * align_net_in_h, GFX_W_ALIGN);
#endif

	FLOAT scale_w = ((FLOAT)(gfx_in.dim.w)) / ((FLOAT)(gfx_out.dim.w));
	FLOAT scale_h = ((FLOAT)(gfx_in.dim.h)) / ((FLOAT)(gfx_out.dim.h));
	HD_GFX_SCALE_QUALITY interpolation = HD_GFX_SCALE_QUALITY_INTEGRATION;
	if (gfx_in.dim.w < net_in_w || gfx_in.dim.h < net_in_h) {
		interpolation = HD_GFX_SCALE_QUALITY_BILINEAR;
	}

	if (scale_w >= 16 || scale_h >= 16) {
		UINT32 scale_temp_w = ALIGN_CEIL(gfx_in.dim.w / 2, GFX_W_ALIGN), scale_temp_h = ALIGN_CEIL(gfx_in.dim.h / 2, GFX_H_ALIGN);
		MEM_PARM scale_temp_buf = {0};
		ret = nova_mem_alloc(&scale_temp_buf, "scale_temp_buf", (UINT32)(scale_temp_w * scale_temp_h * 3 / 2));
		if (ret != HD_OK) {
			printf("nova_mem_alloc scale_temp_buf fail=%d\n", ret);
			return HD_ERR_FAIL;
		}
		HD_GFX_IMG_BUF gfx_temp = {0};
		gfx_temp.dim.w = scale_temp_w;
		gfx_temp.dim.h = scale_temp_h;
		gfx_temp.format = HD_VIDEO_PXLFMT_YUV420;
		gfx_temp.lineoffset[0] = scale_temp_w;
		gfx_temp.lineoffset[1] = scale_temp_w;
		gfx_temp.p_phy_addr[0] = scale_temp_buf.pa;
		gfx_temp.p_phy_addr[1] = scale_temp_buf.pa + scale_temp_w * scale_temp_h;
		ret = nova_resize_yuv420sp(&gfx_temp, &gfx_in, interpolation);
		if (ret != HD_OK) {
			printf("nova_resize_yuv420sp gfx_in fail=%d\n", ret);
			nova_mem_free(&scale_temp_buf);
			return ret;
		}
		ret = nova_resize_yuv420sp(&gfx_out, &gfx_temp, interpolation);
		if (ret != HD_OK) {
			printf("nova_resize_yuv420sp gfx_temp fail=%d\n", ret);
			nova_mem_free(&scale_temp_buf);
			return ret;
		}
		nova_mem_free(&scale_temp_buf);
	} else {
#if defined(CHIP_CNN25_XVR) || defined(CHIP_CNN30_XVR)
		GFX_REGION gfx_image;
		gfx_image.image_w = letter_out_w;
		gfx_image.image_h = letter_out_h;
		gfx_image.offset_w = letter_offset_w;
		gfx_image.offset_h = letter_offset_h;
		ret = nova_resize_yuv420sp_xvr(&gfx_out, &gfx_in, interpolation, &gfx_image);
		if (ret != HD_OK) {
			printf("ai_resize_img_yuv420sp fail=%d\n", ret);
			return ret;
		}
#else
		ret = nova_resize_yuv420sp(&gfx_out, &gfx_in, interpolation);
		if (ret != HD_OK) {
			printf("nova_resize_yuv420sp <16 gfx_in fail=%d\n", ret);
			printf("gfx_in w,h = %u,%u\n", gfx_in.dim.w, gfx_in.dim.h);
			return ret;
		}
#endif
	}

	in_info->img_buf[0].width       = align_net_in_w;
	in_info->img_buf[0].height      = align_net_in_h;
	in_info->img_buf[0].line_ofs    = gfx_out.lineoffset[0];
	in_info->img_buf[0].pa          = scale_out_buf->pa;
	in_info->img_buf[0].va          = scale_out_buf->va;
	in_info->img_buf[0].size        = align_net_in_w * align_net_in_h * 3 / 2;

	return ret;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT nova_input_init(NOVA_INPUT *in_info, NOVA_MODEL *model_info, NOVA_IMG *img_info)
{
	if (!in_info || !model_info || !img_info) {
		return HD_ERR_INV;
	}

	if (img_info->fmt != NOVA_IMG_JPG) {
		return HD_ERR_NOT_SUPPORT;
	}

	NOVA_JPEG *jpeg = img_info->info;
	INT32 width = nova_jpeg_get_width(jpeg);
	INT32 height = nova_jpeg_get_height(jpeg);
	INT32 color = nova_jpeg_get_colorspace(jpeg);
	INT32 components = nova_jpeg_get_components(jpeg);
	printf("jpeg width x height: %d x %d, color: %d, components: %d\n", width, height, color, components);

	HD_VIDEO_PXLFMT dst_fmt = model_info->proc_info.in_buf[0].fmt;
	in_info->img_buf = (AI_BUF *)calloc(1, sizeof(AI_BUF));
	nova_jpeg_readimg(jpeg, dst_fmt, in_info->img_buf, &in_info->img_mem, &in_info->temp_mem);
	hd_common_mem_flush_cache((VOID *)in_info->img_buf[0].va, in_info->img_buf[0].size);

	if (in_info->letterbox_mode != NOVA_LETTERBOX_NONE) {
		nova_input_letterbox(in_info, model_info, img_info);
	}

	return HD_OK;
}

HD_RESULT nova_input_uninit(NOVA_INPUT *in_info)
{
	if (!in_info) {
		return HD_ERR_INV;
	}

	if (in_info->img_buf) {
		free(in_info->img_buf);
	}

	nova_mem_free(&in_info->img_mem);
	nova_mem_free(&in_info->temp_mem);
	nova_mem_free(&in_info->scale_img_mem);

	return HD_OK;
}
