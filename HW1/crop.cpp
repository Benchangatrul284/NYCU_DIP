#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>

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

// Function to crop the image
void cropImage(uint8_t* data, int original_width, int original_height, int bytes_per_pixel, int x, int y, int w, int h, uint8_t* cropped_data) {
    for (int row = 0; row < h; ++row) {
        // Calculate the source and destination row positions
        uint8_t* src_row = data + (y + row) * original_width * bytes_per_pixel;
        uint8_t* dest_row = cropped_data + row * w * bytes_per_pixel;

        // Copy the row within the cropping region
        std::memcpy(dest_row, src_row + x * bytes_per_pixel, w * bytes_per_pixel);
    }
}

int main(int argc, char* argv[]) {
    // Check if the input and output file paths are provided
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input BMP file> <output BMP file>" << std::endl;
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];

    // Open the input BMP file as binary
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

    if (dib_header.bit_count != 8 && dib_header.bit_count != 16 && dib_header.bit_count != 24 && dib_header.bit_count != 32) {
        std::cerr << "Unsupported bit depth: " << dib_header.bit_count << std::endl;
        return 1;
    }

    int width = dib_header.width;
    int height = dib_header.height;
    int bytes_per_pixel = dib_header.bit_count / 8;

    // Show image dimensions
    std::cout << "Image dimensions: " << width << "x" << height << std::endl;

    // Move the file pointer to the pixel data
    input.seekg(bmp_header.offset_data, std::ios::beg);

    // Read pixel data
    int bytes_per_row = (width * bytes_per_pixel + 3) & ~3;  // Row size must be a multiple of 4 bytes
    int data_size = bytes_per_row * height;
    uint8_t* data = new uint8_t[data_size];
    input.read(reinterpret_cast<char*>(data), data_size);

    int x, y, w, h;
    bool valid_input = false;

    // Input loop for valid cropping dimensions
    while (!valid_input) {
        std::cout << "Enter x, y, width, and height for cropping (e.g., 10 10 100 100): ";
        std::cin >> x >> y >> w >> h;

        // Validate the cropping coordinates
        if (x >= 0 && y >= 0 && w > 0 && h > 0 && x + w <= width && y + h <= height) {
            valid_input = true;
        } else {
            std::cerr << "Invalid cropping coordinates. Please try again." << std::endl;
        }
    }

    // Allocate memory for the cropped image
    int cropped_bytes_per_row = (w * bytes_per_pixel + 3) & ~3;  // Row size for cropped image
    int cropped_data_size = cropped_bytes_per_row * h;
    uint8_t* cropped_data = new uint8_t[cropped_data_size];

    // Crop the image
    cropImage(data, width, height, bytes_per_pixel, x, y, w, h, cropped_data);

    // Update BMP and DIB headers for the cropped image
    bmp_header.file_size = sizeof(BMPHeader) + sizeof(DIBHeader) + cropped_data_size;
    dib_header.width = w;
    dib_header.height = h;
    dib_header.image_size = cropped_data_size;

    // Write the cropped image to the output file
    std::ofstream output(output_file, std::ios::binary);
    if (!output) {
        std::cerr << "Error opening output file!" << std::endl;
        delete[] data;
        delete[] cropped_data;
        return 1;
    }

    output.write(reinterpret_cast<char*>(&bmp_header), sizeof(bmp_header));
    output.write(reinterpret_cast<char*>(&dib_header), sizeof(dib_header));
    output.write(reinterpret_cast<char*>(cropped_data), cropped_data_size);

    // Clean up
    delete[] data;
    delete[] cropped_data;
    input.close();
    output.close();

    std::cout << "Cropped image saved as " << output_file << std::endl;

    return 0;
}
