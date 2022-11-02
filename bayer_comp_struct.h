#ifndef _BAYER_COMP_STRUCT_H_
#define _BAYER_COMP_STRUCT_H_

struct BayerComp
{
    char type[7];   // CFAC001
    uint8_t channels;
    uint32_t crc;
    uint16_t width;
    uint16_t height;
    uint32_t channel_size[4];
    // Channel data after channel size
};

#endif // _BAYER_COMP_STRUCT_H_

