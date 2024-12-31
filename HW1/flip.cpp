#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>

#pragma pack(push, 1)  // Ensure no padding
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

void flipHorizontally(uint8_t* data, int width, int height, int bytes_per_pixel) {
    for (int y = 0; y < height; y++) {
        // get the memory address of the beginning of the row
        uint8_t* row = data + y * width * bytes_per_pixel;
        for (int x = 0; x < width / 2; x++) {
            // get the memory address of the pixel at the left side
            uint8_t* left_pixel = row + x * bytes_per_pixel;
            // get the memory address of the pixel at the right side
            uint8_t* right_pixel = row + (width - x - 1) * bytes_per_pixel;
            for (int i = 0; i < bytes_per_pixel; i++) {
                std::swap(left_pixel[i], right_pixel[i]);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    // Check if the input and output file paths are provided
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input BMP file> <output BMP file>" << std::endl;
        return 1;
    }


    // string
    const char* input_file = argv[1];
    const char* output_file = argv[2];

    // Open the input and output files as binary
    std::ifstream input(input_file, std::ios::binary);
    std::ofstream output(output_file, std::ios::binary);

    if (!input) {
        std::cerr << "Error opening input file!" << std::endl;
        return 1;
    }

    if (!output) {
        std::cerr << "Error opening output file!" << std::endl;
        return 1;
    }

    // create BMPHeader and DIBHeader objects
    BMPHeader bmp_header;
    DIBHeader dib_header;

    // Read BMP header
    // The read function reads sizeof(bmp_header) bytes from the input stream 
    // and stores them in the memory location pointed to by 
    // reinterpret_cast<char*>(&bmp_header).

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
    // beg -> beginning
    input.seekg(bmp_header.offset_data, std::ios::beg);

    // Read the pixel data
    // ~3 -> 0xFFFFFFFC (bit mask)
    // we will expect the row of a image to be multiple of 4 bytes (so may need to pad)
    int bytes_per_row = (width * bytes_per_pixel + 3) & ~3;  // Row size must be a multiple of 4 bytes
    int data_size = bytes_per_row * height; // total bytes of the pixel data
    uint8_t* data = new uint8_t[data_size];
    input.read(reinterpret_cast<char*>(data), data_size);

    // Flip the image horizontally
    flipHorizontally(data, width, height, bytes_per_pixel);

    // Write the BMP and DIB headers to the output file
    output.write(reinterpret_cast<char*>(&bmp_header), sizeof(bmp_header));
    output.write(reinterpret_cast<char*>(&dib_header), sizeof(dib_header));

    // Write the flipped pixel data
    output.write(reinterpret_cast<char*>(data), data_size);

    // Clean up
    delete[] data;
    input.close();
    output.close();

    std::cout << "Image flipped and saved as " << output_file << std::endl;

    return 0;
}
