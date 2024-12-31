#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <regex>

// Ensure no padding in structs
#pragma pack(push, 1)
struct BMPHeader {
    uint16_t file_type;   // File type (must be 'BM') -> 2 bytes
    uint32_t file_size;   // Size of the file (in bytes) -> 4 bytes
    uint16_t reserved1;   // Reserved, always 0 -> 2 bytes
    uint16_t reserved2;   // Reserved, always 0 -> 2 bytes
    uint32_t offset_data; // Start position of pixel data -> 4 bytes
};

struct DIBHeader {
    uint32_t size;            // Size of this header (in bytes)
    int32_t width;            // Width of the image (in pixels)
    int32_t height;           // Height of the image (in pixels)
    uint16_t planes;          // Number of color planes (must be 1)
    uint16_t bit_count;       // Bits per pixel (24 for RGB)
    uint32_t compression;     // Compression type (0 = uncompressed)
    uint32_t image_size;      // Size of the raw bitmap data (0 if no compression)
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;     // Number of colors in the palette
    uint32_t important_colors;
};
#pragma pack(pop)

// Apply quantization based on the given bit depth
void applyQuantization(uint8_t* data, int width, int height, int bytes_per_pixel, int bits_per_channel) {
    uint8_t mask = (0xFF << (8 - bits_per_channel));  // Create a mask for the desired bit depth
    // uint8_t mask = (0xFF >> (8 - bits_per_channel));  // Create a mask for the desired bit depth
    for (int y = 0; y < height; y++) {
        uint8_t* row = data + y * width * bytes_per_pixel;
        for (int x = 0; x < width; x++) {
            uint8_t* pixel = row + x * bytes_per_pixel;
            for (int i = 0; i < 3; i++) {  // Apply quantization to R, G, and B channels (ignore alpha if 32bpp)
                pixel[i] = (pixel[i] & mask);  // Zero out the least significant bits
            }
        }
    }
}

// Flip the image horizontally
void flipHorizontally(uint8_t* data, int width, int height, int bytes_per_pixel) {
    for (int y = 0; y < height; y++) {
        uint8_t* row = data + y * width * bytes_per_pixel;
        for (int x = 0; x < width / 2; x++) {
            uint8_t* left_pixel = row + x * bytes_per_pixel;
            uint8_t* right_pixel = row + (width - x - 1) * bytes_per_pixel;
            for (int i = 0; i < bytes_per_pixel; i++) {
                std::swap(left_pixel[i], right_pixel[i]);
            }
        }
    }
}

// Helper function to extract number from the input filename
std::string extractNumber(const std::string& filename) {
    std::regex re("\\d+");  // Regex to match numbers
    std::smatch match;
    if (std::regex_search(filename, match, re)) {
        return match.str();  // Return the first match (e.g., "1" from "input1.bmp")
    }
    return "";
}

int main(int argc, char* argv[]) {
    // Check if the input file path is provided
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input BMP file>" << std::endl;
        return 1;
    }

    const char* input_file = argv[1];

    // Extract the number from the input filename (e.g., "1" from "input1.bmp")
    std::string input_filename(input_file);
    std::string file_number = extractNumber(input_filename);
    if (file_number.empty()) {
        std::cerr << "Invalid input filename format. Could not extract a number." << std::endl;
        return 1;
    }

    // Create output filenames based on the extracted number
    std::string output_file1 = "output" + file_number + "_1.bmp";  // For 6-bit quantization
    std::string output_file2 = "output" + file_number + "_2.bmp";  // For 4-bit quantization
    std::string output_file3 = "output" + file_number + "_3.bmp";  // For 2-bit quantization

    // Open the input file as binary
    std::ifstream input(input_file, std::ios::binary);
    if (!input) {
        std::cerr << "Error opening input file!" << std::endl;
        return 1;
    }

    BMPHeader bmp_header;
    DIBHeader dib_header;

    // Read BMP header
    input.read(reinterpret_cast<char*>(&bmp_header), sizeof(bmp_header));
    if (bmp_header.file_type != 0x4D42) {  // 'BM' in hex
        std::cerr << "Not a BMP file!" << std::endl;
        return 1;
    }

    // Read DIB header
    input.read(reinterpret_cast<char*>(&dib_header), sizeof(dib_header));

    if (dib_header.bit_count != 24 && dib_header.bit_count != 32) {
        std::cerr << "Unsupported bit depth: " << dib_header.bit_count << std::endl;
        return 1;
    }

    int width = dib_header.width;
    int height = dib_header.height;
    int bytes_per_pixel = dib_header.bit_count / 8;

    // Move the file pointer to the beginning of the pixel data
    input.seekg(bmp_header.offset_data, std::ios::beg);

    // Read the pixel data
    int bytes_per_row = (width * bytes_per_pixel + 3) & ~3;  // Row size must be a multiple of 4 bytes
    int data_size = bytes_per_row * height;  // Total bytes of the pixel data
    uint8_t* data = new uint8_t[data_size];
    input.read(reinterpret_cast<char*>(data), data_size);

    // Process the three output files with different bit depths (6, 4, 2 bits per channel)
    const int bit_depths[3] = {6, 4, 2};
    const std::string output_files[3] = {output_file1, output_file2, output_file3};

    for (int i = 0; i < 3; ++i) {
        // Apply quantization for each bit depth
        uint8_t* data_copy = new uint8_t[data_size];
        std::memcpy(data_copy, data, data_size);
        applyQuantization(data_copy, width, height, bytes_per_pixel, bit_depths[i]);

        // Open output file
        std::ofstream output(output_files[i], std::ios::binary);
        if (!output) {
            std::cerr << "Error opening output file " << output_files[i] << "!" << std::endl;
            delete[] data_copy;
            return 1;
        }

        // Write the BMP and DIB headers to the output file
        output.write(reinterpret_cast<char*>(&bmp_header), sizeof(bmp_header));
        output.write(reinterpret_cast<char*>(&dib_header), sizeof(dib_header));

        // Write the modified pixel data
        output.write(reinterpret_cast<char*>(data_copy), data_size);
        output.close();

        // Clean up the copied data
        delete[] data_copy;

        std::cout << "Image saved as " << output_files[i] << " with " << bit_depths[i] << "-bit quantization." << std::endl;
    }

    // Clean up
    delete[] data;
    input.close();

    return 0;
}
