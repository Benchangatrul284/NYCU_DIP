# DIP HW1  
## Problem 1 -- horizontal flip

```bash
g++ -o flip.exe flip.cpp
./flip.exe input1.bmp output1_flip.bmp
./flip.exe input2.bmp output2_flip.bmp
```

## Problem 2 -- resolution 

```bash
g++ -o quantize.exe quantize.cpp
./quantize.exe input1.bmp
./quantize.exe input2.bmp
```

## Problem 3 -- crop

```bash
g++ -o crop.exe crop.cpp
./crop.exe input1.bmp output1_crop.bmp
120 150 400 399
./crop.exe input2.bmp output2_crop.bmp
120 150 100 100
```