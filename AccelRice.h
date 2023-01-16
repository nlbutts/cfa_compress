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
        * @param imgdata a pointer to the pointer to the image data
        * @param outdata a pre-allocated buffer for the output data,
        *                this must be at least width * height bytes
        * @param width the width of the image in pixels
        * @param height the height of the image in pixels
        * @return vector of vectors of uint8_t containing channel data
        */
    uint32_t compress(const uint16_t * imgdata,
                      uint8_t * outdata,
                      uint32_t width,
                      uint32_t height);

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

