#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iomanip>

using namespace std;

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

inline int clamp(int value, int minVal, int maxVal) {
    return max(minVal, min(value, maxVal));
}

inline double clamp(double value, double minVal, double maxVal) {
    return max(minVal, min(value, maxVal));
}

vector<vector<double>> createLoGKernel(double sigma) {
    int radius = static_cast<int>(ceil(3 * sigma));
    int size = 2 * radius + 1;
    vector<vector<double>> kernel(size, vector<double>(size));

    double sigma2 = sigma * sigma;
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            double distanceSquared = x * x + y * y;
            double exponent = -distanceSquared / (2 * sigma2);
            kernel[y + radius][x + radius] = (1 - distanceSquared / (2 * sigma2)) * exp(exponent);
        }
    }
    kernel[radius][radius] += 1; // Delta function
    return kernel;
}

void generateGaussianKernel(vector<vector<double>>& kernel, int kernelSize, double sigma) {
    int radius = kernelSize / 2;
    double sum = 0.0;

    for (int y = -radius; y <= radius; ++y) {
        vector<double> row;
        for (int x = -radius; x <= radius; ++x) {
            double value = exp(-(x * x + y * y) / (2 * sigma * sigma));
            row.push_back(value);
            sum += value;
        }
        kernel.push_back(row);
    }

    // Normalize kernel
    for (auto& row : kernel) {
        for (auto& value : row) {
            value /= sum;
        }
    }
}

void convolve2D(const vector<vector<double>>& kernel, const vector<uint8_t>& src, vector<uint8_t>& dst, int width, int height) {
    int kRadius = kernel.size() / 2;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double sum = 0.0;
            for (int ky = -kRadius; ky <= kRadius; ky++) {
                for (int kx = -kRadius; kx <= kRadius; kx++) {
                    int ix = clamp(x + kx, 0, width - 1);
                    int iy = clamp(y + ky, 0, height - 1);
                    sum += src[iy * width + ix] * kernel[ky + kRadius][kx + kRadius];
                }
            }
            dst[y * width + x] = static_cast<uint8_t>(clamp(sum, 0.0, 255.0));
        }
    }
}

void applyGaussianFilter(const vector<vector<double>>& kernel, vector<uint8_t>& channel, int width, int height) {
    vector<uint8_t> output(channel.size());
    convolve2D(kernel, channel, output, width, height);
    channel = move(output);
}

void gammaCorrection(vector<uint8_t>& channel, double gamma) {
    for (auto& value : channel) {
        double normalized = static_cast<double>(value) / 255.0;
        value = static_cast<uint8_t>(pow(normalized, gamma) * 255);
    }
}

bool loadBMP(const string& filename, BMPHeader& header, BMPInfoHeader& infoHeader, vector<uint8_t>& imageData) {
    ifstream file(filename, ios::binary);
    if (!file) {
        cerr << "Unable to open file " << filename << endl;
        return false;
    }

    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (header.fileType != 0x4D42 || infoHeader.bitCount != 24) {
        cerr << "Only uncompressed 24-bit BMP files are supported." << endl;
        return false;
    }

    int width = infoHeader.width;
    int height = infoHeader.height;
    int padding = (4 - (width * 3) % 4) % 4;

    file.seekg(header.offsetData, ios::beg);
    imageData.resize(height * width * 3);
    for (int i = 0; i < height; i++) {
        file.read(reinterpret_cast<char*>(imageData.data() + i * width * 3), width * 3);
        file.ignore(padding);
    }
    return true;
}

bool saveBMP(const string& filename, const BMPHeader& header, const BMPInfoHeader& infoHeader, const vector<uint8_t>& imageData) {
    ofstream file(filename, ios::binary);
    if (!file) {
        cerr << "Unable to open file " << filename << endl;
        return false;
    }

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    int width = infoHeader.width;
    int height = infoHeader.height;
    int padding = (4 - (width * 3) % 4) % 4;

    for (int i = 0; i < height; i++) {
        file.write(reinterpret_cast<const char*>(imageData.data() + i * width * 3), width * 3);
        file.write("\0\0\0", padding);
    }
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <input.bmp> <output.bmp> [--sharpen <sigma>] [--gamma <gamma>] [--sigma <value>]" << endl;
        return 1;
    }

    string inputFileName = argv[1];
    string outputFileName = argv[2];

    BMPHeader header;
    BMPInfoHeader infoHeader;
    vector<uint8_t> imageData;

    if (!loadBMP(inputFileName, header, infoHeader, imageData)) {
        return 1;
    }

    int width = infoHeader.width;
    int height = infoHeader.height;
    int imageSize = width * height;

    vector<uint8_t> red(imageSize), green(imageSize), blue(imageSize);

    for (int i = 0; i < imageSize; i++) {
        blue[i] = imageData[3 * i];
        green[i] = imageData[3 * i + 1];
        red[i] = imageData[3 * i + 2];
    }

    double sharpenSigma = 0.0, gamma = 0.0, gaussianSigma = 0.0;
    bool doSharpen = false, doGamma = false, doGaussian = false;

    for (int i = 3; i < argc; i++) {
        if (string(argv[i]) == "--sharpen" && i + 1 < argc) {
            sharpenSigma = stod(argv[i + 1]);
            doSharpen = true;
            i++;
        } else if (string(argv[i]) == "--gamma" && i + 1 < argc) {
            gamma = stod(argv[i + 1]);
            doGamma = true;
            i++;
        } else if (string(argv[i]) == "--sigma" && i + 1 < argc) {
            gaussianSigma = stod(argv[i + 1]);
            doGaussian = true;
            i++;
        }
    }

    if (doGaussian) {
        int kernelSize = static_cast<int>(2 * (3 * gaussianSigma) + 1);
        vector<vector<double>> gaussianKernel;
        generateGaussianKernel(gaussianKernel, kernelSize, gaussianSigma);

        applyGaussianFilter(gaussianKernel, red, width, height);
        applyGaussianFilter(gaussianKernel, green, width, height);
        applyGaussianFilter(gaussianKernel, blue, width, height);

        cout << "Gaussian smoothing applied with sigma = " << gaussianSigma << endl;
    }

    if (doGamma) {
        gammaCorrection(red, gamma);
        gammaCorrection(green, gamma);
        gammaCorrection(blue, gamma);
        cout << "Gamma Correction: " << gamma << endl;
    }

    for (int i = 0; i < imageSize; i++) {
        imageData[3 * i] = blue[i];
        imageData[3 * i + 1] = green[i];
        imageData[3 * i + 2] = red[i];
    }

    if (!saveBMP(outputFileName, header, infoHeader, imageData)) {
        return 1;
    }

    cout << "Processing completed successfully!" << endl;
    return 0;
}
