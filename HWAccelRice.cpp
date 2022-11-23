#include <stdlib.h>
#include "HWAccelRice.h"
#include "timeit.h"

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
    memcpy((void*)_inst.dmabuf_virt_addr, in.data(), in.size() * 2);

    Timeit t2("HW Rice Compress");

    int offset = PAGE_ALIGN(in.size() * 2);
    XRice_compress_accel_Set_indata(&_inst, _inst.dmabuf_phy_addr);
    XRice_compress_accel_Set_outdata(&_inst, _inst.dmabuf_phy_addr + offset);
    XRice_compress_accel_Set_k(&_inst, 7);
    XRice_compress_accel_Set_insize(&_inst, in.size() * 2);
    XRice_compress_accel_InterruptGlobalEnable(&_inst);
    XRice_compress_accel_InterruptEnable(&_inst, 1);

    XRice_compress_accel_Start(&_inst);

    u32 count;
    (void)read(_fd, &count, 4);

    t2.print();

    XRice_compress_accel_Set_k(&_inst, 7);
    u32 compsize = XRice_compress_accel_Get_return(&_inst);
    printf("Compressed size: %d bytes count: %d\n", compsize, count);

    uint8_t * compdata = (uint8_t*)_inst.dmabuf_virt_addr + offset;
    out.clear();
    std::copy(&compdata[0], &compdata[compsize], std::back_inserter(out));

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
