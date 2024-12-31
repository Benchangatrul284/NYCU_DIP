## Problem 1 - Low-luminosity Enhancement

```bash
g++ hist.cpp -o hist.exe
./hist.exe input1.bmp output1.bmp
```

```bash
g++ gamma.cpp -o gamma.exe
./gamma.exe input1.bmp output1.bmp 0.6
```

## Problem 2 - Sharpness Enhancement
```bash
g++ sharpen.cpp -o sharpen.exe
./sharpen.exe input2.bmp output2_1.bmp 1
./sharpen.exe input2.bmp output2_2.bmp 3
```

## Problem 3 - Denoise
```bash
g++ denoise.cpp -o denoise.exe
./denoise.exe medium input3.bmp output3_1.bmp 3
./denoise.exe max input3.bmp output3_2.bmp 3
./denoise.exe bilateral input4.bmp output4_1.bmp 19
./denoise.exe gaussian input4.bmp output4_2.bmp 7
```