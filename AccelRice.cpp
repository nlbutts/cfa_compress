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
    ap_uint<5>          hist_index;
    ap_uint<64>         bits;
    ap_uint<8>          bitcount;
} rice_bitstream_t;

static void _Rice_init(rice_bitstream_t * config)
{
    init: for (i = 0; i < RICE_HISTORY; i++)
    {
        config->hist[i] = 0;
    }

    config->hist_index = 0;
    config->bits.range(7, 0) = 7;
    config->bitcount += 8;
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
* _Rice_InitBitstream() - Initialize a bitstream.
*************************************************************************/

static void _Rice_InitBitstream( rice_bitstream_t *stream,
    uint8_t* outdata )
{
    stream->outdata     = outdata;
    stream->bits        = 0;
    stream->total_bytes       = 0;
    stream->bitcount    = 0;
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

#define MAX_BUF 64
int Rice_Compress(const int16_t* indata,
                  uint8_t* outdata,
                  unsigned int insize,
                  uint16_t k )
{
    unsigned int        i, incount, j = 0, y = 0;
    ap_uint<4>          hist[ RICE_HISTORY ] = {0};
    int16_t             sx;
    uint16_t            x;
    uint16_t            sumk = 0;
    unsigned int        total_bytes = 0;
    ap_uint<64>         bits = 0;
    ap_uint<8>          bitcount = 0;
    ap_uint<5>          hist_index;
    uint8_t             tmp_out[MAX_BUF + 16];
    ap_uint<11>         tmp_index = 0;

    //outdata[0] = k;
    tmp_out[tmp_index++] = k;
    total_bytes++;

    init: for (i = 0; i < RICE_HISTORY; i++)
    {
        hist[i] = 0;
    }

    // how many 8-bit values from 16-bit inputs
    incount = insize >> 1;

    /* Encode input stream */
    main: for( i = 0; i < incount; ++ i )
    {
        /* Revise optimum k? */
        if( i >= RICE_HISTORY )
        {
            k = (sumk + (RICE_HISTORY >> 1)) >> 4;
        }

        /* Read word from input buffer */
        sx = *(indata + i);
        x = sx < 0 ? -1-(sx<<1) : sx<<1;

        /* Encode word to output buffer */
        _Rice_EncodeWord( x, k, &bits, &bitcount );

        /* Update history */

        hist_index = i & (RICE_HISTORY - 1);
        sumk -= hist[hist_index];
        hist[ hist_index ] = _Rice_NumBits( x );
        sumk += hist[ hist_index ];
        //printf("k: %d sumk: %d\n", k, sumk);
        output: while (bitcount > 8)
        {
            #pragma HLS loop_tripcount min=0 max=3 avg=1
            //outdata[total_bytes] = bits.range(bitcount - 1, bitcount - 8);
            tmp_out[tmp_index++] = bits.range(bitcount - 1, bitcount - 8);
            bitcount -= 8;
            total_bytes++;
        }

        memout: if (tmp_index >= MAX_BUF)
        {
            memcpy(outdata + j, tmp_out, MAX_BUF);
            j += MAX_BUF;
            memcpy(tmp_out, tmp_out + MAX_BUF, tmp_index - MAX_BUF);
            tmp_index -= MAX_BUF;
        }
    }

    // /* Was there a buffer overflow? */
    // if( i < incount )
    // {
    //     //printf("OVERFLOW\n");
    // }
    // else
    // {
    // }

    // Flush the last few bits
    bits <<= (8 - bitcount);
    //outdata[total_bytes] = bits.range(7, 0);
    tmp_out[tmp_index++] = bits.range(7, 0);
    memcpy(outdata + j, tmp_out, tmp_index);

    return total_bytes + 1;
}

int Rice_Compress_accel( const ap_uint<128>* indata,
                         ap_uint<128>* outdata,
                         unsigned int insize,
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

    //return Rice_Compress(indata, outdata, insize, k);
	for (int i = 0; i < insize/2; i++)
	{
		outdata[i] = indata[i] + 1;
	}
	return insize;
}

std::vector<std::vector<uint8_t> > AccelRice::compress( cv::Mat &img)
{
    // ap_uint<128> * dst = (ap_uint<128>*)new uint8_t[in.size() * (128/4)];
    // ap_uint<128> * src = (ap_uint<128>*)new uint8_t[in.size() * (128/4)];
    // for (int i = 0; i < in.size(); i++)
    // {
    // 	src[i] = in[i];
    // }
    // int size = Rice_Compress_accel(src, dst, in.size() * 2, 7);
    // out.resize(size);
    // for (int i = 0; i < size; i++)
    // {
    //     out[i] = dst[i];
    // }
    // delete [] dst;
    std::vector<std::vector<uint8_t> > compdata;
    return compdata;
}

void AccelRice::decompress( std::vector<uint8_t> &in,
                            std::vector<int16_t> &out,
                            uint32_t uncompressedSize)
{
    // Do nothing. Use the CPU implementation
}
