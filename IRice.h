#ifndef _IRice_H_
#define _IRice_H_

#include <vector>
#include <stdint.h>

/**
 * @brief This is the abstract base class for the Rice implementation.
 * At run time we can switch this out for the CPU implementation or the
 * FPGA accelerated implementation.
 *
 */
class IRice
{
public:
    virtual ~IRice() {};
    /**
     * @brief This is the rice compression method. It takes a signed int16
     * input data and generates the compressed output.
     *
     * @param img a reference to an OpenCV image
     * @return vector of vectors of uint8_t containing channel data
     */
    virtual std::vector<std::vector<uint8_t> > compress( cv::Mat &img) = 0;

    /**
     * @brief This is the rice decompression method. It takes the compressed
     * bit stream and produces the uncomressed data.
     *
     * @param in a reference to the compressed bit stream
     * @param uncompressedSize The size of the uncompressed data
     * @return cv::Mat the reconstructed image
     */
    virtual cv::Mat decompress( std::vector<uint8_t> &in,
                                uint32_t uncompressedSize) = 0;


protected:
    // Abstract base class
    IRice() {};
};

#endif // _IRice_H_

