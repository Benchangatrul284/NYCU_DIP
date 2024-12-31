#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <cstdint>
#include <cmath>
#include <iomanip>

using namespace std;
#pragma pack(push, 1) // Ensure no padding for BMP header
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

void applyMedianFilter(const vector<vector<uint8_t>>& channel, vector<vector<uint8_t>>& output, int width, int height, int kernelSize) {
    int halfKernel = kernelSize / 2;
    vector<uint8_t> window;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            window.clear();

            // Collect pixels within the kernel
            for (int ky = -halfKernel; ky <= halfKernel; ++ky) {
                for (int kx = -halfKernel; kx <= halfKernel; ++kx) {
                    int nx = clamp(x + kx, 0, width - 1);
                    int ny = clamp(y + ky, 0, height - 1);
                    window.push_back(channel[ny][nx]);
                }
            }

            // Sort and pick the median value
            sort(window.begin(), window.end());
            output[y][x] = window[window.size() / 2];
        }
    }
}

void applyBilateralFilter(const std::vector<std::vector<uint8_t>>& channel,
                          std::vector<std::vector<uint8_t>>& output,
                          int width, int height,
                          int kernelSize,
                          float sigmaSpatial = 4,
                          float sigmaRange = 100) {
    int halfKernel = kernelSize / 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float sum = 0.0f;
            float normFactor = 0.0f;
            float centerIntensity = static_cast<float>(channel[y][x]);

            for (int ky = -halfKernel; ky <= halfKernel; ++ky) {
                for (int kx = -halfKernel; kx <= halfKernel; ++kx) {
                    int nx = clamp(x + kx, 0, width - 1);
                    int ny = clamp(y + ky, 0, height - 1);

                    float neighborIntensity = static_cast<float>(channel[ny][nx]);

                    float spatialDistance = kx * kx + ky * ky;
                    float spatialWeight = std::exp(-spatialDistance / (2 * sigmaSpatial * sigmaSpatial));

                    float intensityDifference = neighborIntensity - centerIntensity;
                    float rangeWeight = std::exp(-(intensityDifference * intensityDifference) / (2 * sigmaRange * sigmaRange));

                    float weight = spatialWeight * rangeWeight;

                    sum += neighborIntensity * weight;
                    normFactor += weight;
                }
            }

            output[y][x] = static_cast<uint8_t>(clamp(static_cast<int>(sum / normFactor + 0.5f), 0, 255));
        }
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

void applyMaxFilter(const vector<vector<uint8_t>>& channel, vector<vector<uint8_t>>& output, int width, int height, int kernelSize) {
    int halfKernel = kernelSize / 2;
    vector<uint8_t> window;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            window.clear();

            // Collect pixels within the kernel
            for (int ky = -halfKernel; ky <= halfKernel; ++ky) {
                for (int kx = -halfKernel; kx <= halfKernel; ++kx) {
                    int nx = clamp(x + kx, 0, width - 1);
                    int ny = clamp(y + ky, 0, height - 1);
                    window.push_back(channel[ny][nx]);
                }
            }

            // Sort and pick the median value
            sort(window.begin(), window.end());
            output[y][x] = window[window.size() - 1];
        }
    }
}

void applyMidpointFilter(const std::vector<std::vector<uint8_t>>& channel,
                         std::vector<std::vector<uint8_t>>& output,
                         int width, int height, int kernelSize) {
    int halfKernel = kernelSize / 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t minVal = 255;
            uint8_t maxVal = 0;

            // Collect pixels within the kernel
            for (int ky = -halfKernel; ky <= halfKernel; ++ky) {
                for (int kx = -halfKernel; kx <= halfKernel; ++kx) {
                    int nx = clamp(x + kx, 0, width - 1);
                    int ny = clamp(y + ky, 0, height - 1);
                    uint8_t pixelValue = channel[ny][nx];
                    minVal = std::min(minVal, pixelValue);
                    maxVal = std::max(maxVal, pixelValue);
                }
            }

            // Compute the average of min and max values
            output[y][x] = static_cast<uint8_t>((minVal + maxVal) / 2);
        }
    }
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

void readBMP(const string& filename, BMPHeader& header, BMPInfoHeader& infoHeader, 
             vector<vector<uint8_t>>& red, 
             vector<vector<uint8_t>>& green, 
             vector<vector<uint8_t>>& blue) {
    ifstream inFile(filename, ios::binary);
    if (!inFile) {
        cerr << "Error: Could not open input file." << endl;
        exit(1);
    }

    inFile.read(reinterpret_cast<char*>(&header), sizeof(header));
    inFile.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (header.fileType != 0x4D42 || infoHeader.bitCount != 24) {
        cerr << "Error: Only 24-bit BMP format is supported." << endl;
        exit(1);
    }

    int width = infoHeader.width;
    int height = infoHeader.height;
    int padding = (4 - (width * 3) % 4) % 4;

    red.resize(height, vector<uint8_t>(width));
    green.resize(height, vector<uint8_t>(width));
    blue.resize(height, vector<uint8_t>(width));

    inFile.seekg(header.offsetData, ios::beg);
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

void writeBMP(const string& filename, const BMPHeader& header, const BMPInfoHeader& infoHeader, 
              const vector<vector<uint8_t>>& red, 
              const vector<vector<uint8_t>>& green, 
              const vector<vector<uint8_t>>& blue) {
    ofstream outFile(filename, ios::binary);
    if (!outFile) {
        cerr << "Error: Could not open output file." << endl;
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
    if (argc < 5) {
        cerr << "Usage: " << argv[0] << " <mode> <input.bmp> <output.bmp> <kernel_size>" << endl;
        return 1;
    }

    string mode = argv[1];
    string inputFileName = argv[2];
    string outputFileName = argv[3];
    int kernelSize = stoi(argv[4]);

    if (kernelSize % 2 == 0 || kernelSize < 3) {
        cerr << "Error: Kernel size must be an odd integer >= 3." << endl;
        return 1;
    }

    BMPHeader header;
    BMPInfoHeader infoHeader;
    vector<vector<uint8_t>> red, green, blue;

    readBMP(inputFileName, header, infoHeader, red, green, blue);

    int width = infoHeader.width;
    int height = infoHeader.height;

    vector<vector<uint8_t>> redFiltered(height, vector<uint8_t>(width));
    vector<vector<uint8_t>> greenFiltered(height, vector<uint8_t>(width));
    vector<vector<uint8_t>> blueFiltered(height, vector<uint8_t>(width));

    float sigmaSpatial = 200;
    float sigmaRange = 200;
    if (mode == "bilateral") {
        applyBilateralFilter(red, redFiltered, width, height, kernelSize);
        applyBilateralFilter(green, greenFiltered, width, height, kernelSize);
        applyBilateralFilter(blue, blueFiltered, width, height, kernelSize);
        cout << "Bilateral filter applied"<< endl;
    }
    else if (mode == "medium") {
        applyMedianFilter(red, redFiltered, width, height, kernelSize);
        applyMedianFilter(green, greenFiltered, width, height, kernelSize);
        applyMedianFilter(blue, blueFiltered, width, height, kernelSize);
        cout << "Medium filter applied"<< endl;
    } else if (mode == "max") {
        applyMaxFilter(red, redFiltered, width, height, kernelSize);
        applyMaxFilter(green, greenFiltered, width, height, kernelSize);
        applyMaxFilter(blue, blueFiltered, width, height, kernelSize);
        cout << "Max filter applied"<< endl;
    } else if (mode == "midpoint") {
        applyMidpointFilter(red, redFiltered, width, height, kernelSize);
        applyMidpointFilter(green, greenFiltered, width, height, kernelSize);
        applyMidpointFilter(blue, blueFiltered, width, height, kernelSize);
        cout << "Midpoint filter applied"<< endl;
    } else if (mode == "gaussian") {
        float sigma = (kernelSize-1) / 6.;
        vector<vector<float>> kernel;
        generateGaussianKernel(kernel, kernelSize, sigma);
        applyGaussianFilter(red, redFiltered, width, height, kernel);
        applyGaussianFilter(green, greenFiltered, width, height, kernel);
        applyGaussianFilter(blue, blueFiltered, width, height, kernel);
        cout << "Gaussian filter applied"<< endl;
    } else {
        cerr << "Error: Invalid mode." << endl;
        return 1;
    }
    
    writeBMP(outputFileName, header, infoHeader, redFiltered, greenFiltered, blueFiltered);

    cout << "Output saved as '" << outputFileName << "'." << endl;
    return 0;
}
