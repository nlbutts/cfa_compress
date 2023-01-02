#include <stdlib.h>
#include "HWAccelRice.h"
#include "timeit.h"
#include <xrt/xrt_device.h>
#include <xrt/xrt_kernel.h>

#define PAGE_ALIGN(addr) (((addr / 4096) + 1) * 4096)

HWAccelRice::HWAccelRice()
{
}

HWAccelRice::~HWAccelRice()
{

}

uint32_t HWAccelRice::compress(const uint16_t * imgdata,
                               uint8_t * outdata,
                               uint32_t width,
                               uint32_t height)
{
    unsigned int dev_index = 0;
    auto device = xrt::device(dev_index);
    auto uuid = device.load_xclbin("krnl_AccelRice.xclbin");
    std::cout << "device name:     " << device.get_info<xrt::info::device::name>() << "\n";
    std::cout << "device bdf:      " << device.get_info<xrt::info::device::bdf>() << "\n";

    uint32_t bits = 0;

    auto krnl = xrt::kernel(device, uuid, "Rice_Compress_accel");

    auto arg0 = krnl.group_id(0);
    auto arg1 = krnl.group_id(1);

    auto input_buffer = xrt::bo(device, width * height * 2, arg0);
    auto output_buffer = xrt::bo(device, width * height * 2, arg1);

    //auto input_buffer_mapped = input_buffer.map<void*>();
    input_buffer.write((const void*)imgdata);
    //memcpy(input_buffer_mapped, imgdata, width * height * 2);
    input_buffer.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    int offset = width * height / 2;
    auto run = krnl(input_buffer, output_buffer, width, height, offset, 7, bits);
    run.wait();

    printf("Bits: %08X\n", bits);

    auto output_buffer_mapped = output_buffer.map<uint8_t*>();
    output_buffer.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    uint32_t total_comp_size = 0;
    for (int ch = 0; ch < 4; ch++)
    {
        uint8_t * ptr = output_buffer_mapped + (ch * offset);
        uint32_t * size = (uint32_t*)ptr;
        printf("Ch: %d size: %d\n", ch, *size);
        uint8_t * src = &ptr[4];
        uint8_t * end = &ptr[4] + *size;
        // Copy the size
        memcpy(outdata + (ch * 4), size, 4);
        memcpy(outdata + total_comp_size + 16, ptr + 4, *size);
        total_comp_size += *size;
    }

    return total_comp_size;
}

void HWAccelRice::decompress( std::vector<uint8_t> &in,
                            std::vector<int16_t> &out,
                            uint32_t uncompressedSize)
{
    // Do nothing. Use the CPU implementation
    printf("ERROR: This should not be called\n");
    assert(true);
}
