from PIL import Image
import numpy as np
import matplotlib.pyplot as plt

def generate_histogram_and_cdf(image_path, output_path):
    # Load image and convert to grayscale
    image = Image.open(image_path).convert('L')
    image_array = np.array(image)

    # Flatten the array and compute histogram
    flat = image_array.flatten()
    hist, bins = np.histogram(flat, bins=256, range=[0, 256])

    # Compute CDF
    cdf = hist.cumsum()
    cdf_normalized = cdf * hist.max() / cdf.max()

    # Plot histogram and CDF
    plt.figure(figsize=(12, 5))

    plt.subplot(1, 2, 1)
    plt.bar(range(256), hist, color='gray')
    plt.title('Histogram')
    plt.xlabel('Pixel Intensity')
    plt.ylabel('Frequency')

    plt.subplot(1, 2, 2)
    plt.plot(cdf_normalized, color='red')
    plt.title('Cumulative Distribution Function (CDF)')
    plt.xlabel('Pixel Intensity')
    plt.ylabel('Cumulative Frequency')

    plt.tight_layout()
    plt.savefig(output_path)
    plt.close()

# Example usage
generate_histogram_and_cdf('input1.bmp', 'histogram_cdf_output.png')