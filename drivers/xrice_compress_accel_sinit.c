// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.1 (64-bit)
// Tool Version Limit: 2022.04
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#include "xparameters.h"
#include "xrice_compress_accel.h"

extern XRice_compress_accel_Config XRice_compress_accel_ConfigTable[];

XRice_compress_accel_Config *XRice_compress_accel_LookupConfig(u16 DeviceId) {
	XRice_compress_accel_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XRICE_COMPRESS_ACCEL_NUM_INSTANCES; Index++) {
		if (XRice_compress_accel_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XRice_compress_accel_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XRice_compress_accel_Initialize(XRice_compress_accel *InstancePtr, u16 DeviceId) {
	XRice_compress_accel_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XRice_compress_accel_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XRice_compress_accel_CfgInitialize(InstancePtr, ConfigPtr);
}

#endif

