#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
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
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <input.bmp> <output.bmp> <kernel_size>" << endl;
        return 1;
    }

    string inputFileName = argv[1];
    string outputFileName = argv[2];
    int kernelSize = stoi(argv[3]);

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

    applyMedianFilter(red, redFiltered, width, height, kernelSize);
    applyMedianFilter(green, greenFiltered, width, height, kernelSize);
    applyMedianFilter(blue, blueFiltered, width, height, kernelSize);

    writeBMP(outputFileName, header, infoHeader, redFiltered, greenFiltered, blueFiltered);

    cout << "Median filter applied. Output saved as '" << outputFileName << "'." << endl;
    return 0;
}
