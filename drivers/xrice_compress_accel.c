// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.1 (64-bit)
// Tool Version Limit: 2022.04
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xrice_compress_accel.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XRice_compress_accel_CfgInitialize(XRice_compress_accel *InstancePtr, XRice_compress_accel_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XRice_compress_accel_Start(XRice_compress_accel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRice_compress_accel_ReadReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XRice_compress_accel_WriteReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XRice_compress_accel_IsDone(XRice_compress_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRice_compress_accel_ReadReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XRice_compress_accel_IsIdle(XRice_compress_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRice_compress_accel_ReadReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XRice_compress_accel_IsReady(XRice_compress_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRice_compress_accel_ReadReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XRice_compress_accel_EnableAutoRestart(XRice_compress_accel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRice_compress_accel_WriteReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XRice_compress_accel_DisableAutoRestart(XRice_compress_accel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRice_compress_accel_WriteReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_AP_CTRL, 0);
}

u32 XRice_compress_accel_Get_return(XRice_compress_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRice_compress_accel_ReadReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_AP_RETURN);
    return Data;
}
void XRice_compress_accel_Set_indata(XRice_compress_accel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRice_compress_accel_WriteReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_INDATA_DATA, (u32)(Data));
    XRice_compress_accel_WriteReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_INDATA_DATA + 4, (u32)(Data >> 32));
}

u64 XRice_compress_accel_Get_indata(XRice_compress_accel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRice_compress_accel_ReadReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_INDATA_DATA);
    Data += (u64)XRice_compress_accel_ReadReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_INDATA_DATA + 4) << 32;
    return Data;
}

void XRice_compress_accel_Set_outdata(XRice_compress_accel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRice_compress_accel_WriteReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_OUTDATA_DATA, (u32)(Data));
    XRice_compress_accel_WriteReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_OUTDATA_DATA + 4, (u32)(Data >> 32));
}

u64 XRice_compress_accel_Get_outdata(XRice_compress_accel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRice_compress_accel_ReadReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_OUTDATA_DATA);
    Data += (u64)XRice_compress_accel_ReadReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_OUTDATA_DATA + 4) << 32;
    return Data;
}

void XRice_compress_accel_Set_insize(XRice_compress_accel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRice_compress_accel_WriteReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_INSIZE_DATA, Data);
}

u32 XRice_compress_accel_Get_insize(XRice_compress_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRice_compress_accel_ReadReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_INSIZE_DATA);
    return Data;
}

void XRice_compress_accel_Set_k(XRice_compress_accel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRice_compress_accel_WriteReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_K_DATA, Data);
}

u32 XRice_compress_accel_Get_k(XRice_compress_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRice_compress_accel_ReadReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_K_DATA);
    return Data;
}

void XRice_compress_accel_InterruptGlobalEnable(XRice_compress_accel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRice_compress_accel_WriteReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_GIE, 1);
}

void XRice_compress_accel_InterruptGlobalDisable(XRice_compress_accel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRice_compress_accel_WriteReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_GIE, 0);
}

void XRice_compress_accel_InterruptEnable(XRice_compress_accel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XRice_compress_accel_ReadReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_IER);
    XRice_compress_accel_WriteReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_IER, Register | Mask);
}

void XRice_compress_accel_InterruptDisable(XRice_compress_accel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XRice_compress_accel_ReadReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_IER);
    XRice_compress_accel_WriteReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_IER, Register & (~Mask));
}

void XRice_compress_accel_InterruptClear(XRice_compress_accel *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    //XRice_compress_accel_WriteReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_ISR, Mask);
}

u32 XRice_compress_accel_InterruptGetEnabled(XRice_compress_accel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XRice_compress_accel_ReadReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_IER);
}

u32 XRice_compress_accel_InterruptGetStatus(XRice_compress_accel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    // Current Interrupt Clear Behavior is Clear on Read(COR).
    return XRice_compress_accel_ReadReg(InstancePtr->Control_BaseAddress, XRICE_COMPRESS_ACCEL_CONTROL_ADDR_ISR);
}

