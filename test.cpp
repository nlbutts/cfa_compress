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
        uint8_t * accelva = (uint8_t*)mmap(NULL,
                                           4096,
                                           PROT_READ | PROT_WRITE,
                                           MAP_SHARED, accel, 0xA0040000);
        printf("Mapped address of %p\n", accelva);

        // std::vector<uint8_t> dst;
        // dst.reserve(4096);
        uint8_t * dst = new uint8_t[4096];

        int fifo = open("/dev/axis_fifo_0x00000000a0010000", O_RDWR);
        if (fifo > 0)
        {
            ssize_t size;
            //size = read(fifo, dst, 4096);
            // printf("Clearing FIFO with a read, returned :%d\n", (int)size);
            int len = (diff.size() * 2 / 4) * 4;
            printf("dev node opened successfully, writing %d bytes\n", len);

            uint8_t cr = *accelva;
            printf("Rice accelerator CR: %02X\n", cr);
            // Set k
            *(accelva + 0x20) = 7;
            *(accelva + 0x18) = len;
            *accelva = 0x1;
            cr = *accelva;
            printf("Rice accelerator CR: %02X\n", cr);

            Timeit t("Rice encoding");
            write(fifo, diff.data(), len);
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
            cr = *accelva;
            printf("Rice accelerator CR: %02X\n", cr);
        }
        else
        {
            printf("Can't open handle to axis fifo\n");
        }
    }


    return 0;
}

