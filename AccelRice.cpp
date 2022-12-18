#include <stdlib.h>
#include "AccelRice.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include <memory.h>

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
    uint32_t            processed_samples;
    uint16_t            sumk;
    uint16_t            previous_sample;
} rice_bitstream_t;

static void _Rice_init(rice_bitstream_t &config)
{
    init: for (ap_uint<6> i = 0; i < RICE_HISTORY; i++)
    {
        config.hist[i] = 0;
    }

    config.bitcount             = 0;
    config.processed_samples    = 0;
    config.sumk                 = 0;
    config.previous_sample      = 0;
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
        outdata.write(k);
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

void Rice_Compress_accel(const uint16_t* indata,
                         uint8_t* outdata,
                         uint32_t width,
                         uint32_t height,
                         uint32_t out_offset,
                         int k )
{
#pragma HLS INTERFACE mode=s_axilite port=width
#pragma HLS INTERFACE mode=s_axilite port=height
#pragma HLS INTERFACE mode=s_axilite port=out_offset
#pragma HLS INTERFACE mode=s_axilite port=k
//#pragma HLS DATAFLOW
#pragma HLS INTERFACE m_axi port=indata depth=512 bundle=gem0 num_read_outstanding=16
#pragma HLS INTERFACE mode=m_axi bundle=gem1 depth=512 max_write_burst_length=64 num_write_outstanding=16 port=outdata

    rice_bitstream_t config[4];
    _Rice_init(config[0]);
    _Rice_init(config[1]);
    _Rice_init(config[2]);
    _Rice_init(config[3]);
    hls::stream<uint16_t> instream;
#pragma HLS stream variable=instream type=fifo depth=64
    hls::stream<uint8_t>  outstream;
#pragma HLS stream variable=outstream type=fifo depth=64
    // The first four bytes are the total size of the compressed data that follows
    uint32_t out_index[4];
    uint32_t offsets[4];
    uint32_t offset;
    ap_uint<128> pixel;

    offsets[0] = 0;
    offsets[1] = out_offset;
    offsets[2] = out_offset << 1;
    // This is to avoid a multiply
    offsets[3] = offsets[1] + offsets[2];

    init: for (ap_uint<3> i = 0; i < 4; i++)
    {
        out_index[i] = 4;
        _Rice_init(config[i]);
    }

    rows: for (uint32_t y = 0; y < height; y++)
    {
        cols: for (uint32_t x = 0; x < width; x++)
        {
            /*
            This code gives us an index from 0-3.
            We will get a value of 0 and 1 for even and od pixels on even lines
            We will get a value of 2 and 3 for even and odd pixels on odd lines
            */
            int index = ((y & 1) << 1) + (x & 1);
            instream.write(*indata++);
            int outbytes = Rice_Compress(instream, outstream, config[index], k);
            save: while (!outstream.empty())
            {
                offset = out_index[index] + offsets[index];
                outdata[offset] = outstream.read();
                out_index[index]++;
            }
        }
    }

    flush: for (ap_uint<3> i = 0; i < 4; i++)
    {
        _Rice_flush(config[i], outstream);
        while (!outstream.empty())
        {
            offset = out_index[i] + offsets[i];
            outdata[offset] = outstream.read();
            out_index[i]++;
        }

        // This was offset by 4 above for the length word.
        out_index[i] -= 4;

        offset = offsets[i];
        outdata[offset    ] = (out_index[i]      ) & 0xFF;
        outdata[offset + 1] = (out_index[i] >>  8) & 0xFF;
        outdata[offset + 2] = (out_index[i] >> 16) & 0xFF;
        outdata[offset + 3] = (out_index[i] >> 24) & 0xFF;
    }
}

std::vector<std::vector<uint8_t> > AccelRice::compress(uint16_t * imgdata,
                                                       uint32_t width,
                                                       uint32_t height)
{
/*
The input image data should be contiguous. But the compressed will not be.
The diagram below shows how the data will be stored in memory.
The first 4 bytes will be the total compressed bytes to follow for a channel

+---------------++-------------------------+     +--------------++----------------------+
|               ||                         |     |              ||                      |
| ch1 size      || ch1 compressed data     |     | ch2 size     || ch2 compressed data  |
+---------------++-------------------------+     +--------------++----------------------+
 */
    uint32_t total_pixels = width * height;
    // Align the offset for the channels
    uint32_t offset = (((total_pixels / 4) / 64) + 1) * 64;
    uint8_t * comp_data = new uint8_t[total_pixels * 4];
    uint32_t comp_size[4];
    Rice_Compress_accel(imgdata,
                        comp_data,
                        width,
                        height,
                        offset,
                        7);

    // Put into vectors to return
    std::vector<std::vector<uint8_t> > output;
    for (int i = 0; i < 4; i++)
    {
        uint32_t * data_size = (uint32_t*)(comp_data + (i * offset));
        uint8_t * src = (uint8_t*)(comp_data + (i * offset) + 4);
        uint8_t * end = (uint8_t*)(comp_data + (i * offset) + 4 + *data_size);
        std::vector<uint8_t> comp_channel(src, end);
        output.push_back(comp_channel);
    }

    delete [] comp_data;

    // Make out as large as the input data
    return output;
}

void AccelRice::decompress( std::vector<uint8_t> &in,
                            std::vector<int16_t> &out,
                            uint32_t uncompressedSize)
{
    // Do nothing. Use the CPU implementation
}
