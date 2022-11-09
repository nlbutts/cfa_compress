#include <stdlib.h>
#include "AccelRice.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"


#define RICE_HISTORY    16
#define RICE_WORD       16
#define BITS            16
#define RICE_THRESHOLD  8

typedef ap_axiu<BITS, 1, 1, 1> pixel;
typedef ap_axiu<8, 1, 1, 1> compdata;

typedef struct {
    hls::stream<compdata>*  outdata;
    unsigned int            index;
    ap_uint<40>             bits;
    ap_uint<6> 				bitcount;
} rice_bitstream_t;

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
    hls::stream<compdata> &outdata )
{
    stream->outdata     = &outdata;
    stream->bits        = 0;
    stream->index       = 0;
    stream->bitcount    = 0;
}


/*************************************************************************
* _Rice_WriteBit() - Write a bit to the output stream.
*************************************************************************/

static void _Rice_WriteBit( rice_bitstream_t *stream, bool x )
{
    stream->bitcount++;
    stream->bits[0] = x;
    stream->bits <<= 1;
}


/*************************************************************************
* _Rice_EncodeWord() - Encode and write a word to the output stream.
*************************************************************************/

static void _Rice_EncodeWord( ap_uint<16> x,
                              uint16_t k,
                              rice_bitstream_t *stream )
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
        stream->bits <<= RICE_THRESHOLD;
        stream->bits.range(RICE_THRESHOLD - 1, 0) = temp.range(RICE_THRESHOLD - 1, 0);
        stream->bitcount += RICE_THRESHOLD;

        /* Encode the overflow with alternate coding */
        q -= RICE_THRESHOLD;

        /* Write number of bits needed to represent the overflow */
        o = _Rice_NumBits( q );

        temp[0] = 0;
        stream->bits <<= o + 1;
        stream->bits.range(o, 0) = temp.range(o, 0);
        stream->bitcount += o + 1;

        if (o >= 2)
        {
            stream->bits <<= (o - 2) + 1;
            stream->bits.range(o - 2, 0) = q.range(o - 2, 0);
            stream->bitcount += (o - 2) + 1;
        }

        stream->bits <<= k;
        stream->bits.range(k - 1, 0) = x.range(k - 1, 0);
        stream->bitcount += k;
    }
    else
    {
        temp[0] = 0;
        stream->bits <<= q + 1 + k;
        stream->bits.range(q + k, k) = temp.range(q, 0);
        stream->bits.range(k - 1, 0) = x.range(k - 1, 0);
        stream->bitcount += (q + 1 + k);
    }

    while (stream->bitcount > 8)
    {
        compdata d;
        d.data = stream->bits.range(stream->bitcount - 1, stream->bitcount - 8);
        d.user = 0;
        d.last = 0;
        stream->outdata->write(d);
        stream->bitcount -= 8;
        stream->index++;
    }
}


int Rice_Compress(hls::stream<pixel> &indata,
                  hls::stream<compdata> &outdata,
                  unsigned int insize,
                  uint16_t k )
{
    rice_bitstream_t    stream;
    unsigned int        i, incount;
    ap_uint<4>          hist[ RICE_HISTORY ] = {0};
    ap_uint<5>          j;
    int16_t             sx;
    uint16_t            x;
    uint16_t            sumk = 0;

    _Rice_InitBitstream(&stream, outdata);
    compdata d;
    d.data = k;
    d.user = 1;
    d.last = 0;
    outdata.write(d);

    for (i = 0; i < RICE_HISTORY; i++)
    {
        hist[i] = 0;
    }

    // how many 8-bit values from 16-bit inputs
    incount = insize >> 1;

    /* Encode input stream */
    for( i = 0; i < incount; ++ i )
    {
        /* Revise optimum k? */
        if( i >= RICE_HISTORY )
        {
            k = (sumk + (RICE_HISTORY >> 1)) >> 4;
        }

        /* Read word from input buffer */
        sx = indata.read().data;
        x = sx < 0 ? -1-(sx<<1) : sx<<1;

        /* Encode word to output buffer */
        _Rice_EncodeWord( x, k, &stream );

        /* Update history */
        sumk -= hist[i % RICE_HISTORY];
        hist[ i % RICE_HISTORY ] = _Rice_NumBits( x );
        sumk += hist[ i % RICE_HISTORY ];
        //printf("k: %d sumk: %d\n", k, sumk);
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
    stream.bits <<= (8 - stream.bitcount);
    d.data = stream.bits.range(7, 0);
    d.user = 0;
    d.last = 1;
    outdata.write(d);

    return stream.index + 1;
}

int Rice_Compress_accel( hls::stream<pixel> &indata,
                         hls::stream<compdata> &outdata,
                         unsigned int insize,
                         int k )
{
#pragma HLS INTERFACE mode=s_axilite port=return
#pragma HLS INTERFACE mode=s_axilite port=insize
#pragma HLS INTERFACE mode=s_axilite port=k
//#pragma HLS DATAFLOW
#pragma HLS INTERFACE axis port=indata
#pragma HLS INTERFACE axis port=outdata
    return Rice_Compress(indata, outdata, insize, k);
}

int AccelRice::compress( std::vector<int16_t> &in,
                         std::vector<uint8_t> &out)
{
    hls::stream<pixel> instream;
    hls::stream<compdata> outstream;
    pixel p;
    p.user = 1;
    p.last = 0;
    for (auto it = in.begin(); it != in.end(); ++it)
    {
        p.data = *it;
        p.user = 0;
        if ((it + 1) == in.end())
        {
            p.last = 1;
        }
        instream.write(p);
    }

    Rice_Compress_accel(instream, outstream, in.size() * 2, 7);

    while (!outstream.empty())
    {
        compdata d = outstream.read();
        out.push_back(d.data);
    }
    return out.size();
}

void AccelRice::decompress( std::vector<uint8_t> &in,
                            std::vector<int16_t> &out,
                            uint32_t uncompressedSize)
{
    // Do nothing. Use the CPU implementation
}
