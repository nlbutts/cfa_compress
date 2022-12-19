#ifndef _HW_ACCELRICE_H_
#define _HW_ACCELRICE_H_

#include <vector>
#include <stdint.h>
#include "IRice.h"
#include "drivers/xrice_compress_accel.h"
#include <xrt/xrt_device.h>

/**
 * @brief This is the abstract base class for the Rice implementation.
 * At run time we can switch this out for the CPU implementation or the
 * FPGA accelerated implementation.
 *
 */
class HWAccelRice : public IRice
{
public:
    HWAccelRice();
    virtual ~HWAccelRice();

    /**
     * @brief This is the rice compression method. It takes a signed int16
     * input data and generates the compressed output.
     *
     * @param imgdata a pointer to the pointer to the image data
     * @param width the width of the image in pixels
     * @param height the height of the image in pixels
     * @return vector of vectors of uint8_t containing channel data
     */
    std::vector<std::vector<uint8_t> > compress(uint16_t * imgdata,
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
private:
    XRice_compress_accel    _inst;
    int                     _fd;
};

#endif // _HW_ACCELRICE_H_

