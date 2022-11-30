// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.1 (64-bit)
// Tool Version Limit: 2022.04
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
// control
// 0x00 : Control signals
//        bit 0  - ap_start (Read/Write/COH)
//        bit 1  - ap_done (Read/COR)
//        bit 2  - ap_idle (Read)
//        bit 3  - ap_ready (Read/COR)
//        bit 7  - auto_restart (Read/Write)
//        bit 9  - interrupt (Read)
//        others - reserved
// 0x04 : Global Interrupt Enable Register
//        bit 0  - Global Interrupt Enable (Read/Write)
//        others - reserved
// 0x08 : IP Interrupt Enable Register (Read/Write)
//        bit 0 - enable ap_done interrupt (Read/Write)
//        bit 1 - enable ap_ready interrupt (Read/Write)
//        others - reserved
// 0x0c : IP Interrupt Status Register (Read/COR)
//        bit 0 - ap_done (Read/COR)
//        bit 1 - ap_ready (Read/COR)
//        others - reserved
// 0x10 : Data signal of ap_return
//        bit 31~0 - ap_return[31:0] (Read)
// 0x18 : Data signal of insize
//        bit 31~0 - insize[31:0] (Read/Write)
// 0x1c : reserved
// 0x20 : Data signal of k
//        bit 31~0 - k[31:0] (Read/Write)
// 0x24 : reserved
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

#define XRICE_COMPRESS_ACCEL_CONTROL_ADDR_AP_CTRL     0x00
#define XRICE_COMPRESS_ACCEL_CONTROL_ADDR_GIE         0x04
#define XRICE_COMPRESS_ACCEL_CONTROL_ADDR_IER         0x08
#define XRICE_COMPRESS_ACCEL_CONTROL_ADDR_ISR         0x0c
#define XRICE_COMPRESS_ACCEL_CONTROL_ADDR_AP_RETURN   0x10
#define XRICE_COMPRESS_ACCEL_CONTROL_BITS_AP_RETURN   32
#define XRICE_COMPRESS_ACCEL_CONTROL_ADDR_INSIZE_DATA 0x18
#define XRICE_COMPRESS_ACCEL_CONTROL_BITS_INSIZE_DATA 32
#define XRICE_COMPRESS_ACCEL_CONTROL_ADDR_K_DATA      0x20
#define XRICE_COMPRESS_ACCEL_CONTROL_BITS_K_DATA      32

