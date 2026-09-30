###############################################################################
# Common-Head Makefile                                                        #
###############################################################################
AI2_IPC_CODENAMES = na51055 na51089 na51102 ns02301
AI2_XVR_CODENAMES = na51090 na51103
AI3_IPC_CODENAMES = ns02201 ns02302 ns02501 ns02601 ns02602
AI3_XVR_CODENAMES = ns02401
NPU_CODENAMES = ns02502

ifeq ($(SDK_CODENAME),) # early IC na51055 and na51068
CHIP_CFLAGS = -D CHIP_CNN25_IPC
FOR_AI2_SDK = TRUE
else ifneq ($(filter $(SDK_CODENAME),$(AI2_IPC_CODENAMES)),)
CHIP_CFLAGS = -D CHIP_CNN25_IPC
FOR_AI2_SDK = TRUE
else ifneq ($(filter $(SDK_CODENAME),$(AI2_XVR_CODENAMES)),)
CHIP_CFLAGS = -D CHIP_CNN25_XVR
FOR_AI2_SDK = TRUE
else ifneq ($(filter $(SDK_CODENAME),$(AI3_IPC_CODENAMES)),)
CHIP_CFLAGS = -D CHIP_CNN30_IPC
FOR_AI3_SDK = TRUE
else ifneq ($(filter $(SDK_CODENAME),$(AI3_XVR_CODENAMES)),)
CHIP_CFLAGS = -D CHIP_CNN30_XVR
FOR_AI3_SDK = TRUE
else ifneq ($(filter $(SDK_CODENAME),$(NPU_CODENAMES)),)
CHIP_CFLAGS = -D CHIP_NPU
endif


# DIRs
JPEG_DIR = ../../3rdparty/libjpeg
OUTPUT_DIR = ../output/$(MODULE_NAME)
# PATHs
HDAL_INC_PATH   = $(NVT_HDAL_DIR)/include
HDAL_LIB_PATH   = $(NVT_HDAL_DIR)/output
VOS_LIB_PATH    = $(NVT_VOS_DIR)/output
VENDOR_LIB_PATH = $(NVT_HDAL_DIR)/vendor/output
AI2_LIB         = $(NVT_HDAL_DIR)/vendor/ai2/source
AI3_LIB         = $(NVT_HDAL_DIR)/vendor/ai3/source

uclibc=$(shell echo $(CROSS_COMPILE)|grep uclib)
ifeq ($(FOR_AI2_SDK),TRUE)
ifeq ($(uclibc),)
	PREBUILD_LIB=$(NVT_HDAL_DIR)/vendor/ai/source/prebuilt/lib/glibc
else
	PREBUILD_LIB=$(NVT_HDAL_DIR)/vendor/ai/source/prebuilt/lib/uclibc
endif
endif

ifeq ($(FOR_AI3_SDK),TRUE)
ifeq ($(uclibc),)
	PREBUILD_LIB=$(NVT_HDAL_DIR)/vendor/ai3/source_pub/prebuilt/lib/glibc
else
	PREBUILD_LIB=$(NVT_HDAL_DIR)/vendor/ai3/source_pub/prebuilt/lib/uclibc
endif
endif

ifeq ($(findstring x64,$(LINUX_CPU_TYPE)),x64)
	JPEG_LIB=$(JPEG_DIR)/lib/glibc64
else
	JPEG_LIB=$(JPEG_DIR)/lib/glibc32
endif

# INC FLAGs
EXTRA_INCLUDE += -I$(HDAL_INC_PATH) -I$(NVT_HDAL_DIR)/source
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_flow/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_driver/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_driver/source/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai2/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai2/source_pub/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai2/source_pub/vendor_ai_cpu
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai2/source_pub/vendor_ai_dsp
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai/drivers/k_driver/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai2/drivers/k_flow/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai2/drivers/k_flow/source/kflow_ai_net
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/media/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai3/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai3/source_pub/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai3/drivers/k_driver/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai3/drivers/k_flow/include
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/vendor/ai3/drivers/k_flow/source/kflow_ai_net
EXTRA_INCLUDE += -I$(NVT_VOS_DIR)/drivers/include
EXTRA_INCLUDE += -I$(JPEG_DIR)/include
EXTRA_INCLUDE += -I../util

.PHONY: all clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#--------- ENVIRONMENT SETTING --------------------
WARNING			= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror
COMPILE_OPTS	= -I. -O3 -fPIC -ffunction-sections -fdata-sections -D__LINUX
C_CFLAGS	 = $(NVT_GCOV) $(NVT_ASAN) $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE) $(CHIP_CFLAGS)
ifeq ($(FOR_AI3_SDK),TRUE)
C_CFLAGS	+= $(subst xvr, -D_NVT_NVR_SDK_, $(subst ipc, -D_NVT_IPC_SDK_,$(NVT_SDK_TYPE)))
endif
LD_FLAGS	+= $(NVT_GCOV) $(NVT_ASAN) -lpthread -lm
LD_FLAGS	+= -L$(HDAL_LIB_PATH) -lhdal
LD_FLAGS	+= -L$(VOS_LIB_PATH) -lvos
LD_FLAGS	+= -L$(VENDOR_LIB_PATH) -lvendor_media
ifeq ($(FOR_AI2_SDK),TRUE)
LD_FLAGS	+= -L$(AI2_LIB) -lvendor_ai2
LD_FLAGS	+= -L$(AI2_LIB)_pub -lvendor_ai2_pub
endif
ifeq ($(FOR_AI3_SDK),TRUE)
LD_FLAGS	+= -L$(AI3_LIB) -lvendor_ai3
LD_FLAGS	+= -L$(AI3_LIB)_pub -lvendor_ai3_pub
endif
LD_FLAGS	+= -L$(PREBUILD_LIB) -lprebuilt_ai
LD_FLAGS	+= -L$(JPEG_LIB) -ljpeg
#--------- END OF ENVIRONMENT SETTING -------------
SRC += \
	../util/util_model.c ../util/util_img.c ../util/util_input.c ../util/util_output.c \
	../util/util_file.c

OBJ = $(SRC:.c=.o)

ifeq ("$(wildcard *.c */*.c)","")
all:
	@echo "nothing to be done for '$(MODULE_NAME)'"
clean:
	@echo "nothing to be done for '$(MODULE_NAME)'"
install:
	@echo "nothing to be done for '$(MODULE_NAME)'"
else
all: $(MODULE_NAME)

$(MODULE_NAME): $(OBJ)
	@echo Creating $@...
	@$(CC) -o $@ $(OBJ) $(LD_FLAGS)
	@$(NM) -n $@ > $@.sym
	@$(STRIP) $@
	@$(OBJCOPY) -R .comment -R .note.ABI-tag -R .gnu.version $@

%.o: %.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) -c $< -o $@

clean:
	@rm -f $(MODULE_NAME) $(OBJ) $(MODULE_NAME).sym *.o *.a *.so*
endif

install:
	@cp -avf $(MODULE_NAME) $(ROOTFS_DIR)/rootfs/usr/bin
	@mkdir -p $(OUTPUT_DIR)
	@cp -avf $(MODULE_NAME) $(OUTPUT_DIR)
	@cp -avf $(JPEG_LIB)/libjpeg.so.10 $(OUTPUT_DIR)
	@cp -avf $(LABEL) $(OUTPUT_DIR)

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
#--------- ENVIRONMENT SETTING --------------------
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE)
#--------- END OF ENVIRONMENT SETTING -------------
LIB_NAME = lib$(MODULE_NAME).a
SRC =

OBJ = $(SRC:.c=.o)

all:
	@echo "nothing to be done for '$(LIB_NAME)'"
clean:
	@echo "nothing to be done for '$(LIB_NAME)'"
install:
	@echo "nothing to be done for '$(LIB_NAME)'"
endif
