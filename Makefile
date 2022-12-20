#
# Copyright 2019-2020 Xilinx, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%/L1/*}')

# MK_INC_BEGIN hls_common.mk

.PHONY: help

help::
	@echo ""
	@echo "Makefile Usage:"
	@echo ""
	@echo "  make run CSIM=1 CSYNTH=1 COSIM=1 DEVICE=<FPGA platform> PLATFORM_REPO_PATHS=<path to platform directories>"
	@echo "      Command to run the selected tasks for specified device."
	@echo ""
	@echo "      Valid tasks are CSIM, CSYNTH, COSIM, VIVADO_SYN, VIVADO_IMPL"
	@echo ""
	@echo "      DEVICE is case-insensitive and support awk regex."
	@echo "      For example, \`make run DEVICE='u200.*xdma' COSIM=1\`"
	@echo "      It can also be an absolute path to platform file."
	@echo ""
	@echo "      PLATFORM_REPO_PATHS variable is used to specify the paths in which the platform files will be"
	@echo "      searched for."
	@echo ""
	@echo "  make run CSIM=1 CSYNTH=1 COSIM=1 XPART=<FPGA part name>"
	@echo "      Alternatively, the FPGA part can be speficied via XPART."
	@echo "      For example, \`make run XPART='xcu200-fsgd2104-2-e' COSIM=1\`"
	@echo "      When XPART is set, DEVICE will be ignored."
	@echo ""
	@echo "  make clean "
	@echo "      Command to remove the generated files."
	@echo ""

# MK_INC_END hls_common.mk

# MK_INC_BEGIN vivado.mk

.PHONY: check_vivado
check_vivado:
ifeq (,$(wildcard $(XILINX_VIVADO)/bin/vivado))
	@echo "Cannot locate Vivado installation. Please set XILINX_VIVADO variable." && false
endif

export PATH := $(XILINX_VIVADO)/bin:$(PATH)

# MK_INC_END vivado.mk

# MK_INC_BEGIN vitis_set_part.mk

# MK_INC_BEGIN vitis.mk

.PHONY: check_vpp
check_vpp:
ifeq (,$(wildcard $(XILINX_VITIS)/bin/v++))
	@echo "Cannot locate Vitis installation. Please set XILINX_VITIS variable." && false
endif

.PHONY: check_xrt
check_xrt:
ifeq (,$(wildcard $(XILINX_XRT)/lib/libxilinxopencl.so))
	@echo "Cannot locate XRT installation. Please set XILINX_XRT variable." && false
endif

.PHONY: check_opencv
check_opencv:
ifeq (,$(OPENCV_INCLUDE))
	@echo "Cannot find OpenCV include path. Please set OPENCV_INCLUDE variable" && false
endif
ifeq (,$(OPENCV_LIB))
	@echo "Cannot find Opencv lib path. Please set OPENCV_LIB variable" && false
endif

export PATH := $(XILINX_VITIS)/bin:$(XILINX_XRT)/bin:$(PATH)

ifeq (,$(LD_LIBRARY_PATH))
LD_LIBRARY_PATH := $(XILINX_XRT)/lib
else
LD_LIBRARY_PATH := $(XILINX_XRT)/lib:$(LD_LIBRARY_PATH)
endif
ifneq (,$(wildcard $(XILINX_VITIS)/bin/ldlibpath.sh))
export LD_LIBRARY_PATH := $(shell $(XILINX_VITIS)/bin/ldlibpath.sh $(XILINX_VITIS)/lib/lnx64.o):$(LD_LIBRARY_PATH)
endif

# MK_INC_END vitis.mk

.PHONY: check_part

ifeq (,$(XPART))
# MK_INC_BEGIN vitis_set_platform.mk

ifneq (,$(wildcard $(DEVICE)))
# Use DEVICE as a file path
XPLATFORM := $(DEVICE)
else
# Use DEVICE as a file name pattern
DEVICE_L := $(shell echo $(DEVICE) | tr A-Z a-z)
# 1. search paths specified by variable
ifneq (,$(PLATFORM_REPO_PATHS))
# 1.1 as exact name
XPLATFORM := $(strip $(foreach p, $(subst :, ,$(PLATFORM_REPO_PATHS)), $(wildcard $(p)/$(DEVICE_L)/$(DEVICE_L).xpfm)))
# 1.2 as a pattern
ifeq (,$(XPLATFORM))
XPLATFORMS := $(foreach p, $(subst :, ,$(PLATFORM_REPO_PATHS)), $(wildcard $(p)/*/*.xpfm))
XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(DEVICE_L)/')))
endif # 1.2
endif # 1
# 2. search Vitis installation
ifeq (,$(XPLATFORM))
# 2.1 as exact name
XPLATFORM := $(strip $(wildcard $(XILINX_VITIS)/platforms/$(DEVICE_L)/$(DEVICE_L).xpfm))
# 2.2 as a pattern
ifeq (,$(XPLATFORM))
XPLATFORMS := $(wildcard $(XILINX_VITIS)/platforms/*/*.xpfm)
XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(DEVICE_L)/')))
endif # 2.2
endif # 2
# 3. search default locations
ifeq (,$(XPLATFORM))
# 3.1 as exact name
XPLATFORM := $(strip $(wildcard /opt/xilinx/platforms/$(DEVICE_L)/$(DEVICE_L).xpfm))
# 3.2 as a pattern
ifeq (,$(XPLATFORM))
XPLATFORMS := $(wildcard /opt/xilinx/platforms/*/*.xpfm)
XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(DEVICE_L)/')))
endif # 3.2
endif # 3
endif

define MSG_PLATFORM
No platform matched pattern '$(DEVICE)'.
Available platforms are: $(XPLATFORMS)
To add more platform directories, set the PLATFORM_REPO_PATHS variable or point DEVICE variable to the full path of platform .xpfm file.
endef
export MSG_PLATFORM

define MSG_DEVICE
More than one platform matched: $(XPLATFORM)
Please set DEVICE variable more accurately to select only one platform file, or set DEVICE variable to the full path of the platform .xpfm file.
endef
export MSG_DEVICE

.PHONY: check_platform
check_platform:
ifeq (,$(XPLATFORM))
	@echo "$${MSG_PLATFORM}" && false
endif
ifneq (,$(word 2,$(XPLATFORM)))
	@echo "$${MSG_DEVICE}" && false
endif

XDEVICE := $(basename $(notdir $(firstword $(XPLATFORM))))

# MK_INC_END vitis_set_platform.mk
ifeq (1, $(words $(XPLATFORM)))
# Query the part name of device
ifneq (,$(wildcard $(XILINX_VITIS)/bin/platforminfo))
override XPART := $(shell $(XILINX_VITIS)/bin/platforminfo --json="hardwarePlatform.devices[0].fpgaPart" --platform $(firstword $(XPLATFORM)) | sed 's/^[^:]*://g' | sed 's/[^a-zA-Z0-9]/-/g' | sed 's/-\+/-/g')
endif
endif
check_part: check_platform check_vpp
ifeq (,$(XPART))
	@echo "XPART is not set and cannot be inferred. Please run \`make help\` for usage info." && false
endif
else # XPART
check_part:
	@echo "XPART is directly set to $(XPART)"
endif # XPART

# MK_INC_END vitis_set_part.mk

# MK_INC_BEGIN hls_test_rules.mk

DEVICE = $(XILINX_VITIS)/base_platforms/xilinx_zcu104_base_202210_1/xilinx_zcu104_base_202210_1.xpfm
TARGET = hw
PLATFORM_NAME = zcu104
TEMP_DIR := _x_temp.$(TARGET).$(PLATFORM_NAME)
TEMP_REPORT_DIR := $(CUR_DIR)/reports/_x.$(TARGET).$(PLATFORM_NAME)
BUILD_DIR := build_dir.$(TARGET).$(PLATFORM_NAME)
BUILD_REPORT_DIR := $(CUR_DIR)/reports/_build.$(TARGET).$(PLATFORM_NAME)
#CXXFLAGS += -I$(CUR_DIR)/src/ -fmessage-length=0 --sysroot=$(SYSROOT)  -I$(SYSROOT)/usr/include/xrt -I$(XILINX_HLS)/include -std=c++14 -O3 -Wall -Wno-unknown-pragmas -Wno-unused-label
LDFLAGS += -pthread -L$(SYSROOT)/usr/lib -L$(XILINX_VITIS_AIETOOLS)/lib/aarch64.o -Wl,--as-needed -lxilinxopencl -lxrt_coreutil
VPP_FLAGS += -t $(TARGET) --platform $(DEVICE) --save-temps --config connectivity.cfg --profile.data all:all:all --profile.trace_memory HP0
VPP_LDFLAGS += --optimize 2 -R 2
VPP = v++
K_IMAGE = ~/projects/plinux/images/linux/Image
ROOTFS = ~/projects/plinux/images/linux/rootfs.ext4
EMCONFIG := $(BUILD_DIR)/emconfig.json
EXE_FILE = ref

BINARY_CONTAINERS += $(BUILD_DIR)/krnl_AccelRice_pkg.xclbin
BINARY_CONTAINERS_PKG += $(BUILD_DIR)/krnl_AccelRice.xclbin


.PHONY: run setup runhls clean cleanall check

# Alias to run, for legacy test script
check: run

CSIM ?= 0
CSYNTH ?= 0
COSIM ?= 0
VIVADO_SYN ?= 0
VIVADO_IMPL ?= 0
QOR_CHECK ?= 0

# at least RTL synthesis before check QoR
ifeq (1,$(QOR_CHECK))
ifeq (0,$(VIVADO_IMPL))
override VIVADO_SYN := 1
endif
endif

# need synthesis before cosim or vivado
ifeq (1,$(VIVADO_IMPL))
override CSYNTH := 1
endif

ifeq (1,$(VIVADO_SYN))
override CSYNTH := 1
endif

ifeq (1,$(COSIM))
override CSYNTH := 1
endif

# From testbench.data_recipe of description.json
data:
	@true

run: data setup runhls

setup: | check_part check_opencv
	@rm -f ./settings.tcl
	@if [ -n "$$CLKP" ]; then echo 'set CLKP $(CLKP)' >> ./settings.tcl ; fi
	@echo 'set XPART $(XPART)' >> ./settings.tcl
	@echo 'set CSIM $(CSIM)' >> ./settings.tcl
	@echo 'set CSYNTH $(CSYNTH)' >> ./settings.tcl
	@echo 'set COSIM $(COSIM)' >> ./settings.tcl
	@echo 'set VIVADO_SYN $(VIVADO_SYN)' >> ./settings.tcl
	@echo 'set VIVADO_IMPL $(VIVADO_IMPL)' >> ./settings.tcl
	@echo 'set XF_PROJ_ROOT "$(XF_PROJ_ROOT)"' >> ./settings.tcl
	@echo 'set OPENCV_INCLUDE "$(OPENCV_INCLUDE)"' >> ./settings.tcl
	@echo 'set OPENCV_LIB "$(OPENCV_LIB)"' >> ./settings.tcl
	@echo 'set CUR_DIR "$(CUR_DIR)"' >> ./settings.tcl
	@echo "Configured: settings.tcl"
	@echo "----"
	@cat ./settings.tcl
	@echo "----"

HLS ?= vitis_hls
runhls: data setup | check_vivado check_vpp
	$(HLS) -f run_hls.tcl;

CPPSRC = main_tb.cpp \
		 rice.cpp \
		 cfa_comp.cpp \
		 timeit.cpp \
		 HWAccelRice.cpp \

#CSRC = drivers/xrice_compress_accel.c \
	   drivers/xrice_compress_accel_linux.c \

OBJDIR = obj
CPPOBJS := $(CPPSRC:%.cpp=$(OBJDIR)/%.o)
COBJS += $(CSRC:%.c=$(OBJDIR)/%.o)
CFLAGS = -std=c++17 -O3 -g -DHW_IMPLEMENTATION \
		-I$(SDKTARGETSYSROOT)/usr/include/opencv4 \
		-I$(SDKTARGETSYSROOT)/usr/include/xrt

clean:
	rm -rf settings.tcl *_hls.log cfa_comp.prj
	rm -rf $(OBJDIR)
	rm -f ref*
	rm -f comp.cfa
	rm -rf $(BUILD_DIR)
	rm -rf $(TEMP_DIR)
	rm -f v++*
	rm -f vitis_analyzer*

# Used by Jenkins test
cleanall: clean

$(CPPOBJS): $(OBJDIR)/%.o: %.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) -c $(CFLAGS) $< -o $@

$(COBJS): $(OBJDIR)/drivers/%.o: drivers/%.c
	@mkdir -p $(OBJDIR)/drivers
	$(CXX) -c $(CFLAGS) $< -o $@

.PHONY: ref
ref: $(COBJS) $(CPPOBJS)
	@echo $(COBJS)
	@echo $(CPPOBJS)
	$(CXX) $(CFLAGS) -o $@ $(COBJS) $(CPPOBJS)  -L$(SDKTARGETSYSROOT)/usr/lib -lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lxrt_coreutil

.PHONY: opencv
opencv:
	git clone --depth 1 --branch 4.4.0 https://github.com/opencv/opencv.git
	mkdir -p ocvbuild
	cd ocvbuild; cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=vitis_opencv -D WITH_V4L=ON -DBUILD_TESTS=OFF -DBUILD_ZLIB=ON -DBUILD_JPEG=ON -DWITH_JPEG=ON -DWITH_PNG=ON -DBUILD_EXAMPLES=OFF -DINSTALL_C_EXAMPLES=OFF -DINSTALL_PYTHON_EXAMPLES=OFF -DWITH_OPENEXR=OFF -DBUILD_OPENEXR=OFF -DBUILD_JAVA=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_TESTS=OFF -DBUILD_OPENCV_APPS=OFF -DBUILD_OPENCV_DNN=OFF -DBUILD_OPENCV_FLANN=OFF -DBUILD_OPENCV_GAP=OFF -DBUILD_OPENCV_STITCHING=OFF -DBUILD_OPENCV_TS=OFF -DBUILD_OPENCV_VIDEO=OFF ../opencv
	make -C ocvbuild -j24
	make -C ocvbuild install

.PHONY: patch
patch:
	@echo "Applying kernel patch"

.PHONY: vitis
vitis: $(BINARY_CONTAINERS)

$(TEMP_DIR)/AccelRice.xo: AccelRice.cpp
	@echo "Compiling Kernel: AccelRice"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS) -k Rice_Compress_accel -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^

BINARY_CONTAINER_krnl_IP_OBJS += $(TEMP_DIR)/AccelRice.xo

BINARY_CONTAINERS_DEPS += $(BINARY_CONTAINER_krnl_IP_OBJS)
$(BINARY_CONTAINERS): $(BINARY_CONTAINERS_DEPS)
	mkdir -p $(BUILD_DIR)
	$(VPP) -l $(VPP_FLAGS) --temp_dir $(TEMP_DIR) --report_dir $(BUILD_REPORT_DIR)/krnl_AccelRice $(VPP_LDFLAGS) -o $@ $^

$(EMCONFIG):
	emconfigutil --platform $(DEVICE) --od $(BUILD_DIR)
############################## Preparing sdcard folder ##############################
RUN_SCRIPT := $(BUILD_DIR)/run_script.sh
$(RUN_SCRIPT):
	rm -rf $(RUN_SCRIPT)
	@echo 'export LD_LIBRARY_PATH=/mnt:/tmp:$(LIBRARY_PATH)' >> $(RUN_SCRIPT)
ifneq ($(filter sw_emu hw_emu, $(TARGET)),)
	@echo 'export XCL_EMULATION_MODE=$(TARGET)' >> $(RUN_SCRIPT)
endif
	@echo 'export XILINX_VITIS=/mnt' >> $(RUN_SCRIPT)
	@echo 'export XILINX_XRT=/usr' >> $(RUN_SCRIPT)
	@echo 'if [ -f platform_desc.txt  ]; then' >> $(RUN_SCRIPT)
	@echo '        cp platform_desc.txt /etc/xocl.txt' >> $(RUN_SCRIPT)
	@echo 'fi' >> $(RUN_SCRIPT)
	@echo './$(EXE_NAME) $(PKG_HOST_ARGS)' >> $(RUN_SCRIPT)
	@echo 'return_code=$$?' >> $(RUN_SCRIPT)
	@echo 'if [ $$return_code -ne 0 ]; then' >> $(RUN_SCRIPT)
	@echo '        echo "ERROR: Embedded host run failed, RC=$$return_code"' >> $(RUN_SCRIPT)
	@echo 'else' >> $(RUN_SCRIPT)
	@echo '        echo "INFO: TEST PASSED, RC=0"' >> $(RUN_SCRIPT)
	@echo 'fi' >> $(RUN_SCRIPT)
	@echo 'echo "INFO: Embedded host run completed."' >> $(RUN_SCRIPT)
	@echo 'exit $$return_code' >> $(RUN_SCRIPT)
DATA_FILE := data/test.png
DATA_DIR :=
SD_FILES += $(RUN_SCRIPT)
SD_FILES += $(EXE_FILE)
SD_FILES += $(EMCONFIG)
SD_FILES += system.dtb
SD_FILES += boot.scr
SD_FILES += xrt.ini
SD_FILES += $(DATA_FILE)# where define DATAFILE in json
SD_FILES_WITH_PREFIX = $(foreach sd_file,$(SD_FILES), $(if $(filter $(sd_file),$(wildcard $(sd_file))), --package.sd_file $(sd_file)))
SD_DIRS_WITH_PREFIX = $(foreach sd_dir,$(DATA_DIR),--package.sd_dir $(sd_dir))
PACKAGE_FILES := $(BINARY_CONTAINERS)
PACKAGE_FILES += $(AIE_CONTAINER)
SD_CARD := $(CUR_DIR)/package_$(TARGET)
dfx_hw := off

.PHONY: emconfig
emconfig: $(EMCONFIG)

$(SD_CARD): $(BINARY_CONTAINERS) $(RUN_SCRIPT) $(EMCONFIG)
	@echo "Generating sd_card folder...."
	mkdir -p $(SD_CARD)
	chmod a+rx $(BUILD_DIR)/run_script.sh
ifeq ($(findstring _dfx_, $(PLATFORM_NAME)),_dfx_)
ifeq ($(TARGET),hw)
	$(VPP) -t $(TARGET) --platform $(DEVICE) -p $(PACKAGE_FILES) $(VPP_PACKAGE) -o $(BINARY_CONTAINERS_PKG)
	$(VPP) -t $(TARGET) --platform $(DEVICE) -p --package.out_dir  $(SD_CARD) --package.rootfs $(ROOTFS) --package.kernel_image $(K_IMAGE)  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX) --package.sd_file $(BINARY_CONTAINERS_PKG)
	@echo "### ***** sd_card generation done! ***** ###"
dfx_hw := on
endif
endif
ifeq ($(dfx_hw), off)
	$(VPP) -t $(TARGET) --platform $(DEVICE) -o $(BINARY_CONTAINERS_PKG) -p $(PACKAGE_FILES) $(VPP_PACKAGE) --package.out_dir  $(SD_CARD) --package.rootfs $(ROOTFS) --package.kernel_image $(K_IMAGE)  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX)
	@echo "### ***** sd_card generation done! ***** ###"
endif

.PHONY: sd_card
sd_card: $(SD_CARD)


# MK_INC_END hls_test_rules.mk