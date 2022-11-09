#include <cstdio>
#include <stdlib.h>
#include "opencv2/opencv.hpp"
#include <chrono>
#include <thread>

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "timeit.h"

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
    std::vector<int16_t> diff;
    printf("Data: ");
    for (int y = 0; y < rows; y += 2)
    {
        for (int x = 0; x < cols; x += 2)
        {
            buf.push_back(inimg.at<uint16_t>(y, x));
        }
    }

    printf("Data size: %d words\n", (int)buf.size());
    //printf("%d ", buf[0]);
    diff.push_back(buf[0]);
    for (int i = 1; i < buf.size(); i++)
    {
        int16_t temp = buf[i] - buf[i-1];
        diff.push_back(temp);
        //printf("%04X ", temp);
    }
    printf("\n");

    int accel = open("/dev/mem", O_RDWR);
    printf("Open /dev/mem: %d\n", accel);
    if (accel > 0)
    {
        printf("Attempt to mmap accelerator\n");
        volatile uint32_t * accelva = (uint32_t*)mmap(NULL,
                                             4096,
                                             PROT_READ | PROT_WRITE,
                                             MAP_SHARED, accel, 0xA0040000);
        printf("Mapped address of %p\n", accelva);

        // std::vector<uint8_t> dst;
        // dst.reserve(4096);
        uint8_t * dst = new uint8_t[4096];

        int fifo = open("/dev/axis_fifo_0x00000000a0010000", O_RDWR);
        *accelva = 0;
        dump_stats(accelva);
        *accelva = 0;
        dump_stats(accelva);
        ssize_t size = read(fifo, dst, 4096);
        printf("Flushed FIFO with %d bytes\n", (int)size);
        if (fifo > 0)
        {
            //size = read(fifo, dst, 4096);
            // printf("Clearing FIFO with a read, returned :%d\n", (int)size);
            int len = (diff.size() * 2 / 4) * 4;
            printf("dev node opened successfully, writing %d bytes\n", len);

            // Set k
            *(accelva + 8) = 7;
            *(accelva + 6) = len;
            *accelva = 0x1;
            dump_stats(accelva);

            int block_len = 16 * 4;
            int words_len = (diff.size() * 2) / 4;
            int tx_blocks = words_len / block_len;
            int remaining_words = words_len - (tx_blocks * block_len);

            printf("Sending %d bytes %d words\n", (int)diff.size() * 2, words_len);
            printf("Tx blocks %d remaining words %d\n", tx_blocks, remaining_words);

            Timeit t("Rice encoding");

            int32_t * src = (int32_t*)diff.data();
            for (int i = 0; i < tx_blocks; i++, src += block_len)
            {
                printf("Txing block %d of size %d blocks\n", i, block_len);
                write(fifo, src, block_len);
                size = read(fifo, dst, 4096);
                printf("Received %d bytes\n", (int)size);
                dump_stats(accelva);
            }
            src += block_len;
            write(fifo, src, remaining_words);
            while (1)
            {
                size = read(fifo, dst, 4096);
                t.print();
                printf("Received %d bytes\n", (int)size);
                if (size > 0)
                {
                    // for (int i = 0; i < size; i++)
                    // {
                    //     printf("%02x ", (int)dst[i]);
                    // }
                    break;
                }
            }

            close(fifo);
            printf("Wrote data in chunks of 32-bits\n");

            int bytes_proc = *(accelva + 10);
            printf("Bytes processed: %d\n", bytes_proc);
        }
        else
        {
            printf("Can't open handle to axis fifo\n");
        }
    }


    return 0;
}

