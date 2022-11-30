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

    _fd = open("/dev/uio4", O_RDONLY);
    if (_fd <= 0)
    {
        printf("Failed to open uio dev node\n");
        assert(true);
        return;
    }

    _fifo = open("/dev/axis_fifo_0x00000000a0000000", O_RDWR);
    if (_fifo <= 0)
    {
        printf("Failed to open the FIFO");
        assert(true);
        return;
    }
}

HWAccelRice::~HWAccelRice()
{
    close(_fifo);
    close(_fd);
}

int HWAccelRice::compress( std::vector<int16_t> &in,
                           std::vector<uint8_t> &out)
{
    printf("Starting HW Rice Compress\n");
    Timeit t2("HW Rice Compress");

    XRice_compress_accel_Set_k(&_inst, 7);
    XRice_compress_accel_Set_insize(&_inst, in.size() * 2);
    XRice_compress_accel_InterruptGlobalEnable(&_inst);
    XRice_compress_accel_InterruptEnable(&_inst, 1);
    XRice_compress_accel_Start(&_inst);

    uint8_t buf[100];

    uint8_t * ptr = (uint8_t*)in.data();
    for (int i = 0; i < in.size(); i += 512)
    {
        write(_fifo, &ptr[i], 512);
    }

    // Wait for the Accel to be done
    printf("Waiting for Rice Accel to complete\n");
    u32 count;
    (void)read(_fd, &count, 4);

    t2.print();

    XRice_compress_accel_Set_k(&_inst, 7);
    u32 compsize = XRice_compress_accel_Get_return(&_inst);
    printf("Compressed size: %d bytes count: %d\n", compsize, count);

    for (int i = 0; i < compsize; i++)
    {
        uint8_t buf;
        int rsize = read(_fifo, &buf, 1);
        if (rsize == 1)
        {
            out.push_back(buf);
        }
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
