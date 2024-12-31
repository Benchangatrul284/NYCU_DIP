#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
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

void gammaCorrection(vector<uint8_t>& channel, double gamma) {
    for (auto& value : channel) {
        double normalized = static_cast<double>(value) / 255.0;
        value = static_cast<uint8_t>(pow(normalized, gamma) * 255);
    }
}

void applyGammaCorrection(int width, int height, vector<uint8_t>& red, vector<uint8_t>& green, vector<uint8_t>& blue, double gamma) {
    gammaCorrection(red, gamma);
    gammaCorrection(green, gamma);
    gammaCorrection(blue, gamma);
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <input.bmp> <output.bmp> <gamma>" << endl;
        return 1;
    }

    string inputFileName = argv[1];
    string outputFileName = argv[2];
    double gamma = stod(argv[3]);

    ifstream inFile(inputFileName, ios::binary);
    if (!inFile) {
        cerr << "Error: Could not open input file." << endl;
        return 1;
    }

    BMPHeader header;
    BMPInfoHeader infoHeader;
    inFile.read(reinterpret_cast<char*>(&header), sizeof(header));
    inFile.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (header.fileType != 0x4D42 || infoHeader.bitCount != 24) {
        cerr << "Error: Only 24-bit BMP format is supported." << endl;
        return 1;
    }

    int width = infoHeader.width;
    int height = infoHeader.height;
    int padding = (4 - (width * 3) % 4) % 4;

    vector<uint8_t> red, green, blue;
    uint8_t pixel[3];

    inFile.seekg(header.offsetData, ios::beg);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            inFile.read(reinterpret_cast<char*>(pixel), 3);
            blue.push_back(pixel[0]);
            green.push_back(pixel[1]);
            red.push_back(pixel[2]);
        }
        inFile.ignore(padding);
    }
    inFile.close();

    applyGammaCorrection(width, height, red, green, blue, gamma);

    ofstream outFile(outputFileName, ios::binary);
    outFile.write(reinterpret_cast<char*>(&header), sizeof(header));
    outFile.write(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    int index = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            pixel[0] = blue[index];
            pixel[1] = green[index];
            pixel[2] = red[index];
            outFile.write(reinterpret_cast<char*>(pixel), 3);
            index++;
        }
        outFile.write("\0\0\0", padding);
    }
    outFile.close();

    cout << "Gamma correction completed with gamma = " << gamma << ". Output saved as '" << outputFileName << "'." << endl;
    return 0;
}
