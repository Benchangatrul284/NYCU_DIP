#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <cstdint>
#include <cmath>
#include <iomanip>

#pragma pack(push, 1)
struct BMPHeader {
    uint16_t fileType;
    uint32_t fileSize;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offsetData;
};

struct BMPInfoHeader {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t compression;
    uint32_t imageSize;
    int32_t xPixelsPerMeter;
    int32_t yPixelsPerMeter;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
};
#pragma pack(pop)

int clamp(int value, int min, int max) {
    return std::max(min, std::min(value, max));
}

void readBMP(const std::string& filename, BMPHeader& header, BMPInfoHeader& infoHeader,
             std::vector<std::vector<uint8_t>>& red,
             std::vector<std::vector<uint8_t>>& green,
             std::vector<std::vector<uint8_t>>& blue) {
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile) {
        std::cerr << "Error: Could not open input file." << std::endl;
        exit(1);
    }

    inFile.read(reinterpret_cast<char*>(&header), sizeof(header));
    inFile.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (header.fileType != 0x4D42 || infoHeader.bitCount != 24) {
        std::cerr << "Error: Only 24-bit BMP format is supported." << std::endl;
        exit(1);
    }

    int width = infoHeader.width;
    int height = infoHeader.height;
    int padding = (4 - (width * 3) % 4) % 4;

    red.resize(height, std::vector<uint8_t>(width));
    green.resize(height, std::vector<uint8_t>(width));
    blue.resize(height, std::vector<uint8_t>(width));

    inFile.seekg(header.offsetData, std::ios::beg);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t pixel[3];
            inFile.read(reinterpret_cast<char*>(pixel), 3);
            blue[y][x] = pixel[0];
            green[y][x] = pixel[1];
            red[y][x] = pixel[2];
        }
        inFile.ignore(padding);
    }
    inFile.close();
}

void writeBMP(const std::string& filename, const BMPHeader& header, const BMPInfoHeader& infoHeader,
              const std::vector<std::vector<uint8_t>>& red,
              const std::vector<std::vector<uint8_t>>& green,
              const std::vector<std::vector<uint8_t>>& blue) {
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        std::cerr << "Error: Could not open output file." << std::endl;
        exit(1);
    }

    outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
    outFile.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    int width = infoHeader.width;
    int height = infoHeader.height;
    int padding = (4 - (width * 3) % 4) % 4;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t pixel[3] = {blue[y][x], green[y][x], red[y][x]};
            outFile.write(reinterpret_cast<char*>(pixel), 3);
        }
        outFile.write("\0\0\0", padding);
    }
    outFile.close();
}

int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0] << " <input.bmp> <output.bmp> <sigmaSpatial> <sigmaRange> <kernelSize>" << std::endl;
        return 1;
    }

    std::string inputFileName = argv[1];
    std::string outputFileName = argv[2];
    float sigmaSpatial = std::stof(argv[3]);
    float sigmaRange = std::stof(argv[4]);
    int kernelSize = std::stoi(argv[5]);

    if (sigmaSpatial <= 0 || sigmaRange <= 0 || kernelSize % 2 == 0) {
        std::cerr << "Error: Sigma values must be positive and kernel size must be odd." << std::endl;
        return 1;
    }

    BMPHeader header;
    BMPInfoHeader infoHeader;
    std::vector<std::vector<uint8_t>> red, green, blue;

    readBMP(inputFileName, header, infoHeader, red, green, blue);

    int width = infoHeader.width;
    int height = infoHeader.height;

    // Apply bilateral filter on RGB channels
    std::vector<std::vector<uint8_t>> redFiltered(height, std::vector<uint8_t>(width));
    std::vector<std::vector<uint8_t>> greenFiltered(height, std::vector<uint8_t>(width));
    std::vector<std::vector<uint8_t>> blueFiltered(height, std::vector<uint8_t>(width));

    applyBilateralFilter(red, redFiltered, width, height, kernelSize, sigmaSpatial, sigmaRange);
    applyBilateralFilter(green, greenFiltered, width, height, kernelSize, sigmaSpatial, sigmaRange);
    applyBilateralFilter(blue, blueFiltered, width, height, kernelSize, sigmaSpatial, sigmaRange);

    writeBMP(outputFileName, header, infoHeader, redFiltered, greenFiltered, blueFiltered);

    std::cout << "Bilateral filter applied to RGB channels. Output saved as '" << outputFileName << "'." << std::endl;
    return 0;
}