#include "cfa_comp.h"
#include <boost/crc.hpp>
#include <stdint.h>

CfaComp::CfaComp(IRice & compressor, bool debug)
: _rice(&compressor)
, _debug(debug)
{

}

CfaComp::~CfaComp()
{

}

void CfaComp::append_comp_channel(std::vector<uint8_t> &pkg, std::vector<uint8_t> &comp_data)
{
    pkg.insert(pkg.end(), comp_data.begin(), comp_data.end());
}

int CfaComp::compress(cv::Mat &img, std::vector<uint8_t> &compimg)
{
    uint32_t total_pixels = img.rows * img.cols;
    uint8_t * comp_data = new uint8_t[total_pixels * 4];
    // This will return a vector of vectors
    uint32_t total_size = _rice->compress((uint16_t*)img.data, comp_data, img.cols, img.rows);
    CfaCompData header;
    header.type[0] = 'C';
    header.type[1] = 'F';
    header.type[2] = 'A';
    header.type[3] = 'C';
    header.type[4] = '0';
    header.type[5] = '0';
    header.type[6] = '1';
    header.channels = 4;
    boost::crc_32_type crc;
    crc.process_bytes(img.data, total_pixels * 2);
    header.crc = crc();
    header.width = img.cols;
    header.height = img.rows;
    uint32_t * ch_size = (uint32_t*)comp_data;
    header.channel_size[0] = *(ch_size    );
    header.channel_size[1] = *(ch_size + 1);
    header.channel_size[2] = *(ch_size + 2);
    header.channel_size[3] = *(ch_size + 3);
    auto src = (uint8_t*)&header;
    compimg.clear();
    compimg.insert(compimg.begin(), src, src + sizeof(CfaCompData));
    compimg.insert(compimg.end(), comp_data + 16, comp_data + 16 + total_size);

    float cr = (float)compimg.size() / (total_pixels * 2);
    cr *= 100;

    printf("Compressed size: %d  Uncompressed size: %d  Ratio: %0.1f%%\n",
           (int)compimg.size(), total_pixels * 2, cr);

    delete [] comp_data;

    return compimg.size();
}

bool CfaComp::verifyHeader(CfaCompData * header)
{
    bool result = false;
    if ((header->type[0] == 'C') &&
        (header->type[1] == 'F') &&
        (header->type[2] == 'A') &&
        (header->type[3] == 'C') &&
        (header->type[4] == '0') &&
        (header->type[5] == '0') &&
        (header->type[6] == '1') &&
        (header->channels == 4))
    {
        result = true;
        printf("Header is good\n");
    }
    else
    {
        printf("Header is bad\n");
    }

    return result;
}

void CfaComp::diff_to_int16(std::vector<int16_t> &data)
{
    for (int i = 1; i < data.size(); i++)
    {
        data[i] = data[i - 1] + data[i];
    }
}

int CfaComp::decompress(std::vector<uint8_t> &compimg, cv::Mat &outimg)
{
    CfaCompData * header = (CfaCompData *)compimg.data();

    int result = 0;

    if (verifyHeader(header))
    {
        auto src = compimg.data() + sizeof(CfaCompData);
        std::vector<int16_t> decomp_data[4];
        int uncompressed_size = header->width * header->height * 2 / 4;
        for (int i = 0; i < header->channels; i++)
        {
            std::vector<uint8_t> temp(src, src + header->channel_size[i]);
            _rice->decompress(temp, decomp_data[i], uncompressed_size);
            diff_to_int16(decomp_data[i]);
            src += header->channel_size[i];
            if (_debug)
            {
                std::stringstream ss;
                ss << "test";
                ss << i;
                ss << ".bin";
                save_vector<int16_t>(ss.str(), decomp_data[i]);
            }
        }

        printf("Assembling image\n");
        outimg.create(header->height, header->width, CV_16UC1);

        uint16_t * ptr = (uint16_t*)outimg.data;
        int offset = header->width / 2;
        for (int y = 0; y < header->height; y++)
        {
            for (int x = 0; x < header->width; x++)
            {
                int index = ((y & 1) << 1) + (x & 1);
                *ptr = decomp_data[index][((y >> 1) * offset) + (x >> 1)];
                ptr++;
            }
        }
    }
    else
    {
        result = -1;
    }
    return result;
}

int CfaComp::compare_images(cv::Mat img1, cv::Mat img2)
{
    cv::Mat temp1;
    cv::Mat temp2;
    img1.convertTo(temp1, CV_32F);
    img2.convertTo(temp2, CV_32F);
    int result = -1;
    cv::Mat diff;
    cv::subtract(temp1, temp2, diff);
    auto sum = cv::sum(diff);
    printf("Sum of diff: %f\n", sum.val[0]);
    if ((-0.1 < sum.val[0]) && (sum.val[0] < 0.1))
    {
        printf("IMAGES MATCH\n");
        result = 0;
    }
    else
    {
        printf("FAILURE: Images are different\n");
    }
    return result;
}

int CfaComp::load_file(std::string filename, std::vector<uint8_t> &data)
{
    int ret = 0;
    FILE * f = fopen(filename.c_str(), "rb");
    if (f != nullptr)
    {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        data.resize(size);
        fseek(f, 0, SEEK_CUR);
        auto read_size = fread(data.data(), 1, size, f);
        if (size != read_size)
        {
            printf("Error reading file\n");
        }
        fclose(f);
    }
    else
    {
        ret = -1;
    }
    return ret;
}
