/*
 * LCD_GFX.c
 *
 * Created: 9/20/2021 6:54:25 PM
 *  Author: You
 */ 

#include "LCD_GFX.h"
#include "ST7735.h"
#include <stdlib.h>

/******************************************************************************
* Local Functions
******************************************************************************/



/******************************************************************************
* Global Functions
******************************************************************************/

/**************************************************************************//**
* @fn			uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
* @brief		Convert RGB888 value to RGB565 16-bit color data
* @note
*****************************************************************************/
uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
	return ((((31*(red+4))/255)<<11) | (((63*(green+2))/255)<<5) | ((31*(blue+4))/255));
}

/**************************************************************************//**
* @fn			void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color)
* @brief		Draw a single pixel of 16-bit rgb565 color to the x & y coordinate
* @note
*****************************************************************************/
void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color) {
	LCD_setAddr(x,y,x,y);
	SPI_ControllerTx_16bit(color);
}

/**************************************************************************//**
* @fn			void LCD_drawChar(uint8_t x, uint8_t y, uint16_t character, uint16_t fColor, uint16_t bColor)
* @brief		Draw a character starting at the point with foreground and background colors
* @note
*****************************************************************************/
void LCD_drawChar(uint8_t x, uint8_t y, uint16_t character, uint16_t fColor, uint16_t bColor, uint8_t size){
	
    //FIXED: changed the char sizes to be scaled
    
    uint16_t row = character - 0x20;            //Determine row of ASCII table starting at space
    int i, j, k, l;

    if ((LCD_WIDTH - x > 5 * size) && (LCD_HEIGHT - y > 8 * size)) {
        for (i = 0; i < 5; i++) {
            uint8_t pixels = ASCII[row][i];     //Go through the list of pixels
            for (j = 0; j < 8; j++) {
                uint16_t color = (pixels >> j) & 1 ? fColor : bColor;
                for (k = 0; k < size; k++) {
                    for (l = 0; l < size; l++) {
                        LCD_drawPixel(x + i * size + k, y + j * size + l, color);
                    }
                }
            }
        }
    }
    
}


/******************************************************************************
* LAB 4 TO DO. COMPLETE THE FUNCTIONS BELOW.
* You are free to create and add any additional files, libraries, and/or
*  helper function. All code must be authentically yours.
******************************************************************************/

/**************************************************************************//**
* @fn			void LCD_drawCircle(uint8_t x0, uint8_t y0, uint8_t radius,uint16_t color)
* @brief		Draw a colored circle of set radius at coordinates
* @note
*****************************************************************************/
void LCD_drawCircle(uint8_t x0, uint8_t y0, uint8_t radius,uint16_t color)
{
    /*//set all the addresses going from each vertex
    for (int x = x0 - radius; x <= x0 + radius; x++) {
        for (int y = y0 - radius; y <= y0 + radius; y++) {
            // check if the point is within the circle
            if (((x - x0) * (x - x0) + (y - y0) * (y - y0)) <= (radius * radius)) {
                LCD_setAddr(x, y, x, y);
                SPI_ControllerTx_16bit(color);
            }
        }
    }*/ // too slow ):
    
    for(int x = 0, y = radius, d = 3-2*radius; x <= y; x++){
        LCD_drawPixel(x0 + x, y0 + y, color);
        LCD_drawPixel(x0 - x, y0 + y, color);
        LCD_drawPixel(x0 + x, y0 - y, color);
        LCD_drawPixel(x0 - x, y0 - y, color);
        LCD_drawPixel(x0 + y, y0 + x, color);
        LCD_drawPixel(x0 - y, y0 + x, color);
        LCD_drawPixel(x0 + y, y0 - x, color);
        LCD_drawPixel(x0 - y, y0 - x, color);
        
        if(d < 0){
            d = d + 4 * x + 6;
        } else{
            d = d + 4 * (x - y) + 10;
            y--;
        }
    }
    
    
    
}


/**************************************************************************//**
* @fn			void LCD_drawLine(short x0,short y0,short x1,short y1,uint16_t c)
* @brief		Draw a line from and to a point with a color
* @note
*****************************************************************************/
void LCD_drawLine(short x0,short y0,short x1,short y1,uint16_t c)
{
    //based on the Bresenham's theorem code from wikipedia
    
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;

    int error = dx - dy;
    
    while(1){
        LCD_setAddr(x0,y0,x0,y0);
        SPI_ControllerTx_16bit(c);
        
        if(x0 == x1 && y0 == y1) break;
 
        int e2 = 2 * error;
        if(e2 >= -dy){
            error -= dy;
            x0 += sx;
        }else if(e2 <= dx){
            error += dx;
            y0 += sy;
        }
        
    }
    

}



/**************************************************************************//**
* @fn			void LCD_drawBlock(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,uint16_t color)
* @brief		Draw a colored block at coordinates
* @note
*****************************************************************************/
void LCD_drawBlock(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,uint16_t color)
{
    /*LCD_setAddr(x0, y0, x1, y1);  // Set drawing area correctly

    for (int i = x0; i <= x1; i++) {  // Correct loop bounds
        for (int j = y0; j <= y1; j++) {
            LCD_drawPixel(i, j, color);  // Use drawPixel instead of redundant setAddr calls
        }
    } //too slow ):*/
    
    LCD_setAddr(x0, y0, x1-1, y1-1);
    uint32_t pxCount = abs(x1-x0) * abs(y1-y0);
    
    for(uint32_t i = 0; i < pxCount; i++){
        SPI_ControllerTx_16bit(color);
    }
    
    
}

/**************************************************************************//**
* @fn			void LCD_setScreen(uint16_t color)
* @brief		Draw the entire screen to a color
* @note
*****************************************************************************/
void LCD_setScreen(uint16_t color) 
{
	LCD_setAddr(0,0,LCD_WIDTH, LCD_HEIGHT);
    for(int i = 0; i < LCD_WIDTH; i++){
        for(int j = 0; j < LCD_HEIGHT; j++){
            SPI_ControllerTx_16bit(color);
        }
    }

}

/**************************************************************************//**
* @fn			void LCD_drawString(uint8_t x, uint8_t y, char* str, uint16_t fg, uint16_t bg)
* @brief		Draw a string starting at the point with foreground and background colors
* @note
*****************************************************************************/
void LCD_drawString(uint8_t x, uint8_t y, char* str, uint16_t fg, uint16_t bg, uint8_t size)
{
    int i = 0;
    int space = 6 * size;  // update spacing
    while(str[i] != '\0') {
        LCD_drawChar(x + (i * space), y, str[i], fg, bg, size);
        i++;
    }
}