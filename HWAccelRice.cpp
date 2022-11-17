#include <stdlib.h>
#include "HWAccelRice.h"

#define PAGE_ALIGN(addr) (((addr / 4096) + 1) * 4096)

HWAccelRice::HWAccelRice()
{
    int err = XRice_compress_accel_Initialize(&_inst, "rice-accel-uio");
    if (err != XST_SUCCESS )
    {
        printf("Failed to initialize rice-accel-uio with error: %d\n", err);
        assert(true);
        return;
    }

    _fd = open(_inst.devnode, O_RDONLY);
    if (_fd <= 0)
    {
        printf("Failed to open uio dev node\n");
        assert(true);
        return;
    }
}

HWAccelRice::~HWAccelRice()
{

}

int HWAccelRice::compress( std::vector<int16_t> &in,
                           std::vector<uint8_t> &out)
{
    volatile int16_t * src = (volatile int16_t *)_inst.dmabuf_virt_addr;
    for (int i = 0; i < in.size(); i++)
    {
        src[i] = in[i];
    }

    int offset = PAGE_ALIGN(in.size() * 2);
    XRice_compress_accel_Set_indata(&_inst, _inst.dmabuf_phy_addr);
    XRice_compress_accel_Set_outdata(&_inst, _inst.dmabuf_phy_addr + offset);
    XRice_compress_accel_Set_k(&_inst, 7);
    XRice_compress_accel_Set_insize(&_inst, in.size() * 2);
    XRice_compress_accel_InterruptGlobalEnable(&_inst);
    XRice_compress_accel_InterruptEnable(&_inst, 1);

    XRice_compress_accel_Start(&_inst);

    u32 count;
    read(_fd, &count, 4);

    u32 compsize = XRice_compress_accel_Get_return(&_inst);
    printf("Compressed size: %d bytes count: %d\n", compsize, count);

    volatile uint8_t * compdata = (volatile uint8_t*)_inst.dmabuf_virt_addr;
    out.clear();
    for (int i = 0; i < compsize; i++)
    {
        uint8_t data = compdata[i + offset];
        out.push_back(data);
    }

    return compsize;
}

void HWAccelRice::decompress( std::vector<uint8_t> &in,
                            std::vector<int16_t> &out,
                            uint32_t uncompressedSize)
{
    // Do nothing. Use the CPU implementation
    printf("ERROR: This should not be called\n");
    assert(true);
}
