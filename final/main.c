#define F_CPU 16000000UL

#include <xc.h>
#include <stdio.h>
#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdbool.h>
#include <util/delay.h> 
#include "ST7735.h"
#include "LCD_GFX.h"
#include <string.h>

//for loading "text" file
#include <avr/pgmspace.h>
extern const char testR1[];                         //pull testR1

#define MAX_STITCHES    20
#define MAX_ROWS        50

typedef struct{
    char type[10];                                  //type of stich
    int n;                                          //number of stitches
} Stitch;

typedef struct{
    int r;                                          //row #
    Stitch stitches[MAX_STITCHES];                  //array of stitches
    int stitchCount;                                //current stitch section
} Row;

Row rows[MAX_ROWS];
int rowIndex = 0;                                   //to parse testR1

int currentRow = 0;                                 //for updating rows

void parseText(char* buffer){
    
    // : means round and , means stitch
    if (buffer[0] != 'R') return; // Ensure line starts with "R"

    char* token = strtok(buffer, ":");  // Get "Rx"
    if (token == NULL){ 
        printf("NO TOKEN ):");
        return;
    }
    

    rows[rowIndex].r = atoi(token + 1);  // get # after R

    token = strtok(NULL, ",");  // Get first stitch
    int stitchIndex = 0;

    while (token != NULL && stitchIndex < MAX_STITCHES) {
        sscanf(token, "%s %d", rows[rowIndex].stitches[stitchIndex].type, 
               &rows[rowIndex].stitches[stitchIndex].n);
        stitchIndex++;
        token = strtok(NULL, ",");  // Move to next stitch
    }

    rows[rowIndex].stitchCount = stitchIndex;
    rowIndex++;  // Move to next row
}

/*void updateText(Row r){
    //each time stitch updates, move LCD
    
    //check if current stitch # = 1
    if(r.stitches[0]->n == 1){
    //if 1, check if at end of row or not --> stitchCount
        if(r.stitchCount == 1){                     //check end of row
            r->r++;                                 //move to next row
            //r->stitchCount = 0;                     //reset stitches
            //also buzz haptic
        }else{
            //go to next stitch
            // remove first stitch by shifting remaining stitches
            for (int i = 0; i < r->stitchCount - 1; i++) {
                r->stitches[i] = r->stitches[i + 1];
            }
            r->stitchCount--;  // Reduce stitch count
        }
        
    }else{
        r.stitches[0]->n--;
    }
    //if not 1, decrement stitch count by 1
    
        
    
    //redraw LCD here :D
    
}*/

void main(void) {
    
    uart_init();
    lcd_init();
    
    
    char buffer[20];  // Buffer to store one line at a time
    uint16_t i = 0, j = 0;
    char c;
    
    
    while ((c = pgm_read_byte(&testR1[i])) != '\0') {  // Read from Flash
        if (c == '\n' || j >= sizeof(buffer) - 1) {
            buffer[j] = '\0';  // Null-terminate the line
            
            printf("%s\n", buffer);  // Corrected printf syntax
            j = 0;
        } else {
            buffer[j++] = c;
        }
        i++;
    }
    
    while(1){
        //LCD_setScreen(WHITE);

        LCD_drawString(10, LCD_HEIGHT - (j * 10), buffer, BLACK, WHITE);
    }


}
