/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <string.h>
#include "hd_type.h"
#include "kwrap/debug.h"

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT nova_file_readbin(UINTPTR addr, UINT32 size, const CHAR *filename)
{
	FILE *fd;
	HD_RESULT ret = HD_OK;

	fd = fopen(filename, "rb");

	if (!fd) {
		DBG_ERR("cannot read %s\r\n", filename);
		return HD_ERR_NG;
	}

	DBG_IND("open %s ok\r\n", filename);

	if (fread((VOID *)addr, 1, size, fd) != size) {
		DBG_ERR("read size < %d\r\n", size);
		ret = HD_ERR_NG;
	}

	if (fd) {
		fclose(fd);
	}

	return ret;
}

HD_RESULT nova_file_writebin(UINTPTR addr, UINT32 size, const CHAR *filename)
{
	FILE *fd;
	HD_RESULT ret = HD_OK;

	fd = fopen(filename, "wb");

	if (!fd) {
		DBG_ERR("cannot write %s\r\n", filename);
		return HD_ERR_NG;
	}

	DBG_IND("open %s ok\r\n", filename);

	if (fwrite((VOID *)addr, 1, size, fd) != size) {
		DBG_ERR("wrote size < %d\r\n", size);
		ret = HD_ERR_NG;
	}

	if (fd) {
		fclose(fd);
	}

	return ret;
}

INT32 nova_file_loadbin(UINTPTR addr, const CHAR *filename)
{
	FILE *fd;
	INT32 size = 0;

	fd = fopen(filename, "rb");

	if (NULL == fd) {
		DBG_ERR("cannot read %s\r\n", filename);
		return -1;
	}

	DBG_IND("open %s ok\r\n", filename);

	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	if (size < 0) {
		DBG_ERR("getting %s size failed\r\n", filename);
	} else if ((INT32)fread((VOID *)addr, 1, size, fd) != size) {
		DBG_ERR("read size < %d\r\n", size);
		size = -1;
	}

	if (fd) {
		fclose(fd);
	}

	return size;
}

HD_RESULT nova_file_readtxt(UINTPTR addr, UINT32 line_len, UINT32 line_num, const CHAR *filename)
{
	FILE *fd;
	CHAR *p_line = (CHAR *)addr;
	UINT32 i;

	fd = fopen(filename, "r");

	if (!fd) {
		DBG_ERR("cannot read %s\r\n", filename);
		return HD_ERR_NG;
	}

	DBG_IND("open %s ok\r\n", filename);

	for (i = 0; i < line_num; i++) {
		fgets(p_line, line_len, fd);
		p_line[strlen(p_line) - 1] = '\0'; // remove newline character
		p_line += line_len;
	}

	if (fd) {
		fclose(fd);
	}

	return HD_OK;
}
