#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include "opencv2/opencv.hpp"
//#include "opencv2/imgproc/imgproc.hpp"
//#include "opencv2/highgui/highgui.hpp"
//#include "opencv2/imgcodecs/imgcodecs.hpp"

#include "rice.h"
#include <boost/crc.hpp>
#include "bayer_comp_struct.h"
//#include "bayer_comp_accel.hpp"

/**
 * @brief Appends a channel of data to the compressed image format.
 *
 * +------------------------+
 * |  Header                |
 * |                        |
 * +------------------------+
 * | Compressed Ch1         |
 * |                        |
 * |                        |
 * +------------------------+
 * | Compressed Ch2         |
 * |                        |
 * |                        |
 * +------------------------+
 * | Compressed Ch3         |
 * |                        |
 * |                        |
 * +------------------------+
 * | Compressed Ch4         |
 * |                        |
 * |                        |
 * +------------------------+
 *
 * @param pkg a vector to append data to
 * @param comp_data pointer to the compressed data
 * @param comp_size size of the compressed data
 */
void append_comp_channel(std::vector<uint8_t> &pkg, std::vector<uint8_t> &comp_data)
{
    pkg.insert(pkg.end(), comp_data.begin(), comp_data.end());
    // for (int i = 0; i < comp_size; i++)
    // {
    //     pkg.push_back(comp_data[i]);
    // }
}

/**
 * @brief Debug function to save a vector to a file
 * I hate C++ file I/O, hence the C style file I/O
 *
 * @tparam T vector type
 * @param filename name of the file to save
 * @param pkg reference to a vector
 */
template<typename T>
void save_vector(std::string filename, std::vector<T> &pkg)
{
    printf("Saving vector to file %s\n", filename.c_str());
    FILE * f = fopen(filename.c_str(), "wb");
    fwrite(pkg.data(), sizeof(pkg[0]), pkg.size(), f);
    fclose(f);
}

/**
 * @brief Compress an image using a reference implementation
 *
 * @param img the image to compress
 * @return std::vector<uint8_t> a vector of compress data
 */
std::vector<uint8_t> compress_ref(cv::Mat &img)
{
    int total_pixels = img.rows * img.cols;
    std::vector<int16_t> channels[4];
    std::vector<uint8_t> comp_data[4];
    // uint8_t * comp_data[4];
    // comp_data[0] = (uint8_t*)malloc(total_pixels/2);
    // comp_data[1] = (uint8_t*)malloc(total_pixels/2);
    // comp_data[2] = (uint8_t*)malloc(total_pixels/2);
    // comp_data[3] = (uint8_t*)malloc(total_pixels/2);

    // Split the bayer image into RGGB planes
    uint16_t prev_pixel[4];
    for (int y = 0; y < img.rows; y++)
    {
        for (int x = 0; x < img.cols; x++)
        {
            int index = ((y & 1) << 1) + (x & 1);
            uint16_t pixel = img.at<uint16_t>(y, x);
            if (((y == 0) && (x <= 1)) || ((y == 1) && (x <= 1)))
            {
                channels[index].push_back(pixel);
                prev_pixel[index] = pixel;
            }
            else
            {
                channels[index].push_back(pixel - prev_pixel[index]);
                prev_pixel[index] = pixel;
            }
        }
    }

    Rice rice;

    // Now compress
    int comp_size[4];
    for (int ch = 0; ch < 4; ch++)
    {
        comp_size[ch] = rice.compress(channels[ch], comp_data[ch]);
        // comp_size[ch] = Rice_Compress((void*)channels[ch].data(),
        //                               comp_data[ch],
        //                               channels[ch].size() * 2,
        //                               RICE_FMT_INT16);
        printf("size: %d  comp_size: %d\n",
                (int)channels[ch].size(),
                comp_size[ch]);
    }

    BayerComp header;
    header.type[0] = 'C';
    header.type[1] = 'F';
    header.type[2] = 'A';
    header.type[3] = 'C';
    header.type[4] = '0';
    header.type[5] = '0';
    header.type[6] = '1';
    header.channels = 4;
    boost::crc_32_type crc;
    crc.process_bytes(img.data, total_pixels * 2);
    header.crc = crc();
    header.width = img.cols;
    header.height = img.rows;
    header.channel_size[0] = comp_size[0];
    header.channel_size[1] = comp_size[1];
    header.channel_size[2] = comp_size[2];
    header.channel_size[3] = comp_size[3];
    auto src = (uint8_t*)&header;
    std::vector<uint8_t> pkg(src, src + sizeof(header));

    append_comp_channel(pkg, comp_data[0]);
    append_comp_channel(pkg, comp_data[1]);
    append_comp_channel(pkg, comp_data[2]);
    append_comp_channel(pkg, comp_data[3]);

    save_vector<uint8_t>("comp.cfa", pkg);

    return pkg;
}

/**
 * @brief Checks to make sure the header is correct.
 *
 * @param header pointer to the header
 * @return true header is good
 * @return false header is bad
 */
bool verifyHeader(BayerComp * header)
{
    bool result = false;
    if ((header->type[0] == 'C') &&
        (header->type[1] == 'F') &&
        (header->type[2] == 'A') &&
        (header->type[3] == 'C') &&
        (header->type[4] == '0') &&
        (header->type[5] == '0') &&
        (header->type[6] == '1') &&
        (header->channels == 4))
    {
        result = true;
        printf("Header is good\n");
    }
    else
    {
        printf("Header is bad\n");
    }

    return result;
}

/**
 * @brief Converts from a difference data to a int16
 *
 * @param data pointer to the data
 * @param size size of the data in bytes
 */
//void diff_to_int16(int16_t * data, int size)
void diff_to_int16(std::vector<int16_t> &data)
{
    for (int i = 1; i < data.size(); i++)
    {
        data[i] = data[i - 1] + data[i];
    }
}

/**
 * @brief Decompress a compressed image
 *
 * @param comp_data vector to the compressed data
 * @param filename filename to use to save the image
 * @return cv::Mat This function returns the decompressed image
 */
cv::Mat decompress_ref(std::vector<uint8_t> &comp_data, std::string filename)
{
    cv::Mat img;
    BayerComp * header = (BayerComp *)comp_data.data();

    Rice rice;

    if (verifyHeader(header))
    {
        auto src = comp_data.data() + sizeof(BayerComp);
        //int16_t * decomp_data[4];
        std::vector<int16_t> decomp_data[4];
        int uncompressed_size = header->width * header->height * 2 / 4;
        for (int i = 0; i < header->channels; i++)
        {
            std::vector<uint8_t> temp(src, src + header->channel_size[i]);
            //decomp_data[i] = (int16_t*)malloc(uncompressed_size);
            rice.decompress(temp, decomp_data[i], uncompressed_size);
            // Rice_Uncompress(src,
            //                 decomp_data[i],
            //                 header->channel_size[i],
            //                 uncompressed_size,
            //                 RICE_FMT_INT16);
            diff_to_int16(decomp_data[i]);
            src += header->channel_size[i];
            std::stringstream ss;
            ss << "test";
            ss << i;
            ss << ".bin";
            save_vector<int16_t>(ss.str(), decomp_data[i]);
        }

        printf("Assembling image\n");
        img.create(header->height, header->width, CV_16UC1);

        uint16_t * ptr = (uint16_t*)img.data;
        int offset = header->width / 2;
        for (int y = 0; y < header->height; y++)
        {
            for (int x = 0; x < header->width; x++)
            {
                int index = ((y & 1) << 1) + (x & 1);
                *ptr = decomp_data[index][((y >> 1) * offset) + (x >> 1)];
                ptr++;
            }
        }

        cv::imwrite("ref.png", img);
    }
    return img;
}

/**
 * @brief Compare two images against each other.
 * This assumes the images are the same data type, width, and height.
 * If this assumption is broken, this code will most likely throw an exception
 *
 * @param img1 first image
 * @param img2 second image
 * @return int 0 if good, non zero if not.
 */
int compare_images(cv::Mat img1, cv::Mat img2)
{
    cv::Mat temp1;
    cv::Mat temp2;
    img1.convertTo(temp1, CV_32F);
    img2.convertTo(temp2, CV_32F);
    int result = -1;
    cv::Mat diff;
    cv::subtract(temp1, temp2, diff);
    auto sum = cv::sum(diff);
    printf("Sum of diff: %f\n", sum.val[0]);
    if ((-0.1 < sum.val[0]) && (sum.val[0] < 0.1))
    {
        printf("IMAGES MATCH\n");
        result = 0;
    }
    else
    {
        printf("FAILURE: Images are different\n");
    }
    return result;
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

    int size;
    cv::Mat img = cv::imread(argv[1], -1);

    if (!img.empty())
    {
        auto comp_data = compress_ref(img);
        auto ref = decompress_ref(comp_data, "ref.png");
        result = compare_images(img, ref);
    }
    else
    {
        printf("ERROR: Can't open file\n");
        result = -1;
    }

    // hls::stream<int16_t> indata;
    // hls::stream<uint8_t> outdata;
    // size = 100;
    // for (int i = 0; i < size; i++)
    // {
    //     indata.write(data[i]);
    // }

    // //int compsize = Rice_Compress_accel((int16_t*)data, compdata, size, 7);
    // int compsize = Rice_Compress_accel(indata, outdata, size, 7);

    // for (int i = 0; i < compsize; i++)
    // {
    //     compdata[i] = outdata.read();
    // }

    // printf("Input size: %d  output size: %d\n", size, compsize);

    // FILE * f = fopen("comp.rice", "wb");
    // fwrite(compdata, 1, compsize, f);
    // fclose(f);

    // uint8_t * golden = (uint8_t*)open_file(argv[2], &size);
    // uint8_t * ptr = &golden[0x0d];
    // int result = memcmp(ptr, compdata, compsize - 1);
    // printf("Memory compare result: %d\n", result);

    return result;
}

