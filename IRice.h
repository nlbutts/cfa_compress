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
     * @param in a reference to the int16 input data
     * @param out a reference to the uint8 output data
     * @return int the size of the output data
     */
    virtual int compress( std::vector<int16_t> &in,
                          std::vector<uint8_t> &out) = 0;

    /**
     * @brief This is the rice decompression method. It takes the compressed
     * bit stream and produces the uncomressed data.
     *
     * @param in a reference to the compressed bit stream
     * @param out a reference to the int16 output data
     * @param uncompressedSize The size of the uncompressed data
     */
    virtual void decompress( std::vector<uint8_t> &in,
                             std::vector<int16_t> &out,
                             uint32_t uncompressedSize) = 0;


protected:
    // Abstract base class
    IRice() {};
};

#endif // _IRice_H_

