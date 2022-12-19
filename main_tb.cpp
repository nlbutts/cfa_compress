#include <cstdio>
#include <stdlib.h>
#include <string.h>
#include <chrono>
#include <string>
#include "opencv2/opencv.hpp"

#include "rice.h"
#include "cfa_comp.h"
#include "timeit.h"

#ifdef __SDSVHLS__
#include "AccelRice.h"
#endif

#ifdef HW_IMPLEMENTATION
#include "HWAccelRice.h"
#endif

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

    Rice rice;
    CfaComp cfaComp(rice);

    cv::Mat inimg;
    cv::Mat outimg;
    inimg = cv::imread(argv[1], -1);
    std::vector<uint8_t> comp_data;
    {
        Timeit comptime("Compression time");
        cfaComp.compress(inimg, comp_data);
        comptime.print();
    }
    {
        Timeit comptime("Decompression time");
        cfaComp.decompress(comp_data, outimg);
        comptime.print();
    }

    cv::imwrite("ref.png", outimg);
    cfaComp.compare_images(inimg, outimg);
    cfaComp.save_vector<uint8_t>("comp.cfa", comp_data);

#ifdef __SDSVHLS__
    AccelRice accelrice;
    CfaComp cfaComp2(accelrice);

    comp_data.clear();
    {
        Timeit comptime("Accel Compression time");
        cfaComp2.compress(inimg, comp_data);
        comptime.print();
    }
    {
        Timeit comptime("Decompression time");
        cfaComp.decompress(comp_data, outimg);
        comptime.print();
    }

    cv::imwrite("ref2.png", outimg);
    cfaComp2.compare_images(inimg, outimg);
    cfaComp2.save_vector<uint8_t>("comp2.cfa", comp_data);

#endif

#ifdef HW_IMPLEMENTATION
    HWAccelRice accelrice;
    CfaComp cfaComp3(accelrice);

    comp_data.clear();
    {
        Timeit comptime("HW Accel Total Compression time");
        cfaComp3.compress(inimg, comp_data);
        comptime.print();
    }
    {
        Timeit comptime("Decompression time");
        cfaComp.decompress(comp_data, outimg);
    }

    cv::imwrite("ref2.png", outimg);
    cfaComp3.compare_images(inimg, outimg);
    cfaComp3.save_vector<uint8_t>("comp3.cfa", comp_data);

#endif

    return result;
}

