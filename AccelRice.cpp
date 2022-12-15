#include <stdlib.h>
#include "AccelRice.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include <memory.h>
#include "timeit.h"
#include "opencv2/opencv.hpp"

#define RICE_HISTORY    16
#define RICE_WORD       16
#define BITS            16
#define RICE_THRESHOLD  8

typedef ap_axiu<BITS, 1, 1, 1> pixel;
typedef ap_axiu<8, 1, 1, 1> compdata;

typedef struct {
    ap_uint<4>          hist[ RICE_HISTORY ];
    ap_uint<64>         bits;
    ap_uint<8>          bitcount;
    unsigned int        processed_samples;
    uint16_t            sumk;
    uint16_t            previous_sample;
} rice_bitstream_t;

static void _Rice_init(rice_bitstream_t &config)
{
    init: for (ap_uint<6> i = 0; i < RICE_HISTORY; i++)
    {
        config.hist[i] = 0;
    }

    config.bitcount = 0;
    config.processed_samples = 0;
    config.sumk = 0;
    config.previous_sample = 0;
}

static void _Rice_flush(rice_bitstream_t &config,
                        hls::stream<uint8_t> &outdata)
{
    // Flush the last few bits
    config.bits <<= (8 - config.bitcount);
    outdata.write(config.bits.range(7, 0));
}

/*************************************************************************
* _Rice_NumBits() - Determine number of information bits in a word.
*************************************************************************/

static int _Rice_NumBits( uint16_t x )
{
    uint8_t n = 0;
    if (x)
    {
        n = 32 - __builtin_clz(x);
    }
    return n;
}


/*************************************************************************
* _Rice_EncodeWord() - Encode and write a word to the output stream.
*************************************************************************/

static void _Rice_EncodeWord( ap_uint<16> x,
                              uint16_t k,
                              ap_uint<64>  *bits,
                              ap_uint<8>   *bitcount )
{
	ap_uint<4>  i;
    ap_uint<10> q;
    ap_uint<6>  o;
    ap_uint<16> temp;

    /* Determine overflow */
    q = x >> k;

    temp = 0xFFFF;

    /* Too large rice code? */
    if( q > RICE_THRESHOLD )
    {
        *bits <<= RICE_THRESHOLD;
        bits->range(RICE_THRESHOLD - 1, 0) = temp.range(RICE_THRESHOLD - 1, 0);
        *bitcount += RICE_THRESHOLD;

        /* Encode the overflow with alternate coding */
        q -= RICE_THRESHOLD;

        /* Write number of bits needed to represent the overflow */
        o = _Rice_NumBits( q );

        temp[0] = 0;
        *bits <<= o + 1;
        bits->range(o, 0) = temp.range(o, 0);
        *bitcount += o + 1;

        if (o >= 2)
        {
            *bits <<= (o - 2) + 1;
            bits->range(o - 2, 0) = q.range(o - 2, 0);
            *bitcount += (o - 2) + 1;
        }

        *bits <<= k;
        bits->range(k - 1, 0) = x.range(k - 1, 0);
        *bitcount += k;
    }
    else
    {
        temp[0] = 0;
        *bits <<= q + 1 + k;
        bits->range(q + k, k) = temp.range(q, 0);
        bits->range(k - 1, 0) = x.range(k - 1, 0);
        *bitcount += (q + 1 + k);
    }
}

int Rice_Compress(hls::stream<uint16_t> &indata,
                  hls::stream<uint8_t> &outdata,
                  rice_bitstream_t &config,
                  uint16_t k)
{
    int16_t             sx;
    uint16_t            x;
    uint16_t            sample;
    ap_uint<3>          out_bytes = 0;
    ap_uint<4>          hist_index;

    /* Revise optimum k? */
    if( config.processed_samples >= RICE_HISTORY )
    {
        k = (config.sumk + (RICE_HISTORY >> 1)) >> 4;
    }

    /* Read word from input buffer */
    if (config.processed_samples == 0)
    {
        sx = indata.read();
        config.previous_sample = sx;
    }
    else
    {
        sample = indata.read();
        sx = sample - config.previous_sample;
        config.previous_sample = sample;
    }
    x = sx < 0 ? -1-(sx<<1) : sx<<1;

    config.processed_samples++;

    /* Encode word to output buffer */
    _Rice_EncodeWord( x, k, &config.bits, &config.bitcount);

    /* Update history */

    hist_index = config.processed_samples & (RICE_HISTORY - 1);
    config.sumk -= config.hist[hist_index];
    config.hist[ hist_index ] = _Rice_NumBits( x );
    config.sumk += config.hist[ hist_index ];
    //printf("k: %d sumk: %d\n", k, sumk);
    output: while (config.bitcount > 8)
    {
        #pragma HLS loop_tripcount min=0 max=3 avg=1
        outdata.write(config.bits.range(config.bitcount - 1, config.bitcount - 8));
        config.bitcount -= 8;
        out_bytes++;
    }

    return out_bytes;
}

int Rice_Compress_accel( const uint16_t* indata,
                         uint8_t* outdata,
                         unsigned int total_samples,
                         int k )
{
//#pragma HLS AGGREGATE compact=byte variable=outdata
//#pragma HLS AGGREGATE compact=byte variable=indata
#pragma HLS INTERFACE mode=s_axilite port=return
#pragma HLS INTERFACE mode=s_axilite port=insize
#pragma HLS INTERFACE mode=s_axilite port=k
#pragma HLS DATAFLOW
#pragma HLS INTERFACE m_axi port=indata depth=128 bundle=gem0 max_widen_bitwidth=128 num_read_outstanding=16
#pragma HLS INTERFACE mode=m_axi bundle=gem1 depth=128 max_widen_bitwidth=128 max_write_burst_length=64 num_write_outstanding=16 port=outdata

    uint32_t comp_bytes = 0;
    rice_bitstream_t p1;
    _Rice_init(p1);
    hls::stream<uint16_t> instream;
    hls::stream<uint8_t>  outstream;
    outstream.write(k);
    for (uint32_t i = 0; i < total_samples; i++)
    {
        instream.write(indata[i]);
        int outbytes = Rice_Compress(instream, outstream, p1, k);
        while (!outstream.empty())
        {
            outdata[comp_bytes++] = outstream.read();
        }

    }

    _Rice_flush(p1, outstream);
    while (!outstream.empty())
    {
        outdata[comp_bytes++] = outstream.read();
    }
	return comp_bytes;
}

std::vector<std::vector<uint8_t> > AccelRice::compress( cv::Mat &img)
{
    Timeit t("HW Rice Compress");

    int total_pixels = img.rows * img.cols;
    std::vector<int16_t> channels[4];
    std::vector<uint8_t> comp_data[4];
    int channel_index[4] = {0};

    for (int i = 0; i < 4; i++)
    {
        channels[i].resize(total_pixels / 4);
    }

    // Split the bayer image into RGGB planes
    uint16_t prev_pixel[4];
    for (int y = 0; y < img.rows; y++)
    {
        for (int x = 0; x < img.cols; x++)
        {
            int index = ((y & 1) << 1) + (x & 1);
            uint16_t pixel = img.at<uint16_t>(y, x);
            channels[index][channel_index[index]] = pixel;
            channel_index[index]++;
        }
    }

    // Now compress
    std::vector<std::vector<uint8_t> > outdata;
    int comp_size[4];
    for (int ch = 0; ch < 4; ch++)
    {
        comp_data[ch].resize(total_pixels);
        comp_size[ch] = Rice_Compress_accel((const uint16_t*)channels[ch].data(),
                                            (uint8_t*)comp_data[ch].data(),
                                            channels[ch].size(),
                                            7);
        comp_data[ch].resize(comp_size[ch]);
        outdata.push_back(comp_data[ch]);
        // printf("size: %d  comp_size: %d\n",
        //         (int)channels[ch].size() * 2,
        //         comp_size[ch]);
    }

    // Make out as large as the input data
    t.print();
    return outdata;
}

void AccelRice::decompress( std::vector<uint8_t> &in,
                            std::vector<int16_t> &out,
                            uint32_t uncompressedSize)
{
    // Do nothing. Use the CPU implementation
}
