from skimage.metrics import structural_similarity as ssim
import cv2

def calculate_ssim(original_image_path, noised_image_path):
    # Load images
    original = cv2.imread(original_image_path, cv2.IMREAD_GRAYSCALE)
    noised = cv2.imread(noised_image_path, cv2.IMREAD_GRAYSCALE)
    
    # Ensure images are the same size
    if original.shape != noised.shape:
        raise ValueError("Images must have the same dimensions for SSIM calculation.")
    
    # Calculate SSIM
    score, diff = ssim(original, noised, full=True)
    
    return score

# Example usage
original_image_path = 'input3_org.bmp'
noised_image_path = 'output3.bmp'
ssim_value = calculate_ssim(original_image_path, noised_image_path)

print(f"SSIM value: {ssim_value}")
