import cv2
from skimage.restoration import estimate_sigma

def estimate_noise(image_path):
    img = cv2.imread(image_path)
    return estimate_sigma(img, average_sigmas=False, channel_axis=-1 )


if __name__ == '__main__':
    image_path = 'input4.bmp'
    noise_estimation = estimate_noise(image_path)

    print(f"Estimated noise: {noise_estimation}")