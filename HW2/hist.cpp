#include <iostream>
#include <fstream>
#include <vector>
#include <string>
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

void histogramEqualization(vector<uint8_t>& intensities) {
    int histogram[256] = {0};
    int size = intensities.size();

    for (int i = 0; i < size; i++) {
        histogram[intensities[i]]++;
    }

    int cdf[256] = {0};
    cdf[0] = histogram[0];
    for (int i = 1; i < 256; i++) {
        cdf[i] = cdf[i - 1] + histogram[i];
    }

    int min_cdf = 0;
    for (int i = 0; i < 256; i++) {
        if (cdf[i] > 0) {
            min_cdf = cdf[i];
            break;
        }
    }

    for (int i = 0; i < 256; i++) {
        cdf[i] = (cdf[i] - min_cdf) * 255 / (size - min_cdf);
        if (cdf[i] < 0) cdf[i] = 0;
    }

    for (int i = 0; i < size; i++) {
        intensities[i] = static_cast<uint8_t>(cdf[intensities[i]]);
    }
}

void applyIntensityHistogramEqualization(int width, int height, vector<uint8_t>& red, vector<uint8_t>& green, vector<uint8_t>& blue) {
    int size = width * height;
    vector<uint8_t> intensities(size);

    // Calculate the intensity (average of RGB channels)
    for (int i = 0; i < size; i++) {
        intensities[i] = static_cast<uint8_t>((red[i] + green[i] + blue[i]) / 3.);
    }

    // Apply histogram equalization on intensity
    histogramEqualization(intensities);

    // Calculate the new RGB values based on the new intensity
    for (int i = 0; i < size; i++) {
        if (intensities[i] == 0) {
            red[i] = green[i] = blue[i] = 0;
        } else {
            double ratio = static_cast<double>(intensities[i]) / ((red[i] + green[i] + blue[i]) / 3.0);
            red[i] = min(static_cast<int>(red[i] * ratio), 255);
            green[i] = min(static_cast<int>(green[i] * ratio), 255);
            blue[i] = min(static_cast<int>(blue[i] * ratio), 255);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <input.bmp>" << " <output.bmp>" << endl;
        return 1;
    }

    string inputFileName = argv[1];
    ifstream inFile(inputFileName, ios::binary);
    if (!inFile) {
        cerr << "Error: Could not open input file." << endl;
        return 1;
    }

    string outputFileName = argv[2];

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

    applyIntensityHistogramEqualization(width, height, red, green, blue);

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

    cout << "Intensity-based histogram equalization completed. Output saved as '" << outputFileName << "'." << endl;
    return 0;
}
