// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.1 (64-bit)
// Tool Version Limit: 2022.04
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef XRICE_COMPRESS_ACCEL_H
#define XRICE_COMPRESS_ACCEL_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/
#ifndef __linux__
#include "xil_types.h"
#include "xil_assert.h"
#include "xstatus.h"
#include "xil_io.h"
#else
#include <stdint.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
#endif
#include "xrice_compress_accel_hw.h"

/**************************** Type Definitions ******************************/
#ifdef __linux__
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#else
typedef struct {
    u16 DeviceId;
    u64 Control_BaseAddress;
} XRice_compress_accel_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XRice_compress_accel;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XRice_compress_accel_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XRice_compress_accel_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XRice_compress_accel_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XRice_compress_accel_ReadReg(BaseAddress, RegOffset) \
    *(volatile u32*)((BaseAddress) + (RegOffset))

#define Xil_AssertVoid(expr)    assert(expr)
#define Xil_AssertNonvoid(expr) assert(expr)

#define XST_SUCCESS             0
#define XST_DEVICE_NOT_FOUND    2
#define XST_OPEN_DEVICE_FAILED  3
#define XIL_COMPONENT_IS_READY  1
#endif

/************************** Function Prototypes *****************************/
#ifndef __linux__
int XRice_compress_accel_Initialize(XRice_compress_accel *InstancePtr, u16 DeviceId);
XRice_compress_accel_Config* XRice_compress_accel_LookupConfig(u16 DeviceId);
int XRice_compress_accel_CfgInitialize(XRice_compress_accel *InstancePtr, XRice_compress_accel_Config *ConfigPtr);
#else
int XRice_compress_accel_Initialize(XRice_compress_accel *InstancePtr, const char* InstanceName);
int XRice_compress_accel_Release(XRice_compress_accel *InstancePtr);
#endif

void XRice_compress_accel_Start(XRice_compress_accel *InstancePtr);
u32 XRice_compress_accel_IsDone(XRice_compress_accel *InstancePtr);
u32 XRice_compress_accel_IsIdle(XRice_compress_accel *InstancePtr);
u32 XRice_compress_accel_IsReady(XRice_compress_accel *InstancePtr);
void XRice_compress_accel_EnableAutoRestart(XRice_compress_accel *InstancePtr);
void XRice_compress_accel_DisableAutoRestart(XRice_compress_accel *InstancePtr);
u32 XRice_compress_accel_Get_return(XRice_compress_accel *InstancePtr);

void XRice_compress_accel_Set_insize(XRice_compress_accel *InstancePtr, u32 Data);
u32 XRice_compress_accel_Get_insize(XRice_compress_accel *InstancePtr);
void XRice_compress_accel_Set_k(XRice_compress_accel *InstancePtr, u32 Data);
u32 XRice_compress_accel_Get_k(XRice_compress_accel *InstancePtr);

void XRice_compress_accel_InterruptGlobalEnable(XRice_compress_accel *InstancePtr);
void XRice_compress_accel_InterruptGlobalDisable(XRice_compress_accel *InstancePtr);
void XRice_compress_accel_InterruptEnable(XRice_compress_accel *InstancePtr, u32 Mask);
void XRice_compress_accel_InterruptDisable(XRice_compress_accel *InstancePtr, u32 Mask);
void XRice_compress_accel_InterruptClear(XRice_compress_accel *InstancePtr, u32 Mask);
u32 XRice_compress_accel_InterruptGetEnabled(XRice_compress_accel *InstancePtr);
u32 XRice_compress_accel_InterruptGetStatus(XRice_compress_accel *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
