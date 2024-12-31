#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <cstdint>
#include <iomanip> // for setw and setprecision
using namespace std;
// BMP header structures
#pragma pack(push, 1)
struct BMPHeader {
    uint16_t fileType{0x4D42}; // "BM"
    uint32_t fileSize{0};       // Size of file in bytes
    uint16_t reserved1{0};
    uint16_t reserved2{0};
    uint32_t offsetData{0};     // Offset to image data in bytes
};

struct BMPInfoHeader {
    uint32_t size{0};           // Size of this header in bytes
    int32_t width{0};           // Width of bitmap in pixels
    int32_t height{0};          // Height of bitmap in pixels
    uint16_t planes{1};         // Number of color planes (must be 1)
    uint16_t bitCount{0};       // Number of bits per pixel
    uint32_t compression{0};    // Compression type (0 = uncompressed)
    uint32_t imageSize{0};      // Image size (0 for uncompressed)
    int32_t xPixelsPerMeter{0};
    int32_t yPixelsPerMeter{0};
    uint32_t colorsUsed{0};
    uint32_t colorsImportant{0};
};
#pragma pack(pop)

// Generate 2D LoG Kernel with the delta function (center point)
vector<vector<double>> createLoGKernel(double sigma) {
    int radius = static_cast<int>(ceil(3 * sigma));
    int size = 2 * radius + 1;
    vector<vector<double>> kernel(size, vector<double>(size));

    double sigma2 = sigma * sigma;
    double sigma4 = sigma2 * sigma2;
    double normalization_factor = 1;
    double sum = 0;

    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            double distance_squared = x * x + y * y;
            double exponent = -distance_squared / (2 * sigma2);
            kernel[y + radius][x + radius] = normalization_factor * (1 - distance_squared / (2 * sigma2)) * exp(exponent);
            sum += kernel[y + radius][x + radius];
        }
        
    }
    
    
    
    // Print kernel for verification
    cout << "2D LoG Kernel with sigma = " << sigma << ":\n";
    kernel[radius][radius] += 1;

    // 
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            cout << setw(10) << fixed << setprecision(4) << kernel[y][x] << " ";
        }
        cout << "\n";
    }
    return kernel;
}

// Apply 2D convolution
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

// Load BMP image (basic uncompressed 24-bit)
bool loadBMP(const string& filename, BMPHeader& header, BMPInfoHeader& infoHeader, vector<uint8_t>& imageData) {
    ifstream file(filename, ios::binary);
    if (!file) {
        cerr << "Unable to open file " << filename << endl;
        return false;
    }

    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (infoHeader.bitCount != 24 || infoHeader.compression != 0) {
        cerr << "Only uncompressed 24-bit BMP files are supported." << endl;
        return false;
    }

    file.seekg(header.offsetData, file.beg);
    int imageSize = infoHeader.width * infoHeader.height * 3;
    imageData.resize(imageSize);
    file.read(reinterpret_cast<char*>(imageData.data()), imageSize);

    file.close();
    return true;
}

// Save BMP image
bool saveBMP(const string& filename, const BMPHeader& header, const BMPInfoHeader& infoHeader, const vector<uint8_t>& imageData) {
    ofstream file(filename, ios::binary);
    if (!file) {
        cerr << "Unable to open file " << filename << endl;
        return false;
    }

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
    file.write(reinterpret_cast<const char*>(imageData.data()), imageData.size());

    file.close();
    return true;
}

// Main sharpening function
void sharpenImage(const string& inputFilename, const string& outputFilename, double sigma) {
    BMPHeader header;
    BMPInfoHeader infoHeader;
    vector<uint8_t> imageData;

    if (!loadBMP(inputFilename, header, infoHeader, imageData)) {
        return;
    }

    int width = infoHeader.width;
    int height = infoHeader.height;
    int imageSize = width * height;

    // Separate color channels
    vector<uint8_t> redChannel(imageSize);
    vector<uint8_t> greenChannel(imageSize);
    vector<uint8_t> blueChannel(imageSize);

    for (int i = 0; i < imageSize; i++) {
        blueChannel[i] = imageData[3 * i];
        greenChannel[i] = imageData[3 * i + 1];
        redChannel[i] = imageData[3 * i + 2];
    }

    // Apply LoG filter
    vector<uint8_t> redOutput(imageSize);
    vector<uint8_t> greenOutput(imageSize);
    vector<uint8_t> blueOutput(imageSize);

    auto logKernel = createLoGKernel(sigma);
    convolve2D(logKernel, redChannel, redOutput, width, height);
    convolve2D(logKernel, greenChannel, greenOutput, width, height);
    convolve2D(logKernel, blueChannel, blueOutput, width, height);

    // Reconstruct image
    for (int i = 0; i < imageSize; i++) {
        imageData[3 * i] = blueOutput[i];
        imageData[3 * i + 1] = greenOutput[i];
        imageData[3 * i + 2] = redOutput[i];
    }

    // Save the sharpened image
    saveBMP(outputFilename, header, infoHeader, imageData);
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <input BMP> <output BMP> <sigma>" << endl;
        return 1;
    }

    string inputFilename = argv[1];
    string outputFilename = argv[2];
    double sigma = stod(argv[3]);

    sharpenImage(inputFilename, outputFilename, sigma);

    return 0;
}
