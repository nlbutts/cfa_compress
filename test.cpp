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

void dump_stats(volatile uint32_t * addr)
{
    int cr = *addr;
    int k = *(addr + 8);
    int insize = *(addr + 6);
    int outsize = *(addr + 4);
    printf("Rice CR: %02X  k: %d  insize: %d  outsize: %d\n", cr, k, insize, outsize);
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

    if (argc != 2)
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
    XRice_compress_accel_Initialize(&inst, "rice-accel");

    printf("IsDone: %d isIdle: %d isReady: %d\n",
            XRice_compress_accel_IsDone(&inst),
            XRice_compress_accel_IsIdle(&inst),
            XRice_compress_accel_IsReady(&inst));

    printf("K: %d\n", XRice_compress_accel_Get_k(&inst));

    printf("Control Addr: %016lX  dma virt addr: %016lX  dma phy addr: %16lX\n", inst.Control_BaseAddress, inst.dmabuf_virt_addr, inst.dmabuf_phy_addr);

    // Put the data into the DMA buffer
    printf("Pushing data into DMA buffer\n");
    int16_t * src = (int16_t*)(inst.dmabuf_virt_addr);
    src[0] = buf[0];
    printf("%04X ", src[0]);
    for (int i = 1; i < buf.size(); i++)
    {
        int16_t temp = buf[i] - buf[i-1];
        src[i] = temp;
        if (buf.size() < 200)
            printf("%04X ", (int)temp);
    }
    printf("\n");

    printf("Setting pointers in module\n");
    int offset = PAGE_ALIGN(buf.size());
    printf("Offset for out data: %08X\n", offset);
    XRice_compress_accel_Set_indata(&inst, inst.dmabuf_phy_addr);
    XRice_compress_accel_Set_outdata(&inst, inst.dmabuf_phy_addr + offset);
    XRice_compress_accel_Set_k(&inst, 7);
    XRice_compress_accel_Set_insize(&inst, buf.size() * 2);

    XRice_compress_accel_Start(&inst);

    u32 done;
    Timeit t("Rice accel time");
    while (!XRice_compress_accel_IsIdle(&inst)) {};
    t.print();

    u32 compsize = XRice_compress_accel_Get_return(&inst);
    printf("Compressed size: %d\n", compsize);

    uint8_t * dst = (uint8_t*)(inst.dmabuf_virt_addr + offset);
    if (compsize < 200)
    {
        for (int i = 0; i < compsize; i++)
        {
            printf("%02X ", dst[i]);
        }
    }

    XRice_compress_accel_Release(&inst);

    return 0;
}

