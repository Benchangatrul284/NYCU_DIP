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

void generateGaussianKernel(std::vector<std::vector<float>>& kernel, int kernelSize, float sigma) {
    int halfSize = kernelSize / 2;
    float sum = 0.0f;

    for (int y = -halfSize; y <= halfSize; ++y) {
        std::vector<float> row;
        for (int x = -halfSize; x <= halfSize; ++x) {
            float value = std::exp(-(x * x + y * y) / (2 * sigma * sigma));
            row.push_back(value);
            sum += value;
        }
        kernel.push_back(row);
    }

    // Normalize the kernel
    for (auto& row : kernel) {
        for (auto& value : row) {
            value /= sum;
        }
    }
}

void printKernel(const std::vector<std::vector<float>>& kernel) {
    std::cout << "Gaussian Kernel:" << std::endl;
    for (const auto& row : kernel) {
        for (const auto& value : row) {
            std::cout << std::fixed << std::setprecision(4) << value << " ";
        }
        std::cout << std::endl;
    }
}

void applyGaussianFilter(const std::vector<std::vector<uint8_t>>& channel,
                         std::vector<std::vector<uint8_t>>& output,
                         int width, int height,
                         const std::vector<std::vector<float>>& kernel) {
    int kernelSize = kernel.size();
    int halfKernel = kernelSize / 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float sum = 0.0f;

            for (int ky = -halfKernel; ky <= halfKernel; ++ky) {
                for (int kx = -halfKernel; kx <= halfKernel; ++kx) {
                    int nx = clamp(x + kx, 0, width - 1);
                    int ny = clamp(y + ky, 0, height - 1);
                    float weight = kernel[ky + halfKernel][kx + halfKernel];
                    sum += channel[ny][nx] * weight;
                }
            }

            output[y][x] = static_cast<uint8_t>(clamp(static_cast<int>(sum + 0.5f), 0, 255));
        }
    }
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
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <input.bmp> <output.bmp> <sigma>" << std::endl;
        return 1;
    }

    std::string inputFileName = argv[1];
    std::string outputFileName = argv[2];
    float sigma = std::stof(argv[3]);

    if (sigma <= 0) {
        std::cerr << "Error: Sigma must be a positive number." << std::endl;
        return 1;
    }

    int kernelSize = static_cast<int>(2 * (3 * sigma) + 1);

    BMPHeader header;
    BMPInfoHeader infoHeader;
    std::vector<std::vector<uint8_t>> red, green, blue;

    readBMP(inputFileName, header, infoHeader, red, green, blue);

    int width = infoHeader.width;
    int height = infoHeader.height;

    // Generate Gaussian kernel
    std::vector<std::vector<float>> kernel;
    generateGaussianKernel(kernel, kernelSize, sigma);

    // Print the kernel
    printKernel(kernel);

    // Apply Gaussian filter directly on RGB channels
    std::vector<std::vector<uint8_t>> redFiltered(height, std::vector<uint8_t>(width));
    std::vector<std::vector<uint8_t>> greenFiltered(height, std::vector<uint8_t>(width));
    std::vector<std::vector<uint8_t>> blueFiltered(height, std::vector<uint8_t>(width));

    applyGaussianFilter(red, redFiltered, width, height, kernel);
    applyGaussianFilter(green, greenFiltered, width, height, kernel);
    applyGaussianFilter(blue, blueFiltered, width, height, kernel);

    writeBMP(outputFileName, header, infoHeader, redFiltered, greenFiltered, blueFiltered);

    std::cout << "Gaussian smoothing applied directly to RGB channels with sigma = " << sigma << ". Output saved as '"
              << outputFileName << "'." << std::endl;
    return 0;
}