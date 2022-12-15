#ifndef _ACCELRICE_H_
#define _ACCELRICE_H_

#include <vector>
#include <stdint.h>
#include "IRice.h"

/**
 * @brief This is the abstract base class for the Rice implementation.
 * At run time we can switch this out for the CPU implementation or the
 * FPGA accelerated implementation.
 *
 */
class AccelRice : public IRice
{
public:
    AccelRice() {};
    virtual ~AccelRice() {};
    /**
     * @brief This is the rice compression method. It takes a signed int16
     * input data and generates the compressed output.
     *
     * @param img a reference to an OpenCV image
     * @return vector of vectors of uint8_t containing channel data
     */
    std::vector<std::vector<uint8_t> > compress( cv::Mat &img);

    /**
     * @brief THIS IS NOT IMPLEMENTED
     *
     * @param in a reference to the compressed bit stream
     * @param out a reference to the int16 output data
     * @param uncompressedSize The size of the uncompressed data
     */
    void decompress( std::vector<uint8_t> &in,
                     std::vector<int16_t> &out,
                     uint32_t uncompressedSize);
};

#endif // _ACCELRICE_H_

