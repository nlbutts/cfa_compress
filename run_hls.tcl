#
# Copyright 2019-2021 Xilinx, Inc.
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

source settings.tcl

set PROJ "cfa_comp.prj"
set SOLN "sol1"
set PRJROOT $env(PRJROOT)
#set VITIS_LIB /home/nlbutts/projects/vitis_isp/Vitis_Libraries/vision
set CFLAGS " -I ${OPENCV_INCLUDE} -I ./ -D__SDSVHLS__ -std=c++14"
set INFILE "${PRJROOT}/data/test.png"
set WAVE 1


if {![info exists CLKP]} {
  set CLKP 3.2
}

open_project -reset $PROJ

add_files "AccelRice.cpp" -cflags ${CFLAGS} -csimflags ${CFLAGS}
add_files -tb "cfa_comp.cpp" -cflags ${CFLAGS} -csimflags ${CFLAGS}
add_files -tb "main_tb.cpp" -cflags ${CFLAGS} -csimflags ${CFLAGS}
add_files -tb "rice.cpp" -cflags ${CFLAGS} -csimflags ${CFLAGS}
add_files -tb "cfa_comp.cpp" -cflags ${CFLAGS} -csimflags ${CFLAGS}
add_files -tb "timeit.cpp" -cflags ${CFLAGS} -csimflags ${CFLAGS}
set_top Rice_Compress_accel

open_solution -reset $SOLN


set_part $XPART
create_clock -period $CLKP
config_export -description riceaccel -format ip_catalog -library riceaccel -output ip -rtl verilog -vendor NLB -version 1.1

if {$CSIM == 1} {
  csim_design -ldflags "-L ${OPENCV_LIB} -lopencv_imgcodecs -lopencv_imgproc -lopencv_core " -argv " ${INFILE}"
}

if {$CSYNTH == 1} {
  csynth_design
}

if {$COSIM == 1} {
  if {$WAVE} {
    cosim_design -ldflags "-L ${OPENCV_LIB} -lopencv_imgcodecs -lopencv_imgproc -lopencv_core " -argv ${INFILE} -trace_level all -wave_debug
  } else {
    cosim_design -ldflags "-L ${OPENCV_LIB} -lopencv_imgcodecs -lopencv_imgproc -lopencv_core " -argv ${INFILE}
  }
}

if {$VIVADO_SYN == 1} {
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  export_design -flow impl -rtl verilog
}

exit
