#include "bmp_image.h"
#include "pack_defines.h"
#include <fstream>
#include <vector>
#include <cstdint>
#include <algorithm>

namespace img_lib {

static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

PACKED_STRUCT_BEGIN BMPFileHeader {
    uint16_t bfSignature;
    uint32_t bfTotalSize;
    uint32_t bfReserved;
    uint32_t bfOffBits;
} PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BMPInfoHeader {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} PACKED_STRUCT_END

bool SaveBMP(const Path& file, const Image& image) {
    std::ofstream out(file, std::ios::binary);
    if (!out) return false;

    int width = image.GetWidth();
    int height = image.GetHeight();
    int stride = GetBMPStride(width);

    BMPFileHeader fh{};
    BMPInfoHeader ih{};

    fh.bfSignature = 0x4D42; // "BM"
    fh.bfOffBits = 14 + 40;
    fh.bfTotalSize = fh.bfOffBits + stride * height;
    fh.bfReserved = 0;

    ih.biSize = 40;
    ih.biWidth = width;
    ih.biHeight = height;
    ih.biPlanes = 1;
    ih.biBitCount = 24;
    ih.biCompression = 0;
    ih.biSizeImage = stride * height;
    ih.biXPelsPerMeter = 11811;
    ih.biYPelsPerMeter = 11811;
    ih.biClrUsed = 0;
    ih.biClrImportant = 0x01000000;

    out.write(reinterpret_cast<const char*>(&fh), sizeof(fh));
    out.write(reinterpret_cast<const char*>(&ih), sizeof(ih));
    if (!out) return false;

    std::vector<uint8_t> row(stride);

    for (int y = height - 1; y >= 0; --y) { 
        const Color* line = image.GetLine(y);
        std::fill(row.begin(), row.end(), 0); 
        for (int x = 0; x < width; ++x) {
            row[x * 3 + 0] = static_cast<uint8_t>(line[x].b);
            row[x * 3 + 1] = static_cast<uint8_t>(line[x].g);
            row[x * 3 + 2] = static_cast<uint8_t>(line[x].r);
        }
        out.write(reinterpret_cast<const char*>(row.data()), stride);
        if (!out) return false;
    }

    return out.good();
}

Image LoadBMP(const Path& file) {
    std::ifstream ifs(file, std::ios::binary);
    if (!ifs) return {};

    BMPFileHeader fh{};
    BMPInfoHeader ih{};

    if (!ifs.read(reinterpret_cast<char*>(&fh), sizeof(fh))) return {};
    if (!ifs.read(reinterpret_cast<char*>(&ih), sizeof(ih))) return {};

    if (fh.bfSignature != 0x4D42) return {}; 
    if (ih.biPlanes != 1) return {};
    if (ih.biBitCount != 24) return {};
    if (ih.biCompression != 0) return {};
    if (ih.biWidth <= 0) return {};
    if (ih.biHeight == 0) return {};

    int width = ih.biWidth;
    int height = ih.biHeight > 0 ? ih.biHeight : -ih.biHeight;
    bool bottom_up = ih.biHeight > 0;

    int stride = GetBMPStride(width);

    ifs.seekg(fh.bfOffBits, std::ios::beg);
    if (!ifs) return {};

    Image img(width, height, Color::Black());
    std::vector<uint8_t> row(stride);

    for (int i = 0; i < height; ++i) {
        int y = bottom_up ? (height - 1 - i) : i;
        if (!ifs.read(reinterpret_cast<char*>(row.data()), stride))
            return {};
        for (int x = 0; x < width; ++x) {
            uint8_t b = row[x * 3 + 0];
            uint8_t g = row[x * 3 + 1];
            uint8_t r = row[x * 3 + 2];
            img.GetLine(y)[x].r = static_cast<std::byte>(r);
            img.GetLine(y)[x].g = static_cast<std::byte>(g);
            img.GetLine(y)[x].b = static_cast<std::byte>(b);
        }
    }

    return img;
}

} // namespace img_lib