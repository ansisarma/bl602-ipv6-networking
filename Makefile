

PROJECT_NAME := projectiot
PROJECT_PATH := $(abspath .)
PROJECT_BOARD := evb
export PROJECT_PATH PROJECT_BOARD

-include ./proj_config.mk

ifeq ($(origin BL60X_SDK_PATH), undefined)
$(error ****** Please set BL60X_SDK_PATH to your SDK root ******)
endif

NETWORK_FLAGS := \
    -DLWIP_IPV6=1 \
    -DLWIP_IPV6_MLD=1 \
    -DLWIP_IPV6_AUTOCONFIG=1

CFLAGS   += $(NETWORK_FLAGS)
CPPFLAGS += $(NETWORK_FLAGS)
CXXFLAGS += $(NETWORK_FLAGS)
CFLAGS += -Wno-error=volatile -Wno-volatile
CXXFLAGS += -Wno-error=volatile -Wno-volatile

CFLAGS   += -Wno-error=unused-function -Wno-error=unused-parameter
CXXFLAGS += -Wno-error=unused-function -Wno-error=unused-parameter

COMPONENTS_NETWORK := dns_server

COMPONENTS_BLSYS   := bltime blfdt blmtd blota bloop loopadc looprt loopset
COMPONENTS_VFS     := romfs
COMPONENTS_UTILS   := cjson etl

INCLUDE_COMPONENTS += freertos bl602 bl602_std bl602_wifi bl602_wifidrv hal_drv lwip vfs yloop utils cli
INCLUDE_COMPONENTS += blog blog_testc easyflash4 mbedtls-bl602

INCLUDE_COMPONENTS += $(COMPONENTS_NETWORK)

INCLUDE_COMPONENTS += $(COMPONENTS_BLSYS)

INCLUDE_COMPONENTS += $(COMPONENTS_VFS)
INCLUDE_COMPONENTS += $(COMPONENTS_UTILS)

INCLUDE_COMPONENTS += $(PROJECT_NAME)

include $(BL60X_SDK_PATH)/make_scripts_riscv/project.mk
