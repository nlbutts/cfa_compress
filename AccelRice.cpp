#include <stdlib.h>
#include "AccelRice.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include <memory.h>
#include <stdio.h>

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

void load_data(const uint16_t* indata, hls::stream<uint16_t> instream[4], uint32_t width, uint32_t height)
{
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            uint8_t index = ((y & 1) << 1) + (x & 1);
            instream[index].write(indata[(y * width) + x]);
        }
    }
}

void process_data(hls::stream<uint16_t> instream[4],
                  hls::stream<uint8_t> outstream[4],
                  uint16_t k,
                  hls::stream<bool> &done_loading,
                  hls::stream<bool> &done_processing,
                  hls::stream<bool> &start)
{
    rice_bitstream_t config[4];

    if (!start.empty())
    {
        bool value = start.read();
        if (value)
        {
            _Rice_init(config[0]);
            _Rice_init(config[1]);
            _Rice_init(config[2]);
            _Rice_init(config[3]);
        }
    }

    while (done_loading.empty())
    {
        for (uint8_t ch = 0; ch < 4; ch++)
        {
            int outbytes = Rice_Compress(instream[ch], outstream[ch], config[ch], k);
        }
    }

    // Done could be set pretty early. So now empty the instream
    for (uint8_t ch = 0; ch < 4; ch++)
    {
        while (!instream[ch].empty())
        {
            int outbytes = Rice_Compress(instream[ch], outstream[ch], config[ch], k);
        }
    }

    for (uint8_t ch = 0; ch < 4; ch++)
    {
        _Rice_flush(config[ch], outstream[ch]);
    }
    (void)done_loading.read();
    done_processing.write(1);
}

void process_channel(hls::stream<uint16_t> &instream,
                     hls::stream<uint8_t> &outstream,
                     uint16_t k,
                     uint32_t pixels_per_channel,
                     hls::stream<bool> &done)
{
    rice_bitstream_t config;
    _Rice_init(config);

    for (uint32_t index = 0; index < pixels_per_channel; index++)
    {
        Rice_Compress(instream, outstream, config, k);
    }

    _Rice_flush(config, outstream);
    done.write(1);
}

void save_data(hls::stream<uint8_t> outstream[4],
               uint8_t * outdata,
               uint32_t out_offset[4],
               hls::stream<bool> done[4])
{
    uint32_t size[4];
    uint32_t offset[4];

    for (uint8_t ch = 0; ch < 4; ch++)
    {
        size[ch] = 0;
        offset[ch] = out_offset[ch] + 4;
    }

    uint32_t run = 0;
    while (run != 0xF)
    {
        for (uint8_t ch = 0; ch < 4; ch++)
        {
            save: while (!outstream[ch].empty())
            {
                outdata[offset[ch]++] = outstream[ch].read();
                size[ch]++;
                if (!done[ch].empty())
                {
                    run |= (1 << ch);
                    done[ch].read();
                }
            }
        }
    }

    for (uint8_t ch = 0; ch < 4; ch++)
    {
        savefinal: while (!outstream[ch].empty())
        {
            outdata[offset[ch]++] = outstream[ch].read();
            size[ch]++;
        }

        outdata[out_offset[ch]    ] = (size[ch]      ) & 0xFF;
        outdata[out_offset[ch] + 1] = (size[ch] >>  8) & 0xFF;
        outdata[out_offset[ch] + 2] = (size[ch] >> 16) & 0xFF;
        outdata[out_offset[ch] + 3] = (size[ch] >> 24) & 0xFF;
    }
}

uint32_t init(uint32_t out_offset, uint32_t offsets[4], uint32_t width, uint32_t height)
{
    offsets[0] = 0;
    offsets[1] = out_offset;
    offsets[2] = out_offset << 1;
    // This is to avoid a multiply
    offsets[3] = offsets[1] + offsets[2];
    return (width * height) >> 2;
}

void Rice_Compress_accel(const uint16_t* indata,
                         uint8_t* outdata,
                         uint32_t width,
                         uint32_t height,
                         uint32_t out_offset,
                         int k)
{
#pragma HLS INTERFACE mode=s_axilite port=width
#pragma HLS INTERFACE mode=s_axilite port=height
#pragma HLS INTERFACE mode=s_axilite port=out_offset
#pragma HLS INTERFACE mode=s_axilite port=k
#pragma HLS DATAFLOW
#pragma HLS INTERFACE mode=m_axi port=indata bundle=gem0 depth=8192
#pragma HLS INTERFACE mode=m_axi port=outdata bundle=gem1 depth=16384

    hls::stream<uint16_t> instream[4];
#pragma HLS stream variable=instream type=fifo depth=8
    hls::stream<uint8_t>  outstream[4];
#pragma HLS stream variable=outstream type=fifo depth=8
    // The first four bytes are the total size of the compressed data that follows
    uint32_t offsets[4];
    uint32_t pixels_per_channel;
    hls::stream<bool> done[4];

    pixels_per_channel = init(out_offset, offsets, width, height);
    load_data(indata, instream, width, height);
    process_channel(instream[0], outstream[0], k, pixels_per_channel, done[0]);
    process_channel(instream[1], outstream[1], k, pixels_per_channel, done[1]);
    process_channel(instream[2], outstream[2], k, pixels_per_channel, done[2]);
    process_channel(instream[3], outstream[3], k, pixels_per_channel, done[3]);
    //process_data(instream, outstream, k, done_loading, done_processing, start);
    save_data(outstream, outdata, offsets, done);
}

uint32_t AccelRice::compress(const uint16_t * imgdata,
                             uint8_t * outdata,
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
    uint32_t offset = total_pixels / 2;
    uint8_t * comp_data = new uint8_t[total_pixels * 4];
    uint32_t comp_size[4];
    printf("Total pixels: %d  offset: %d\n", total_pixels, offset);
    Rice_Compress_accel(imgdata,
                        comp_data,
                        width,
                        height,
                        offset,
                        7);
    printf("Rice_Compress_accel returned\n");
    uint32_t total_comp_size = 0;
    for (int ch = 0; ch < 4; ch++)
    {
        uint8_t * ptr = comp_data + (ch * offset);
        uint32_t * size = (uint32_t*)ptr;
        printf("Ch: %d size: %d\n", ch, *size);
        uint8_t * src = &ptr[4];
        uint8_t * end = &ptr[4] + *size;
        // Copy the size
        memcpy(outdata + (ch * 4), size, 4);
        memcpy(outdata + total_comp_size + 16, ptr + 4, *size);
        total_comp_size += *size;
    }

    delete [] comp_data;

    // Make out as large as the input data
    return total_comp_size;
}

void AccelRice::decompress( std::vector<uint8_t> &in,
                            std::vector<int16_t> &out,
                            uint32_t uncompressedSize)
{
    // Do nothing. Use the CPU implementation
}
