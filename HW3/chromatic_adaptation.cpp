#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <iomanip>

using namespace std;
#pragma pack(push, 1)
// BMP file headers
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

// RGB structure
struct RGB {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
};

// Utility function to clamp a value between 0 and 255
inline uint8_t clamp(int value) {
    return static_cast<uint8_t>((value < 0) ? 0 : (value > 255) ? 255 : value);
}

// Chromatic Adaptation using Grey World method
void applyGreyWorldAdaptation(vector<vector<RGB>>& image) {
    double totalR = 0, totalG = 0, totalB = 0;
    int width = image[0].size();
    int height = image.size();

    for (const auto& row : image) {
        for (const auto& pixel : row) {
            totalR += pixel.red;
            totalG += pixel.green;
            totalB += pixel.blue;
        }
    }

    double meanR = totalR / (width * height);
    double meanG = totalG / (width * height);
    double meanB = totalB / (width * height);
    double meanGray = (meanR + meanG + meanB) / 3.0;

    double R_coef = meanGray / meanR;
    double G_coef = meanGray / meanG;
    double B_coef = meanGray / meanB;

    for (auto& row : image) {
        for (auto& pixel : row) {
            pixel.red = clamp(static_cast<int>(pixel.red * R_coef));
            pixel.green = clamp(static_cast<int>(pixel.green * G_coef));
            pixel.blue = clamp(static_cast<int>(pixel.blue * B_coef));
        }
    }
}

// Chromatic Adaptation using Max-RGB method
void applyMaxRGBAdaptation(vector<vector<RGB>>& image) {
    int maxR = 0, maxG = 0, maxB = 0;

    for (const auto& row : image) {
        for (const auto& pixel : row) {
            maxR = max(maxR, static_cast<int>(pixel.red));
            maxG = max(maxG, static_cast<int>(pixel.green));
            maxB = max(maxB, static_cast<int>(pixel.blue));
        }
    }

    int AVCM = (maxR + maxG + maxB) / 3;
    for (auto& row : image) {
        for (auto& pixel : row) {
            pixel.red = clamp(static_cast<int>(pixel.red * AVCM / maxR));
            pixel.green = clamp(static_cast<int>(pixel.green * AVCM / maxG));
            pixel.blue = clamp(static_cast<int>(pixel.blue * AVCM / maxB));
        }
    }
}

// Read BMP file into a 2D vector of RGB pixels
vector<vector<RGB>> readBMP(const string& filename, BMPFileHeader& fileHeader, BMPInfoHeader& infoHeader) {
    ifstream file(filename, ios::binary);
    if (!file) {
        throw runtime_error("Error opening input file.");
    }

    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (fileHeader.bfType != 0x4D42 || infoHeader.biBitCount != 24) {
        throw runtime_error("Unsupported BMP format. Only 24-bit BMP files are supported.");
    }

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    int padding = (4 - (width * 3) % 4) % 4;

    vector<vector<RGB>> image(height, vector<RGB>(width));
    for (int i = 0; i < height; i++) {
        file.read(reinterpret_cast<char*>(image[i].data()), width * sizeof(RGB));
        file.ignore(padding);
    }

    return image;
}

// Write BMP file from a 2D vector of RGB pixels
void writeBMP(const string& filename, const BMPFileHeader& fileHeader, const BMPInfoHeader& infoHeader, const vector<vector<RGB>>& image) {
    ofstream file(filename, ios::binary);
    if (!file) {
        throw runtime_error("Error opening output file.");
    }

    file.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    int padding = (4 - (width * 3) % 4) % 4;

    for (const auto& row : image) {
        file.write(reinterpret_cast<const char*>(row.data()), width * sizeof(RGB));
        file.write("\0\0\0", padding);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << "<mode> <input.bmp> <output.bmp>\n";
        return 1;
    }

    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;
    string mode = argv[1];
    try {
        auto image = readBMP(argv[2], fileHeader, infoHeader);
        if (mode == "grey") {
            applyGreyWorldAdaptation(image);
            writeBMP(argv[3], fileHeader, infoHeader, image);
        } 
        else if (mode == "max") {
            applyMaxRGBAdaptation(image);
            writeBMP(argv[3], fileHeader, infoHeader, image);
        } 
        else {
            throw runtime_error("Invalid mode. Use 'grey' or 'max'.");
        }
    } 
    catch (const exception& ex) {
        cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
