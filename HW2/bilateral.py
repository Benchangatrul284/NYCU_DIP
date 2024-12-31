import cv2

def denoise_image(input_path, output_path):
    # Read the image
    img = cv2.imread(input_path)

    # Check if the image was loaded successfully
    if img is None:
        print(f"Error: Unable to load image '{input_path}'")
        return

    # Apply bilateral filter
    # Parameters: d (diameter of each pixel neighborhood), sigmaColor, sigmaSpace
    denoised_img = cv2.bilateralFilter(img, d=13, sigmaColor=100, sigmaSpace=100)

    # Save the denoised image
    cv2.imwrite(output_path, denoised_img)

if __name__ == '__main__':
    input_image_path = 'input4.bmp'
    output_image_path = 'output4.bmp'

    denoise_image(input_image_path, output_image_path)
    print(f"Denoised image saved as '{output_image_path}'")