import numpy as np
import matplotlib.pyplot as plt

def create_log_kernel(sigma):
    radius = int(np.ceil(3 * sigma))
    size = 2 * radius + 1
    kernel = np.zeros((size, size))

    sigma2 = sigma * sigma
    normalization_factor = 1

    for y in range(-radius, radius + 1):
        for x in range(-radius, radius + 1):
            distance_squared = x * x + y * y
            exponent = -distance_squared / (2 * sigma2)
            kernel[y + radius, x + radius] = normalization_factor * (1 - distance_squared / (2 * sigma2)) * np.exp(exponent)

    # Add delta function (center point)
    kernel[radius, radius] += 1

    return kernel

def plot_kernel(kernel, sigma):
    plt.figure(figsize=(8, 6))
    plt.imshow(kernel, cmap='viridis', interpolation='none')
    plt.colorbar()
    plt.title(f'2D LoG Kernel with sigma = {sigma}')
    plt.xlabel('X-axis')
    plt.ylabel('Y-axis')
    plt.savefig(f"2D LoG Kernel {sigma}.png")

# Example usage
sigma = 0.5
log_kernel = create_log_kernel(sigma)
plot_kernel(log_kernel, sigma)