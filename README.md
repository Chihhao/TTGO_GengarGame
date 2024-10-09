# TTGO_GengarGame  
Code Ref: https://github.com/VolosR/TRexTTGOdisplay  
Gengar Image Ref: https://www.deviantart.com/xxlpanda/art/Gengar-Sprite-795339646  

## Features  
* 3 heroes with different difficulty (speed)
* 3 heroes with different skill
* Enter sleep mode if no button pressed in 12 seconds  
* Keep the maximum score in eeprom
* Battery Icon and charging animation (Built-In ADC)

## Board  
TTGO T-Display V1.1  

## Arduino Library Version 
Arduino IDE: 1.8.19  
ESP32 Library Version: 2.0.2  
Button2 Library Version: 1.4.0  
TFT_eSPI Library Version: 2.4.32  
* In TFT_eSPI Library, mark #include <User_Setup.h>, and unmark #include <User_Setups/Setup25_TTGO_T_Display.h> 

## 3D Print Case  
Ref: https://www.thingiverse.com/thing:4501444  
I reviced it to fit 803040 1000mAh battery. 

## Pictrue  
![image](https://github.com/Chihhao/TTGO_GengarGame/blob/main/image/Demo_New.gif)  
![image](https://github.com/Chihhao/TTGO_GengarGame/blob/main/image/image01.jpg)  
![image](https://github.com/Chihhao/TTGO_GengarGame/blob/main/image/image02.jpg)  
![image](https://github.com/Chihhao/TTGO_GengarGame/blob/main/image/image03.png)  
![image](https://github.com/Chihhao/TTGO_GengarGame/blob/main/image/image04.jpg)  

## Image to C Code  
* use lcd-image-converter  
* https://thesolaruniverse.wordpress.com/2021/11/05/displaying-color-pictures-on-a-240x240-tft-screen-with-st7789-controller-with-an-esp32-wroom-32/   
Type: Color 
Color R5G6B5 
Block Size: 16bit 
Byte Order: Big-Endian 
