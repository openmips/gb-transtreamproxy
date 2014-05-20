#############################################
#              BUILD OPTION
#############################################
# - BUILD_MODE			: debug | release
# - BUILD_CROSS			: y | n
# - SILENT_BUILD_MODE	: y | n
#############################################

BUILD_MODE=debug
BUILD_CROSS=y

SILENT_BUILD_MODE=y

ifeq ($(BUILD_CROSS),y)
ARCH=mipsel
IMAGE=vusolo2
FLATFORM=mipsel-oe-linux
OE_TOP=/home/oskwon/works/openvuplus
endif
