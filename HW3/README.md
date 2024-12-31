## Problem 1
g++ chromatic_adaptation.cpp -o chromatic_adaptation.exe
./chromatic_adaptation.exe grey input1.bmp output1_1.bmp
./chromatic_adaptation.exe max input2.bmp output2_1.bmp
./chromatic_adaptation.exe max input3.bmp output3_1.bmp
./chromatic_adaptation.exe grey input4.bmp output4_1.bmp

## Problem 2
g++ enhance.cpp -o enhance.exe
./enhance.exe output1_1.bmp output1_2.bmp --gamma 0.6 --sharpen 0.5
./enhance.exe output2_1.bmp output2_2.bmp --gamma 0.6
./enhance.exe output3_1.bmp output3_2.bmp --gamma 0.6
./enhance.exe output4_1.bmp output4_2.bmp --gamma 1.5 --sigma 0.5

## Problem 3
g++ warm_cool.cpp -o warm_cool.exe
./warm_cool.exe warm output1_2.bmp output1_3.bmp
./warm_cool.exe cool output1_2.bmp output1_4.bmp
./warm_cool.exe warm output2_2.bmp output2_3.bmp
./warm_cool.exe cool output2_2.bmp output2_4.bmp
./warm_cool.exe warm output3_2.bmp output3_3.bmp
./warm_cool.exe cool output3_2.bmp output3_4.bmp
./warm_cool.exe warm output4_2.bmp output4_3.bmp
./warm_cool.exe cool output4_2.bmp output4_4.bmp
