#ifndef _CFA_COMP_H_
#define _CFA_COMP_H_

#include <opencv2/opencv.hpp>
#include "IRice.h"
#include "cfa_comp_struct.h"

class CfaComp
{
public:
    /**
     * @brief Construct a new Cfa Comp object
     *
     * @param compressor the compressor to use
     * @param debug true to enable additional debug output
     */
    CfaComp(IRice & compressor, bool debug = false);
    ~CfaComp();

    /**
     * @brief Compress an image using the passed in Rice compressor
     *
     * @param img the image to compress
     * @param compimg compressed image
     * @return int size of the compressed data
     */
    int compress(cv::Mat &img, std::vector<uint8_t> &compimg);

    /**
     * @brief Decompress a file and produce an image
     *
     * @param compimg compressed image data
     * @param outimg the output image
     * @return int 0 if successful otherwise an error code
     */
    int decompress(std::vector<uint8_t> &compimg, cv::Mat &outimg);

    /**
     * @brief Compare two images against each other.
     * This assumes the images are the same data type, width, and height.
     * If this assumption is broken, this code will most likely throw an exception
     *
     * @param img1 first image
     * @param img2 second image
     * @return int 0 if good, non zero if not.
     */
    int compare_images(cv::Mat img1, cv::Mat img2);

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

private:
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
    void append_comp_channel(std::vector<uint8_t> &pkg, std::vector<uint8_t> &comp_data);

    /**
     * @brief Verify the header
     *
     * @param header pointer to the header
     * @return true header is good
     * @return false header is bad
     */
    bool verifyHeader(CfaCompData * header);

    /**
     * @brief Converts from a difference data to a int16
     *
     * @param data pointer to the data
     * @param size size of the data in bytes
     */
    void diff_to_int16(std::vector<int16_t> &data);

    /**
     * @brief Loads a binary file into a vector
     *
     * @param filename name of the input file
     * @param data reference to a vector to store the data
     * @return int 0 if succesful, otherwise error code
     */
    int load_file(std::string filename, std::vector<uint8_t> &data);

private:
    IRice * _rice;  // Callee owns this
    bool    _debug; // Enable debug output
};

#endif //_CFA_COMP_H_