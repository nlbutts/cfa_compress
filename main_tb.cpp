#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <string.h>
//#include "opencv2/opencv.hpp"

#include "rice.h"
#include "cfa_comp.h"
//#include "bayer_comp_accel.hpp"


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
    cfaComp.compress(inimg, comp_data);
    cfaComp.decompress(comp_data, outimg);

    cv::imwrite("ref.png", outimg);
    cfaComp.compare_images(inimg, outimg);
    cfaComp.save_vector<uint8_t>("comp.cfa", comp_data);

    // int size;
    // cv::Mat img = cv::imread(argv[1], -1);

    // if (!img.empty())
    // {
    //     auto comp_data = compress_ref(img);
    //     auto ref = decompress_ref(comp_data, "ref.png");
    //     result = compare_images(img, ref);
    // }
    // else
    // {
    //     printf("ERROR: Can't open file\n");
    //     result = -1;
    // }

    return result;
}

