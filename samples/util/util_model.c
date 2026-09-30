/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <string.h>
#include "kwrap/cpu.h"
#include "kwrap/debug.h"
#include "util_img.h"
#include "util_model.h"
#if defined(CHIP_CNN25_IPC) || defined(CHIP_CNN25_XVR)
extern HD_RESULT vendor_ai_cpu_util_fixed2float(VOID *in_data, HD_VIDEO_PXLFMT in_fmt, FLOAT *out_data, FLOAT out_scale_ratio, INT32 data_size);
#endif

/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
#define GFX_W_ALIGN             4                   ///< width align of GFX engine limitation
#define GFX_H_ALIGN             2                   ///< height align of GFX engine limitation
#define CPU_CACHELINE_ALIGN     VOS_ALIGN_BYTES     ///< buffer align of CPU cache line

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/
static UINT32 load_bin_file_to_mem(const CHAR *filename, MEM_PARM *mem_parm)
{
	FILE *fd;
	UINT32 size = 0;

	fd = fopen(filename, "rb");
	if (!fd) {
		printf("cannot read %s\r\n", filename);
		return -1;
	}

	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	if (size < 0) {
		printf("getting %s size failed\r\n", filename);
	} else if (fread((VOID *)mem_parm->va, 1, size, fd) != size) {
		printf("read size < %lu\r\n", (unsigned long)size);
		size = -1;
	}
	mem_parm->size = size;

	if (fd) {
		fclose(fd);
	}
	return size;
}

static UINT32 get_bin_size(const CHAR *filename)
{
	FILE *bin_fd;
	UINT32 bin_size = 0;

	bin_fd = fopen(filename, "rb");
	if (!bin_fd) {
		printf("get mdoel bin (%s) size fail\n", filename);
		return (-1);
	}

	fseek(bin_fd, 0, SEEK_END);
	bin_size = ftell(bin_fd);
	fseek(bin_fd, 0, SEEK_SET);
	fclose(bin_fd);

	return bin_size;
}

static HD_RESULT alloc_mem_load_bin(MEM_PARM *p_mem, CHAR *mem_name, const CHAR *bin_path)
{
	HD_RESULT ret = HD_OK;
	UINT32 bin_size = get_bin_size(bin_path);

	ret = nova_mem_alloc(p_mem, mem_name, bin_size);
	if (ret != HD_OK) {
		printf("alloc %s mem fail\r\n", mem_name);
		return HD_ERR_FAIL;
	}
	load_bin_file_to_mem(bin_path, p_mem);

	return ret;
}

static HD_RESULT mem_init(VOID)
{
	HD_RESULT ret = HD_OK;
#if defined(CHIP_CNN25_IPC) || defined(CHIP_CNN30_IPC) || defined(CHIP_CNN30_XVR) || defined(CHIP_NPU)
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
#if defined(_NVT_NVR_SDK_) // NT98635
	int sys_fd;
	struct nvtmem_hdal_base sys_hdal;
	uintptr_t hdal_start_addr0, hdal_start_addr1;

	sys_fd = open("/dev/nvtmem0", O_RDWR);
	if (sys_fd < 0) {
		printf("Error: cannot open /dev/nvtmem0 device.\n");
		exit(0);
	}
	if (ioctl(sys_fd, NVTMEM_GET_DTS_HDAL_BASE, &sys_hdal) < 0) {
		printf("PCIE_SYS_IOC_HDALBASE! \n");
		close(sys_fd);
		exit(0);
	}
	close(sys_fd);

	/* init ddr0 user_blk */
	hdal_start_addr0 = sys_hdal.base[0];
	mem_cfg.pool_info[0].start_addr = hdal_start_addr0;
	mem_cfg.pool_info[0].blk_cnt = 1;
	mem_cfg.pool_info[0].blk_size = 200 * 1024 * 1024;
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_USER_BLK;
	mem_cfg.pool_info[0].ddr_id = sys_hdal.ddr_id[0];
	printf("create ddr%d: hdal_memory(%#lx, %ldKB), usr_blk(%#lx, %dKB)\n", mem_cfg.pool_info[0].ddr_id,
			hdal_start_addr0, sys_hdal.size[0] / 1024, mem_cfg.pool_info[0].start_addr,
			mem_cfg.pool_info[0].blk_size * mem_cfg.pool_info[0].blk_cnt / 1024);

	/* init ddr1 user_blk, if ddr1 is exist */
	if (sys_hdal.size[1] != 0) {
		hdal_start_addr1 = sys_hdal.base[1];
		mem_cfg.pool_info[1].start_addr = hdal_start_addr1;
		mem_cfg.pool_info[1].blk_cnt = 1;
		mem_cfg.pool_info[1].blk_size = 200 * 1024 * 1024;
		mem_cfg.pool_info[1].type = HD_COMMON_MEM_USER_BLK;
		mem_cfg.pool_info[1].ddr_id = sys_hdal.ddr_id[1];
		printf("create ddr%d: hdal_memory(%#lx, %ldKB) usr_blk(%#lx, %dKB)\n", mem_cfg.pool_info[1].ddr_id,
				hdal_start_addr1, sys_hdal.size[1] / 1024, mem_cfg.pool_info[1].start_addr,
				mem_cfg.pool_info[1].blk_size * mem_cfg.pool_info[1].blk_cnt / 1024);
	} else {
		printf("create ddr1: hdal_memory(%#lx, %ldKB) is not exist\n", sys_hdal.base[1], sys_hdal.size[1] / 1024);
	}
	usleep(30000); // wait for printf completely
#endif
	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("ERR: hd_common_mem_init err: %d\r\n", ret);
	}
#elif defined(CHIP_CNN25_XVR)
	ret = vendor_common_clear_pool_blk(HD_COMMON_MEM_USER_BLK, 0);
	if (ret != HD_OK) {
		printf("ERR: vendor_common_clear_pool_blk HD_COMMON_MEM_USER_BLK fail=%d\n", ret);
		return ret;
	}
	ret = vendor_common_clear_pool_blk(HD_COMMON_MEM_CNN_POOL, 0);
	if (ret != HD_OK) {
		printf("ERR: vendor_common_clear_pool_blk fail=%d\n", ret);
		return ret;
	}
#endif
	return ret;
}

static HD_RESULT mem_uninit(VOID)
{
	HD_RESULT ret = HD_OK;
#if defined(CHIP_CNN25_IPC) || defined(CHIP_CNN30_IPC) || defined(CHIP_CNN30_XVR) || defined(CHIP_NPU)
	ret = hd_common_mem_uninit();
#endif
	return ret;
}

static HD_RESULT ai_init(VOID)
{
	HD_RESULT ret = HD_OK;
#if defined(CHIP_CNN30_IPC) || defined(CHIP_CNN30_XVR)
	VENDOR_AI3_DEV_CFG dev_cfg = {0};
	ret = vendor_ai3_dev_init(&dev_cfg);
	if (ret != HD_OK) {
		printf("ERR: vendor_ai3_dev_init fail=%d\n", ret);
		return ret;
	}

#if NOVA_PRINT_AI_VER
	VENDOR_AI3_VER ai3_ver = {0};
	ret = vendor_ai3_dev_get(VENDOR_AI3_CFG_VER, &ai3_ver);
	if (ret != HD_OK) {
		printf("ERR: vendor_ai3_dev_get(CFG_VER) fail=%d\n", ret);
		return ret;
	}
	printf("vendor_ai version = %s\r\n", ai3_ver.vendor_ai_impl_version);
	printf("kflow_ai  version = %s\r\n", ai3_ver.kflow_ai_impl_version);
	printf("kdrv_ai   version = %s\r\n", ai3_ver.kdrv_ai_impl_version);
	printf("\n");
#endif
#elif defined(CHIP_CNN25_IPC) || defined(CHIP_CNN25_XVR)
	// config extend engine plugin, process scheduler
	{
		UINT32 schd = VENDOR_AI_PROC_SCHD_FAIR;
		vendor_ai_cfg_set(VENDOR_AI_CFG_PLUGIN_ENGINE, vendor_ai_cpu1_get_engine());
		vendor_ai_cfg_set(VENDOR_AI_CFG_PROC_SCHD, &schd);
	}
	ret = vendor_ai_init();
	if (ret != HD_OK) {
		printf("ERR: vendor_ai_init fail=%d\n", ret);
		return ret;
	}
#if NOVA_PRINT_AI_VER
	// get config of sdk version
	VENDOR_AI_NET_CFG_IMPL_VERSION sdk_ver;
	vendor_ai_cfg_get(VENDOR_AI_CFG_IMPL_VERSION, &sdk_ver);
	printf("vendor_ai version = %s\r\n", sdk_ver.vendor_ai_impl_version);
	printf("kflow_ai  version = %s\r\n", sdk_ver.kflow_ai_impl_version);
	printf("kdrv_ai   version = %s\r\n", sdk_ver.kdrv_ai_impl_version);
	printf("\n");
#endif
#endif
	return ret;
}

static HD_RESULT ai_uninit(VOID)
{
	HD_RESULT ret = HD_OK;
#if defined(CHIP_CNN30_IPC) || defined(CHIP_CNN30_XVR)
	ret = vendor_ai3_dev_uninit();
	if (ret != HD_OK) {
		printf("ERR: vendor_ai3_dev_uninit fail=%d\n", ret);
	}
#elif defined(CHIP_CNN25_IPC) || defined(CHIP_CNN25_XVR)
	ret = vendor_ai_uninit();
	if (ret != HD_OK) {
		printf("ERR: vendor_ai_uninit fail=%d\n", ret);
	}
#endif
	return ret;
}

#if defined(CHIP_CNN30_IPC) || defined(CHIP_CNN30_XVR)
#if NOVA_PRINT_MODEL_INFO
static VOID network_io_buf_info_dump_print_info(VENDOR_AI3_BUF_INFO *buf_info)
{
	printf("  +- name    = %s\r\n", buf_info->name);
	printf("  +- fmt     = 0x%08x\r\n", (unsigned int)buf_info->fmt);
	printf("    +- bits  = %d\r\n", HD_VIDEO_PXLFMT_BITS(buf_info->fmt));
	printf("    +- sign  = %d\r\n", HD_VIDEO_PXLFMT_SIGN(buf_info->fmt));
	printf("    +- int   = %d\r\n", HD_VIDEO_PXLFMT_INT(buf_info->fmt));
	printf("    +- frac  = %d\r\n", HD_VIDEO_PXLFMT_FRAC(buf_info->fmt));
	printf("  +- width   = %d\r\n", (int)buf_info->width);
	printf("  +- height  = %d\r\n", (int)buf_info->height);
	printf("  +- channel = %d\r\n", (int)buf_info->channel);
	printf("  +- batch   = %d\r\n", (int)buf_info->batch_num);
	printf("  +- time    = %d\r\n", (int)buf_info->time);
	printf("  +- layout  = %s\r\n", buf_info->layout);
	printf("\r\n");
}

static HD_RESULT network_io_buf_info_dump(VENDOR_AI3_CFG_BUF *model_buf)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AI3_IO_BUF_INFO ai_io_info = {0};

	ai_io_info.model_buf.pa   = model_buf->pa;
	ai_io_info.model_buf.va   = model_buf->va;
	ai_io_info.model_buf.size = model_buf->size;

	// query in/out buffer count
	ret = vendor_ai3_dev_get(VENDOR_AI3_CFG_IO_CNT, &ai_io_info); // get in_buf_cnt & out_buf_cnt
	if (ret != HD_OK) {
		printf("ERR: vendor_ai3_dev_get(VENDOR_AI3_CFG_IO_CNT) fail=%d\n", ret);
		return HD_ERR_FAIL;
	}
	printf("in_buf_cnt = %d, out_buf_cnt = %d\r\n", (int)ai_io_info.in_buf_cnt, (int)ai_io_info.out_buf_cnt);

	// alloc context for in/out buffer info
	ai_io_info.in_buf_info  = (VENDOR_AI3_BUF_INFO *)malloc(sizeof(VENDOR_AI3_BUF_INFO) * ai_io_info.in_buf_cnt);
	ai_io_info.out_buf_info = (VENDOR_AI3_BUF_INFO *)malloc(sizeof(VENDOR_AI3_BUF_INFO) * ai_io_info.out_buf_cnt);
	if (ai_io_info.in_buf_info == NULL || ai_io_info.out_buf_info == NULL) {
		printf("ERR: malloc in_buf_info/out_buf_info failed...\n");
		ret = HD_ERR_FAIL;
		goto free_context;
	}

	// query in/out buffer info
	vendor_ai3_dev_get(VENDOR_AI3_CFG_IO_INFO, &ai_io_info); // get in_buf_info & out_buf_info
	if (ret != HD_OK) {
		printf("ERR: vendor_ai3_dev_get(VENDOR_AI3_CFG_IO_INFO) fail=%d\n", ret);
		ret = HD_ERR_FAIL;
		goto free_context;
	}

	for (UINT32 i = 0; i < ai_io_info.in_buf_cnt; i++) {
		printf("input [%d] =>\r\n", (int)i);
		network_io_buf_info_dump_print_info(&ai_io_info.in_buf_info[i]);
	}
	for (UINT32 i = 0; i < ai_io_info.out_buf_cnt; i++) {
		printf("output [%d] =>\r\n", (int)i);
		network_io_buf_info_dump_print_info(&ai_io_info.out_buf_info[i]);
	}

free_context:
	if (ai_io_info.in_buf_info) {
		free(ai_io_info.in_buf_info);
	}
	if (ai_io_info.out_buf_info) {
		free(ai_io_info.out_buf_info);
	}
	return ret;
}
#endif // NOVA_PRINT_MODEL_INFO
#endif

static HD_RESULT model_init(NET_PROC *p_net, CHAR *name)
{
	HD_RESULT ret = HD_OK;
	UINT32 i = 0;

	ret = alloc_mem_load_bin(&p_net->proc_mem, name, p_net->model_filename);
	if (ret != HD_OK) {
		printf("ERR: alloc_mem_load_bin fail=%d\n", ret);
		return HD_ERR_FAIL;
	}
#if defined(CHIP_CNN30_IPC) || defined(CHIP_CNN30_XVR)
#if NOVA_PRINT_MODEL_INFO
	VENDOR_AI3_TOOL_VER ai_model_ver = {0};
	ai_model_ver.model_buf.pa   = p_net->proc_mem.pa;
	ai_model_ver.model_buf.va   = p_net->proc_mem.va;
	ai_model_ver.model_buf.size = p_net->proc_mem.size;
	ret = vendor_ai3_dev_get(VENDOR_AI3_CFG_TOOLVER, &ai_model_ver); // get model version
	if (ret != HD_OK) {
		printf("ERR: vendor_ai3_dev_get(VENDOR_AI3_CFG_TOOLVER) fail=%d\n", ret);
		return HD_ERR_FAIL;
	}
	printf("tool gen version = %s\r\n", ai_model_ver.tool_version);

	ret = network_io_buf_info_dump((VENDOR_AI3_CFG_BUF *)&p_net->proc_mem);
	if (ret != HD_OK) {
		printf("ERR: network_io_buf_info_dump() fail=%d\n", ret);
		return HD_ERR_FAIL;
	}
#endif

	// query model info for WORKBUF/RONLYBUF size , then alloc WORKBUF/RONLYBUF
	VENDOR_AI3_MODEL_INFO model_info = {0};
	model_info.model_buf.pa   = p_net->proc_mem.pa;
	model_info.model_buf.va   = p_net->proc_mem.va;
	model_info.model_buf.size = p_net->proc_mem.size;
	ret = vendor_ai3_dev_get(VENDOR_AI3_CFG_MODEL_INFO, &model_info);
	if (ret != HD_OK) {
		printf("ERR: vendor_ai3_dev_get(MODEL_INFO) fail=%d\n", ret);
		return HD_ERR_FAIL;
	}
#if NOVA_PRINT_MODEL_INFO
	printf("model_info get => workbuf size = %d, ronlybuf size = %d\r\n", model_info.proc_mem.buf[AI3_PROC_BUF_WORKBUF].size, model_info.proc_mem.buf[AI3_PROC_BUF_RONLYBUF].size);
#endif

	CHAR mem_name[23] ;
	snprintf(mem_name, 23, "%s_intl_mem", name);
	// alloc WORKBUF/RONLYBUF
	ret = nova_mem_alloc(&p_net->intl_mem, mem_name, model_info.proc_mem.buf[AI3_PROC_BUF_RONLYBUF].size);
	if (ret != HD_OK) {
		printf("ERR: alloc ronlybuf fail=%d\n", ret);
		return HD_ERR_FAIL;
	}
	snprintf(mem_name, 23, "%s_io_mem", name);
	ret = nova_mem_alloc(&p_net->io_mem, mem_name, model_info.proc_mem.buf[AI3_PROC_BUF_WORKBUF].size);
	if (ret != HD_OK) {
		printf("ERR: alloc workbuf fail=%d\n", ret);
		return HD_ERR_FAIL;
	}

	VENDOR_AI3_PROC_CFG proc_cfg = {0};
	proc_cfg.model_buf.pa   = p_net->proc_mem.pa;
	proc_cfg.model_buf.va   = p_net->proc_mem.va;
	proc_cfg.model_buf.size = p_net->proc_mem.size;
	proc_cfg.proc_mem.buf[AI3_PROC_BUF_RONLYBUF].pa   = p_net->intl_mem.pa;
	proc_cfg.proc_mem.buf[AI3_PROC_BUF_RONLYBUF].va   = p_net->intl_mem.va;
	proc_cfg.proc_mem.buf[AI3_PROC_BUF_RONLYBUF].size = p_net->intl_mem.size;
	proc_cfg.proc_mem.buf[AI3_PROC_BUF_WORKBUF].pa   = p_net->io_mem.pa;
	proc_cfg.proc_mem.buf[AI3_PROC_BUF_WORKBUF].va   = p_net->io_mem.va;
	proc_cfg.proc_mem.buf[AI3_PROC_BUF_WORKBUF].size = p_net->io_mem.size;
	proc_cfg.plugin[AI3_PLUGIN_CPU] = vendor_ai_cpu1_get_engine();
	ret = vendor_ai3_net_open(&p_net->proc_id, &proc_cfg, &p_net->net_info);
	if (ret != HD_OK) {
		printf("ERR: proc_id(%u) vendor_ai3_net_open() fail=%d\n", p_net->proc_id, ret);
		return HD_ERR_FAIL;
	}
	DBG_IND("open success => get proc_id (%u), input: %u, output: %u\r\n", p_net->proc_id, p_net->net_info.in_buf_cnt, p_net->net_info.out_buf_cnt);

	// get out path list
	p_net->out_mem = (MEM_PARM *)calloc(p_net->net_info.out_buf_cnt, sizeof(MEM_PARM));
	p_net->float_out = (FLOAT **)calloc(p_net->net_info.out_buf_cnt, sizeof(FLOAT *));
	p_net->out_buf = (AI_BUF *)calloc(p_net->net_info.out_buf_cnt, sizeof(AI_BUF));
	for (i = 0; i < p_net->net_info.out_buf_cnt; i++) {
		ret = vendor_ai3_net_get(p_net->proc_id, p_net->net_info.out_path_list[i], &p_net->out_buf[i]);
		if (HD_OK != ret) {
			printf("ERR: proc_id(%u) get out buf fail, i(%d), out_path(0x%lx)\n", p_net->proc_id, i, (unsigned long)p_net->net_info.out_path_list[i]);
			return HD_ERR_FAIL;
		}
		// allocate out buf
		CHAR mem_name[50];
		snprintf(mem_name, 50, "%s_%u_output_buf", name, i);
		ret = nova_mem_alloc(&p_net->out_mem[i], mem_name, p_net->out_buf[i].size);
		if (ret != HD_OK) {
			printf("ERR: proc_id(%lu) alloc ai_out_buf fail\r\n", (unsigned long)p_net->proc_id);
			return HD_ERR_FAIL;
		}
		DBG_IND("alloc_outbuf %d: pa = 0x%lx, va = 0x%lx, size = %u\n", i, (unsigned long)p_net->out_mem[i].pa, (unsigned long)p_net->out_mem[i].va, p_net->out_mem[i].size);

		UINT32 length = p_net->out_buf[i].width * p_net->out_buf[i].height * p_net->out_buf[i].channel * p_net->out_buf[i].batch_num;
		p_net->float_out[i] = (FLOAT *)calloc(length, sizeof(FLOAT));
	}

	p_net->in_mem = (MEM_PARM *)calloc(p_net->net_info.in_buf_cnt, sizeof(MEM_PARM));
	p_net->in_buf = (AI_BUF *)calloc(p_net->net_info.in_buf_cnt, sizeof(AI_BUF));
	for (i = 0; i < p_net->net_info.in_buf_cnt; i++) {
		ret = vendor_ai3_net_get(p_net->proc_id, p_net->net_info.in_path_list[i], &p_net->in_buf[i]);
		if (HD_OK != ret) {
			printf("ERR: proc_id(%u) get in buf fail, i(%d), in_path(0x%lx)\n", p_net->proc_id, i, (unsigned long)p_net->net_info.in_path_list[i]);
			return HD_ERR_FAIL;
		}
#if 0
		// do not allocate in buf here, because input size may be inappropriate
		CHAR mem_name[50];
		snprintf(mem_name, 23, "%s_%u_input_buf", name, i);
		ret = hdal_mem_alloc(&p_net->in_mem[i], mem_name, p_net->in_buf[i].batch_ofs * p_net->in_buf[i].batch_num);
		if (ret != HD_OK) {
			printf("ERR: proc_id(%lu) alloc ai_in_buf fail\r\n", p_net->proc_id);
			return HD_ERR_FAIL;
		}
		printf("alloc_inbuf %d: pa = 0x%lx, va = 0x%lx, size = %u\n", i, p_net->in_mem[i].pa, p_net->in_mem[i].va, p_net->in_mem[i].size);
#endif
	}

	ret = vendor_ai3_net_start(p_net->proc_id);
	if (HD_OK != ret) {
		printf("ERR: proc_id(%u) vendor_ai3_net_start fail !!\n", p_net->proc_id);
	}

	UINT32 job_priority = VENDOR_AI_JOB_PRI(0);
	UINT32 core_mask = VENDOR_AI_CORE_MASK_DEFAULT;
	vendor_ai3_net_set(p_net->proc_id, VENDOR_AI3_NET_PARAM_JOB_PRI, &job_priority);
	vendor_ai3_net_set(p_net->proc_id, VENDOR_AI3_NET_PARAM_CORE_MASK, &core_mask);
#elif defined(CHIP_CNN25_IPC) || defined(CHIP_CNN25_XVR)
	// set buf opt
	{
		VENDOR_AI_NET_CFG_BUF_OPT cfg_buf_opt = {0};
		cfg_buf_opt.method = VENDOR_AI_NET_BUF_OPT_SHRINK;
		cfg_buf_opt.ddr_id = DDR_ID0;
		vendor_ai_net_set(p_net->proc_id, VENDOR_AI_NET_PARAM_CFG_BUF_OPT, &cfg_buf_opt);
	}
	// set job opt
	{
		VENDOR_AI_NET_CFG_JOB_OPT cfg_job_opt = {0};
		cfg_job_opt.method = VENDOR_AI_NET_JOB_OPT_LINEAR;
		cfg_job_opt.wait_ms = -1;
		cfg_job_opt.schd_parm = VENDOR_AI_FAIR_CORE_ALL; //FAIR dispatch to ALL core
		vendor_ai_net_set(p_net->proc_id, VENDOR_AI_NET_PARAM_CFG_JOB_OPT, &cfg_job_opt);
	}
	vendor_ai_net_set(p_net->proc_id, VENDOR_AI_NET_PARAM_CFG_MODEL, (VENDOR_AI_NET_CFG_MODEL *)&p_net->proc_mem);
	// open
	vendor_ai_net_open(p_net->proc_id);
	// alloc_io_buf
	VENDOR_AI_NET_CFG_WORKBUF wbuf = {0};
	ret = vendor_ai_net_get(p_net->proc_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("ERR: proc_id(%u) get VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", p_net->proc_id);
		return HD_ERR_FAIL;
	}
	ret = nova_mem_alloc(&p_net->io_mem, "ai_io_buf", wbuf.size);
	if (ret != HD_OK) {
		printf("ERR: proc_id(%u) alloc ai_io_buf fail\r\n", p_net->proc_id);
		return HD_ERR_FAIL;
	}
	wbuf.pa = p_net->io_mem.pa;
	wbuf.va = p_net->io_mem.va;
	wbuf.size = p_net->io_mem.size;
	ret = vendor_ai_net_set(p_net->proc_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("ERR: proc_id(%u) set VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", p_net->proc_id);
		return HD_ERR_FAIL;
	}
#if NOVA_PRINT_MODEL_INFO
	printf("alloc_io_buf: work buf, pa = %#lx, va = %#lx, size = %u\r\n", (unsigned long)wbuf.pa, (unsigned long)wbuf.va, wbuf.size);
#endif
	ret = vendor_ai_net_start(p_net->proc_id);
	if (HD_OK != ret) {
		printf("ERR: proc_id(%u) start fail !!\n", p_net->proc_id);
		return HD_ERR_FAIL;
	}
	ret = vendor_ai_net_get(p_net->proc_id, VENDOR_AI_NET_PARAM_INFO, &p_net->net_info);
	if (HD_OK != ret) {
		printf("ERR: proc_id(%u) get VENDOR_AI_NET_PARAM_INFO fail(%d) !!\n", p_net->proc_id, ret);
		return HD_ERR_FAIL;
	}
	// get out path list
	p_net->out_mem = (MEM_PARM *)calloc(p_net->net_info.out_buf_cnt, sizeof(MEM_PARM));
	p_net->float_out = (FLOAT **)calloc(p_net->net_info.out_buf_cnt, sizeof(FLOAT *));
	p_net->out_buf = (AI_BUF *)calloc(p_net->net_info.out_buf_cnt, sizeof(AI_BUF));
	p_net->out_path_list = (UINT32 *)calloc(p_net->net_info.out_buf_cnt, sizeof(UINT32));
	ret = vendor_ai_net_get(p_net->proc_id, VENDOR_AI_NET_PARAM_OUT_PATH_LIST, p_net->out_path_list);
	if (HD_OK != ret) {
		printf("ERR: proc_id(%u) get out_path_list fail\n", p_net->proc_id);
		return HD_ERR_FAIL;
	}
	for (i = 0; i < p_net->net_info.out_buf_cnt; i++) {
		ret = vendor_ai_net_get(p_net->proc_id, p_net->out_path_list[i], &p_net->out_buf[i]);
		if (HD_OK != ret) {
			printf("ERR: proc_id(%u) get out buf fail, i(%d), out_path(0x%lx)\n", p_net->proc_id, i, (unsigned long)p_net->out_path_list[i]);
			return HD_ERR_FAIL;
		}
		// allocate out buf
		CHAR mem_name[50];
		snprintf(mem_name, 50, "%s_%u_output_buf", name, (unsigned int)i);
		UINT32 mem_size = ALIGN_CEIL_64(p_net->out_buf[i].batch_ofs * p_net->out_buf[i].batch_num);
		ret = nova_mem_alloc(&p_net->out_mem[i], mem_name, mem_size);
		if (ret != HD_OK) {
			printf("ERR: proc_id(%lu) alloc ai_out_buf fail\r\n", (unsigned long)p_net->proc_id);
			return HD_ERR_FAIL;
		}
#if NOVA_PRINT_MODEL_INFO
		printf("alloc_outbuf %d: pa = 0x%lx, va = 0x%lx, size = %u\n", i, (unsigned long)p_net->out_mem[i].pa, (unsigned long)p_net->out_mem[i].va, p_net->out_mem[i].size);
#endif
		UINT32 length = p_net->out_buf[i].width * p_net->out_buf[i].height * p_net->out_buf[i].channel * p_net->out_buf[i].batch_num;
		p_net->float_out[i] = (FLOAT *)calloc(length, sizeof(FLOAT));
	}

	p_net->in_mem = (MEM_PARM *)calloc(p_net->net_info.in_buf_cnt, sizeof(MEM_PARM));
	p_net->in_buf = (VENDOR_AI_BUF *)calloc(p_net->net_info.in_buf_cnt, sizeof(VENDOR_AI_BUF));
	p_net->in_path_list = (UINT32 *)calloc(p_net->net_info.in_buf_cnt, sizeof(UINT32));
	ret = vendor_ai_net_get(p_net->proc_id, VENDOR_AI_NET_PARAM_IN_PATH_LIST, p_net->in_path_list);
	if (HD_OK != ret) {
		printf("ERR: proc_id(%u) get in_path_list fail\n", p_net->proc_id);
		return HD_ERR_FAIL;
	}

	VENDOR_AI_BUF in_buf_shape = {0};
	for (i = 0; i < p_net->net_info.in_buf_cnt; i++) {
		ret = vendor_ai_net_get(p_net->proc_id, p_net->in_path_list[i], &p_net->in_buf[i]);
		if (HD_OK != ret) {
			printf("ERR: proc_id(%u) get in buf fail, i(%d), in_path(0x%lx)\n", p_net->proc_id, i, (unsigned long)p_net->in_path_list[i]);
			return HD_ERR_FAIL;
		}
		ret = vendor_ai_net_get(p_net->proc_id, VENDOR_AI_NET_PARAM_OUT(0, i), &in_buf_shape);
		p_net->in_buf[i].width = in_buf_shape.width;
		p_net->in_buf[i].height = in_buf_shape.height;
#if 0
		// do not allocate in buf here, because input size may be inappropriate
		CHAR mem_name[50];
		snprintf(mem_name, 23, "%s_%u_input_buf", name, i);
		ret = hdal_mem_alloc(&p_net->in_mem[i], mem_name, p_net->in_buf[i].batch_ofs * p_net->in_buf[i].batch_num);
		if (ret != HD_OK) {
			printf("ERR: proc_id(%lu) alloc ai_in_buf fail\r\n", p_net->proc_id);
			return HD_ERR_FAIL;
		}
		printf("alloc_inbuf %d: pa = 0x%lx, va = 0x%lx, size = %u\n", i, p_net->in_mem[i].pa, p_net->in_mem[i].va, p_net->in_mem[i].size);
#endif
	}
#endif
	return ret;
}

static HD_RESULT model_uninit(NET_PROC *p_net)
{
	HD_RESULT ret = HD_OK;
	UINT32 i = 0;
#if defined(CHIP_CNN30_IPC) || defined(CHIP_CNN30_XVR)
	ret = vendor_ai3_net_stop(p_net->proc_id);
	if (HD_OK != ret) {
		printf("ERR: proc_id(%u) vendor_ai3_net_stop fail !!\n", p_net->proc_id);
	}
	ret = vendor_ai3_net_close(p_net->proc_id);
	if (ret != HD_OK) {
		printf("ERR: proc_id(%u) vendor_ai3_net_close fail=%d\n", p_net->proc_id, ret);
	}
#elif defined(CHIP_CNN25_IPC) || defined(CHIP_CNN25_XVR)
	ret = vendor_ai_net_stop(p_net->proc_id);
	if (HD_OK != ret) {
		printf("ERR: proc_id(%u) vendor_ai_net_stop fail !!\n", p_net->proc_id);
	}
	ret = vendor_ai_net_close(p_net->proc_id);
	if (ret != HD_OK) {
		printf("ERR: proc_id(%u) vendor_ai_net_close fail=%d\n", p_net->proc_id, ret);
	}
	if (p_net->in_path_list) {
		free(p_net->in_path_list);
	}
	if (p_net->out_path_list) {
		free(p_net->out_path_list);
	}
#endif
	if (p_net->intl_mem.pa && p_net->intl_mem.va) {
		nova_mem_free(&p_net->intl_mem);
	}
	if (p_net->io_mem.pa && p_net->io_mem.va) {
		nova_mem_free(&p_net->io_mem);
	}
	if (p_net->proc_mem.pa && p_net->proc_mem.va) {
		nova_mem_free(&p_net->proc_mem);
	}
	for (i = 0; i < p_net->net_info.out_buf_cnt; i++) {
		if (p_net->out_mem[i].pa && p_net->out_mem[i].va) {
			nova_mem_free(&p_net->out_mem[i]);
		}
		if (p_net->float_out[i]) {
			free(p_net->float_out[i]);
		}
	}

	for (i = 0; i < p_net->net_info.in_buf_cnt; i++) {
		if (p_net->in_mem[i].pa && p_net->in_mem[i].va) {
			nova_mem_free(&p_net->in_mem[i]);
		}
	}
	if (p_net->in_mem) {
		free(p_net->in_mem);
	}
	if (p_net->out_mem) {
		free(p_net->out_mem);
	}
	if (p_net->in_buf) {
		free(p_net->in_buf);
	}
	if (p_net->out_buf) {
		free(p_net->out_buf);
	}
	if (p_net->float_out) {
		free(p_net->float_out);
	}

	return ret;
}

static HD_RESULT _load_label(UINTPTR addr, UINT32 line_len, const CHAR *filename)
{
	FILE *fd;
	CHAR *p_line = (CHAR *)addr;

	fd = fopen(filename, "r");
	if (!fd) {
		printf("ERR: load label(%s) fail\r\n", filename);
		return HD_ERR_NG;
	}

	while (fgets(p_line, line_len, fd) != NULL) {
		p_line[strlen(p_line) - 1] = '\0'; // remove newline character
		p_line += line_len;
	}

	if (fd) {
		fclose(fd);
	}

	printf("load label(%s) ok\r\n", filename);
	return HD_OK;
}

static VOID ai_net_sort_result(FLOAT *p_in, INT32 len, INT32 *p_idx, INT32 top_n, AI_NET_OUTPUT_CLASS *p_classes)
{
	INT32 tmp;
	INT32 i, j;

	for (i = 0; i < len; i++) {
		p_idx[i] = i;
	}

	// sort top results
	for (i = 0; i < top_n; i++) {
		for (j = len - 1; j > i; j--) {
			if (p_in[p_idx[j]] > p_in[p_idx[j - 1]]) {
				SWAP(p_idx[j], p_idx[j - 1], tmp);
			}
		}
	}

	// copy top results to output buffer
	for (i = 0; i < top_n; i++) {
		p_classes[i].no = p_idx[i];
		p_classes[i].score = p_in[p_idx[i]];
	}
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                             */
/*-----------------------------------------------------------------------------*/
HD_RESULT nova_set_image_buf(MEM_PARM *img_mem, HD_DIM img_dim, HD_VIDEO_PXLFMT img_fmt, AI_BUF *img_buf)
{
	HD_RESULT ret = HD_OK;
	UINT32 width = ALIGN_CEIL(img_dim.w, 2);
	UINT32 height = ALIGN_CEIL(img_dim.h, 2);
	UINT32 align_width = ALIGN_CEIL(img_dim.w, GFX_W_ALIGN);
	UINT32 align_height = ALIGN_CEIL(img_dim.h, GFX_H_ALIGN);
	UINT32 channel = 0;
	UINT32 img_size = 0, align_img_size = 0;

	switch (img_fmt) {
	case HD_VIDEO_PXLFMT_YUV420:
		img_size = align_width * align_height * 3 / 2;
		channel = 2;
		break;
	case HD_VIDEO_PXLFMT_RGB888_PLANAR:
		img_size = align_width * align_height * 3;
		channel = 3;
		break;
	default:
		printf("ERR: nova_set_image_buf failed, img fmt %d not supported\n", img_fmt);
		return HD_ERR_NOT_SUPPORT;
	}
	align_img_size = ALIGN_CEIL(img_size, CPU_CACHELINE_ALIGN);
	nova_mem_alloc(img_mem, "img_mem", align_img_size);

	img_buf->width = width;
	img_buf->height = height;
	img_buf->channel = channel;
	img_buf->line_ofs = align_width;
	img_buf->fmt  = img_fmt;
	img_buf->pa   = img_mem->pa;
	img_buf->va   = img_mem->va;
	img_buf->sign = MAKEFOURCC('A', 'B', 'U', 'F');
	img_buf->size = align_img_size;
	return ret;
}

HD_RESULT nova_model_init(NOVA_MODEL *model_info)
{
	HD_RESULT ret = HD_OK;
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("ERR: hd_common_init fail=%d\n", ret);
		goto hd_exit;
	}
	ret = hd_gfx_init();
	if (ret != HD_OK) {
		printf("ERR: hd_gfx_init fail=%d\n", ret);
		goto gfx_exit;
	}
	ret = mem_init();
	if (ret != HD_OK) {
		printf("ERR: mem_init fail=%d\n", ret);
		goto mem_exit;
	}
	ret = ai_init();
	if (ret != HD_OK) {
		printf("ERR: ai_init fail=%d\n", ret);
		goto ai_exit;
	}
	memcpy((VOID *)model_info->proc_info.model_filename, (VOID *)model_info->model_path, sizeof(CHAR) * PATH_LEN);
	ret = model_init(&model_info->proc_info, "cls_model");
	if (ret != HD_OK) {
		printf("ERR: model_init fail=%d\n", ret);
		goto model_exit;
	}
	// load label
	if (strlen(model_info->label_path) != 0) {
		ret = _load_label((UINTPTR)model_info->class_labels, LABEL_LEN, model_info->label_path);
		if (ret != HD_OK) {
			printf("ERR: load_label fail=%d\n", ret);
			return HD_ERR_FAIL;
		}
	}
	return ret;

model_exit:
	ai_uninit();
ai_exit:
	mem_uninit();
mem_exit:
	hd_gfx_uninit();
gfx_exit:
	hd_common_uninit();
hd_exit:
	return HD_ERR_NG;
}

HD_RESULT nova_model_uninit(NOVA_MODEL *model_info)
{
	HD_RESULT ret = HD_OK;
	ret = model_uninit(&model_info->proc_info);
	if (ret != HD_OK) {
		printf("ERR: model_uninit fail=%d\n", ret);
	}
	ret = ai_uninit();
	if (ret != HD_OK) {
		printf("ERR: ai_uninit fail=%d\n", ret);
	}
	ret = mem_uninit();
	if (ret != HD_OK) {
		printf("ERR: mem_uninit fail=%d\n", ret);
	}
	ret = hd_gfx_uninit();
	if (ret != HD_OK) {
		printf("ERR: hd_gfx_uninit fail=%d\n", ret);
	}
	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("ERR: hd_common_uninit fail=%d\n", ret);
	}
	return ret;
}

HD_RESULT nova_model_inference(NOVA_MODEL *model_info, NOVA_INPUT *in_info)
{
	HD_RESULT ret = HD_OK;
	AI_BUF ai_buf = {0};
	NET_PROC *p_net = &model_info->proc_info;
#if defined(CHIP_CNN30_IPC) || defined(CHIP_CNN30_XVR)
	for (UINT32 i = 0; i < p_net->net_info.in_buf_cnt; i++) {
		vendor_ai3_net_set(p_net->proc_id, p_net->net_info.in_path_list[i], &in_info->img_buf[i]);
	}
	for (UINT32 i = 0; i < p_net->net_info.out_buf_cnt; i++) {
		vendor_ai3_net_get(p_net->proc_id, p_net->net_info.out_path_list[i], &ai_buf);

		if (ai_buf.size > p_net->out_mem[i].size) {
			printf("output size %u < ai_buf.size %u\r\n", p_net->out_mem[i].size, ai_buf.size);
			return HD_ERR_NG;
		}

		ai_buf.va = p_net->out_mem[i].va;
		ai_buf.pa = p_net->out_mem[i].pa;
		ai_buf.size = p_net->out_mem[i].size;

		vendor_ai3_net_set(p_net->proc_id, p_net->net_info.out_path_list[i], &ai_buf);
	}
	ret = vendor_ai3_net_proc(p_net->proc_id);
	if (HD_OK != ret) {
		printf("ERR: proc_id(%u) proc fail !!\n", p_net->proc_id);
		return ret;
	}
	for (UINT32 i = 0; i < p_net->net_info.out_buf_cnt; i++) {
		// get out buf (by out path list)
		ret = vendor_ai3_net_get(p_net->proc_id, p_net->net_info.out_path_list[i], &ai_buf);
		if (HD_OK != ret) {
			printf("ERR: proc_id(%u) get out buf fail, i(%d), out_path(0x%lx)\n", p_net->proc_id, i, (unsigned long)p_net->net_info.out_path_list[i]);
			return ret;
		}
		UINT32 mem_size = ALIGN_CEIL_64(ai_buf.batch_ofs * ai_buf.batch_num);
		if (hd_common_mem_flush_cache((VOID *)ai_buf.va, mem_size) != HD_OK) {
			printf("ERR: flush cache failed.\r\n");
		}
		INT32 data_num = ai_buf.batch_num * ai_buf.channel * ai_buf.height * ai_buf.width;
		vendor_ai_cpu_util_fixed2float((VOID *)ai_buf.va, ai_buf.fmt, p_net->float_out[i], ai_buf.scale_ratio, data_num, ai_buf.zero_point);
	}
#elif defined(CHIP_CNN25_IPC) || defined(CHIP_CNN25_XVR)
	for (UINT32 i = 0; i < p_net->net_info.in_buf_cnt; i++) {
		vendor_ai_net_set(p_net->proc_id, p_net->in_path_list[i], &in_info->img_buf[i]);
	}
	for (UINT32 i = 0; i < p_net->net_info.out_buf_cnt; i++) {
		vendor_ai_net_get(p_net->proc_id, p_net->out_path_list[i], &ai_buf);

		if (ai_buf.size > p_net->out_mem[i].size) {
			printf("output size %u < ai_buf.size %u\r\n", p_net->out_mem[i].size, ai_buf.size);
			return HD_ERR_NG;
		}

		ai_buf.va = p_net->out_mem[i].va;
		ai_buf.pa = p_net->out_mem[i].pa;
		ai_buf.size = p_net->out_mem[i].size;

		vendor_ai_net_set(p_net->proc_id, p_net->out_path_list[i], &ai_buf);
	}
	ret = vendor_ai_net_proc(p_net->proc_id);
	if (HD_OK != ret) {
		printf("ERR: proc_id(%u) proc fail !!\n", p_net->proc_id);
		return ret;
	}
	for (UINT32 i = 0; i < p_net->net_info.out_buf_cnt; i++) {
		// get out buf (by out path list)
		ret = vendor_ai_net_get(p_net->proc_id, p_net->out_path_list[i], &ai_buf);
		if (HD_OK != ret) {
			printf("ERR: proc_id(%u) get out buf fail, i(%d), out_path(0x%lx)\n", p_net->proc_id, i, (unsigned long)p_net->out_path_list[i]);
			return ret;
		}
		if (hd_common_mem_flush_cache((VOID *)ai_buf.va, ai_buf.size) != HD_OK) {
			printf("ERR: flush cache failed.\r\n");
		}
		INT32 data_num = ai_buf.batch_num * ai_buf.channel * ai_buf.height * ai_buf.width;
		vendor_ai_cpu_util_fixed2float((VOID *)ai_buf.va, ai_buf.fmt, p_net->float_out[i], ai_buf.scale_ratio, data_num);
	}
#endif
	return ret;
}

HD_RESULT nova_mem_alloc(MEM_PARM *mem_parm, CHAR *name, UINT32 size)
{
	HD_RESULT ret = HD_OK;
	if (mem_parm->pa && mem_parm->va) {
		printf("WRN: MEM_PARM already alloc ? release first...\n");
		nova_mem_free(mem_parm);
	}
	if (size == 0) {
		printf("ERR: nova_mem_alloc fail, wrong size = 0\r\n");
		return HD_ERR_NG;
	}
#if defined(CHIP_CNN25_IPC) || defined(CHIP_CNN30_IPC) || defined(CHIP_CNN30_XVR) || defined(CHIP_NPU)
	UINTPTR pa = 0;
	VOID   *va = NULL;
	//alloc private pool
	ret = hd_common_mem_alloc(name, &pa, (VOID **)&va, size, DDR_ID0);
	if (ret != HD_OK) {
		printf("ERR: hd_common_mem_alloc (%s) size fail\n", name);
		return ret;
	}
	mem_parm->pa   = pa;
	mem_parm->va   = (UINTPTR)va;
	mem_parm->size = size;
	mem_parm->blk  = -1;
#elif defined(CHIP_CNN25_XVR)
	mem_parm->size = size;
	mem_parm->blk = hd_common_mem_get_block(HD_COMMON_MEM_CNN_POOL, mem_parm->size, DDR_ID0);
	if (HD_COMMON_MEM_VB_INVALID_BLK == mem_parm->blk) {
		printf("ERR: hd_common_mem_get_block fail\r\n");
		return HD_ERR_NG;
	}
	mem_parm->pa = hd_common_mem_blk2pa(mem_parm->blk);
	if (mem_parm->pa == 0) {
		printf("ERR: hd_common_mem_blk2pa fail, blk = %#lx\r\n", mem_parm->blk);
		hd_common_mem_release_block(mem_parm->blk);
		return HD_ERR_NG;
	}
	/* Must use "HD_COMMON_MEM_MEM_TYPE_CACHE", or it will cause the cpu layer to perform inefficiently */
	mem_parm->va = (UINTPTR)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, mem_parm->pa, mem_parm->size);
	if (mem_parm->va == 0) {
		return HD_ERR_NG;
	}
#endif
	return ret;
}

HD_RESULT nova_mem_free(MEM_PARM *mem_parm)
{
	if ((mem_parm == NULL) || (mem_parm->pa == 0) || (mem_parm->va == 0)) {
		return HD_OK;
	}

	HD_RESULT ret = HD_OK;
	//free private pool
#if defined(CHIP_CNN25_IPC) || defined(CHIP_CNN30_IPC) || defined(CHIP_CNN30_XVR) || defined(CHIP_NPU)
	ret =  hd_common_mem_free(mem_parm->pa, (VOID *)mem_parm->va);
	if (ret != HD_OK) {
		return ret;
	}
#elif defined(CHIP_CNN25_XVR)
	if (mem_parm->va) {
		ret = hd_common_mem_munmap((VOID *)mem_parm->va, mem_parm->size);
		if (ret != HD_OK) {
			printf("ERR: hd_common_mem_munmap fail\n");
			return ret;
		}
	}
	ret = hd_common_mem_release_block(mem_parm->blk);
	if (ret != HD_OK) {
		printf("ERR: hd_common_mem_release_block fail\n");
		return ret;
	}
#endif
	mem_parm->pa = 0;
	mem_parm->va = 0;
	mem_parm->size = 0;
	mem_parm->blk = -1;
	return ret;
}

HD_RESULT ai_net_accuracy_process(AI_NET_ACCURACY_PARM *p_parm)
{
	FLOAT *p_in     = (FLOAT *)p_parm->in_addr;

	AI_NET_OUTPUT_CLASS *p_classes = p_parm->classes;
	INT32 num       = p_parm->shape.num;
	INT32 channels  = p_parm->shape.channels;
	INT32 height    = p_parm->shape.height;
	INT32 width     = p_parm->shape.width;
	INT32 top_n     = p_parm->top_n;
	INT32 *p_idx    = p_parm->class_idx;
	INT32 i, j, k;

	if (width == 1 && height == 1) {
		top_n = MIN(channels, top_n);
		for (i = 0; i < num; i++) {
			ai_net_sort_result(p_in, channels, p_idx, top_n, p_classes);

			p_in += channels;
			p_classes += top_n;
		}
	} else {
		top_n = MIN(width, top_n);
		for (i = 0; i < num; i++) {
			for (j = 0; j < channels; j++) {
				for (k = 0; k < height; k++) {
					ai_net_sort_result(p_in, width, p_idx, top_n, p_classes);

					p_in += width;
					p_classes += top_n;
				}
			}
		}
	}
	p_parm->top_n = top_n;
	return HD_OK;
}
