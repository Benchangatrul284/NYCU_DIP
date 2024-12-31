import numpy as np
import matplotlib.pyplot as plt

# Define power-law transformation function
def power_law_transform(r, gamma):
    return 256 * ((r/256) ** gamma)

# Generate input values (normalized pixel values between 0 and 1)
r = np.linspace(0, 256, 256)

# Apply power-law transformations for gamma = 0.6 and gamma = 1.4
s_gamma_0_6 = power_law_transform(r, 0.6)
s_gamma_1_4 = power_law_transform(r, 1.4)

# Plot the transformations
plt.figure(figsize=(8, 6))
plt.plot(r, s_gamma_0_6, label=r'$\gamma=0.6$', color='blue')
plt.plot(r, s_gamma_1_4, label=r'$\gamma=1.4$', color='red')
plt.xlabel('Input Intensity (r)')
plt.ylabel('Output Intensity (s)')
plt.title('Power-Law Transformation')
plt.legend()
plt.grid()
plt.savefig("Power-Law Transformation.png")