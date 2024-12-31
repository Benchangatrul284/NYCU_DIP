#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <algorithm>

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct BMPInfoHeader {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

struct RGB {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
};

inline uint8_t clamp(int value) {
    return static_cast<uint8_t>((value < 0) ? 0 : (value > 255) ? 255 : value);
}

void adjustColorTemperature(std::vector<std::vector<RGB>>& image, const std::string& mode) {
    double redFactor = 1.0, blueFactor = 1.0;
    double greenFactor = 1.0;
    if (mode == "warm") {
        redFactor = 1.2;
        greenFactor = 1.1;
        blueFactor = 0.8;
    } else if (mode == "cool") {
        redFactor = 0.8;
        greenFactor = 0.9;
        blueFactor = 1.2;
    } else {
        throw std::runtime_error("Invalid mode. Use 'warm' or 'cool'.");
    }

    for (auto& row : image) {
        for (auto& pixel : row) {
            pixel.red = clamp(static_cast<int>(pixel.red * redFactor));
            pixel.blue = clamp(static_cast<int>(pixel.blue * blueFactor));
        }
    }
}

std::vector<std::vector<RGB>> readBMP(const std::string& filename, BMPFileHeader& fileHeader, BMPInfoHeader& infoHeader) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Error opening input file.");
    }

    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (fileHeader.bfType != 0x4D42 || infoHeader.biBitCount != 24) {
        throw std::runtime_error("Unsupported BMP format. Only 24-bit BMP files are supported.");
    }

    int width = infoHeader.biWidth;
    int height = std::abs(infoHeader.biHeight);
    int padding = (4 - (width * 3) % 4) % 4;

    std::vector<std::vector<RGB>> image(height, std::vector<RGB>(width));
    for (int i = 0; i < height; i++) {
        file.read(reinterpret_cast<char*>(image[i].data()), width * sizeof(RGB));
        file.ignore(padding);
    }

    return image;
}

void writeBMP(const std::string& filename, const BMPFileHeader& fileHeader, const BMPInfoHeader& infoHeader, const std::vector<std::vector<RGB>>& image) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Error opening output file.");
    }

    file.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    int width = infoHeader.biWidth;
    int height = std::abs(infoHeader.biHeight);
    int padding = (4 - (width * 3) % 4) % 4;

    for (const auto& row : image) {
        file.write(reinterpret_cast<const char*>(row.data()), width * sizeof(RGB));
        file.write("\0\0\0", padding);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <mode> <input.bmp> <output.bmp>\n";
        return 1;
    }

    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;
    std::string mode = argv[1];

    try {
        auto image = readBMP(argv[2], fileHeader, infoHeader);
        adjustColorTemperature(image, mode);
        writeBMP(argv[3], fileHeader, infoHeader, image);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
