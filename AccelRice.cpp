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
    hls::stream<uint8_t>*   outdata;
    unsigned int            index;
    ap_uint<9>              tempbits;
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
    hls::stream<uint8_t> &outdata )
{
    stream->outdata     = &outdata;
    stream->tempbits    = 1;
    stream->index       = 0;
}


/*************************************************************************
* _Rice_WriteBit() - Write a bit to the output stream.
*************************************************************************/

static void _Rice_WriteBit( rice_bitstream_t *stream, bool x )
{
    stream->tempbits <<= 1;
    stream->tempbits[0] = x;
    if (stream->tempbits & 0x100)
    {
        stream->outdata->write(stream->tempbits & 0xFF);
        stream->tempbits = 1;
        stream->index++;
    }
}


/*************************************************************************
* _Rice_EncodeWord() - Encode and write a word to the output stream.
*************************************************************************/

static void _Rice_EncodeWord( uint16_t x,
                              uint16_t k,
                              rice_bitstream_t *stream )
{
    ap_uint<4>  i;
    ap_uint<10> q;
    ap_uint<6>  o;
    ap_int<9>   j;

    /* Determine overflow */
    q = x >> k;

    /* Too large rice code? */
    if( q > RICE_THRESHOLD )
    {
        /* Write Rice code (except for the final zero) */
        for( j = 0; j < RICE_THRESHOLD; ++ j )
        {
            _Rice_WriteBit( stream, 1 );
        }

        /* Encode the overflow with alternate coding */
        q -= RICE_THRESHOLD;

        /* Write number of bits needed to represent the overflow */
        o = _Rice_NumBits( q );
        for( j = 0; j < o; ++ j )
        {
            _Rice_WriteBit( stream, 1 );
        }
        _Rice_WriteBit( stream, 0 );

        /* Write the o-1 least significant bits of q "as is" */
        for( j = o-2; j >= 0; -- j )
        {
            _Rice_WriteBit( stream, (q >> j) & 1 );
        }
    }
    else
    {
        /* Write Rice code */
        for( i = 0; i < q; ++ i )
        {
            _Rice_WriteBit( stream, 1 );
        }
        _Rice_WriteBit( stream, 0 );
    }

    /* Encode the rest of the k bits */
    for( j = k-1; j >= 0; -- j )
    {
        _Rice_WriteBit( stream, (x >> j) & 1 );
    }
}


int Rice_Compress(hls::stream<int16_t> &indata,
                  hls::stream<uint8_t> &outdata,
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
    outdata.write(k);

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
            // k = 0;
            // for( j = 0; j < RICE_HISTORY; ++ j )
            // {
            //     k += hist[ j ];
            // }
            // RICE_HISTORY is 16 which is a shift by 4
            //k = (k + (RICE_HISTORY>>1)) >> 4;
            k = (sumk + (RICE_HISTORY >> 1)) >> 4;
        }

        /* Read word from input buffer */
        sx = indata.read();
        x = sx < 0 ? -1-(sx<<1) : sx<<1;

        /* Encode word to output buffer */
        _Rice_EncodeWord( x, k, &stream );

        /* Update history */
        sumk -= hist[i % RICE_HISTORY];
        hist[ i % RICE_HISTORY ] = _Rice_NumBits( x );
        sumk += hist[ i % RICE_HISTORY ];
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
    while (!(stream.tempbits & 0x100))
    {
        stream.tempbits <<= 1;
    }
    outdata.write(stream.tempbits & 0xFF);

    return stream.index + 1;
}

int Rice_Compress_accel( hls::stream<int16_t> &indata,
                         hls::stream<uint8_t> &outdata,
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
    hls::stream<int16_t> instream;
    hls::stream<uint8_t> outstream;
    for (auto it = in.begin(); it != in.end(); ++it)
    {
        instream.write(*it);
    }

    Rice_Compress_accel(instream, outstream, in.size() * 2, 7);

    while (!outstream.empty())
    {
        out.push_back(outstream.read());
    }
    return out.size();
}

void AccelRice::decompress( std::vector<uint8_t> &in,
                            std::vector<int16_t> &out,
                            uint32_t uncompressedSize)
{
    // Do nothing. Use the CPU implementation
}
