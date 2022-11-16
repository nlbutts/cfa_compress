#include <cstdio>
#include <stdlib.h>
#include "opencv2/opencv.hpp"
#include <chrono>
#include <thread>

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "timeit.h"
#include "xrice_compress_accel.h"

#define PAGE_ALIGN(addr) (((addr / 4096) + 1) * 4096)

using namespace std::chrono_literals;

void dump_stats(XRice_compress_accel &inst)
{
    printf("IsDone: %d isIdle: %d isReady: %d\n",
            XRice_compress_accel_IsDone(&inst),
            XRice_compress_accel_IsIdle(&inst),
            XRice_compress_accel_IsReady(&inst));
}

/**
 * @brief This is our friendly neighborhood main
 *
 * @param argc number of arguments
 * @param argv arguments
 * @return int return value
 */
int main(int argc, char ** argv)
{
    int result = 0;

    if (argc < 2)
    {
        printf("Usage: simple_hdr <input image>\n");
        return -1;
    }

    cv::Mat inimg;
    cv::Mat outimg;
    inimg = cv::imread(argv[1], -1);

    int rows = inimg.rows;
    int cols = inimg.cols;

    std::vector<uint16_t> buf;
    printf("Data: ");
    for (int y = 0; y < rows; y += 2)
    {
        for (int x = 0; x < cols; x += 2)
        {
            buf.push_back(inimg.at<uint16_t>(y, x));
        }
    }

    printf("Data size: %d words\n", (int)buf.size());

    XRice_compress_accel inst;
    XRice_compress_accel_Initialize(&inst, "rice-accel-uio");
    dump_stats(inst);
    printf("K: %d\n", XRice_compress_accel_Get_k(&inst));

    printf("Control Addr: %016lX  dma virt addr: %016lX  dma phy addr: %16lX\n", inst.Control_BaseAddress, inst.dmabuf_virt_addr, inst.dmabuf_phy_addr);

    // Put the data into the DMA buffer
    printf("Pushing data into DMA buffer\n");
    int16_t * src = (int16_t*)(inst.dmabuf_virt_addr);

    int size = buf.size();
    std::vector<int16_t> diff;
    diff.push_back(buf[0]);
    printf("%04X ", src[0]);
    for (int i = 1; i < buf.size(); i++)
    {
        int16_t temp = buf[i] - buf[i-1];
        diff.push_back(temp);
        if (i < 64)
            printf("%04X ", (int)(temp & 0xFFFF));
    }
    printf("\n");

    memcpy(src, diff.data(), diff.size()*2);

    int dev = open("/dev/uio4", O_RDONLY);
    int sync = open("/sys/devices/platform/amba_pl@0/a0040000.Rice_Compress_accel/sync", O_WRONLY | O_SYNC);
    if (sync > 0)
    {
        printf("Flushing cache???\n");
        write(sync, "1\n", 2);
    }
    else
    {
        printf("Sync was not opened\n");
    }

    printf("Setting pointers in module\n");
    int offset = PAGE_ALIGN(buf.size()*2);
    printf("Offset for out data: %08X\n", offset);
    XRice_compress_accel_Get_return(&inst);
    XRice_compress_accel_Set_indata(&inst, inst.dmabuf_phy_addr);
    XRice_compress_accel_Set_outdata(&inst, inst.dmabuf_phy_addr + offset);
    XRice_compress_accel_Set_k(&inst, 7);
    XRice_compress_accel_Set_insize(&inst, size * 2);
    XRice_compress_accel_InterruptGlobalEnable(&inst);
    XRice_compress_accel_InterruptEnable(&inst, 1);

    XRice_compress_accel_Start(&inst);

    u32 count;
    read(dev, &count, 4);

    u32 compsize = XRice_compress_accel_Get_return(&inst);
    printf("Compressed size: %d bytes count: %d\n", compsize, count);
    //t.print();

    int outfile = open("comphw.bin", O_CREAT | O_WRONLY);
    if (outfile <= 0)
    {
        printf("Error opening output file\n");
        return -1;
    }

    volatile uint8_t * dst = (uint8_t*)(inst.dmabuf_virt_addr + offset);
    write(outfile, (void*)dst, compsize);
    close(outfile);

    for (int i = 0; i < compsize; i++)
    {
        if (i < 64)
            printf("%02X ", dst[i]);
    }
    printf("\n");

    XRice_compress_accel_Release(&inst);

    return 0;
}

