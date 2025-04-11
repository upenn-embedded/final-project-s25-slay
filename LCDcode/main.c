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

#define MAX_STITCHES    5
#define MAX_ROWS        5

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

bool isDigit(char c) {
    return (c >= '0' && c <= '9');
}

void parseText(char* buffer){
    
    // : means round and , means stitch
    if (buffer[0] != 'R') return;

    char* token = strtok(buffer, ":");
    if (token == NULL) return;

    rows[rowIndex].r = atoi(token + 1);

    token = strtok(NULL, ",");
    int stitchIndex = 0;

    while (token != NULL && stitchIndex < MAX_STITCHES) {
        // Remove leading spaces
        while (*token == ' ') token++;

        Stitch *s = &rows[rowIndex].stitches[stitchIndex];
        
        // Check if token starts with digit (e.g., "5sc")
        if (isDigit(token[0])) {
            s->n = atoi(token);
            // Copy stitch type (after the number)
            int len = 0;
            while (isDigit(token[len])) len++;
            strncpy(s->type, token + len, sizeof(s->type) - 1);
            s->type[sizeof(s->type) - 1] = '\0';  // Null terminate
        } else {
            // no number-> default to 1
            s->n = 1;
            strncpy(s->type, token, sizeof(s->type) - 1);
            s->type[sizeof(s->type) - 1] = '\0';
        }

        stitchIndex++;
        token = strtok(NULL, ",");
    }

    rows[rowIndex].stitchCount = stitchIndex;
    rowIndex++;
}

void updateText(Row *r){
    //each time stitch updates, move LCD
    
    //check if current stitch # = 1
    if(r->stitches[0].n == 1){
    //if 1, check if at end of row or not --> stitchCount
        if(r->stitchCount == 1){                     //check end of row
            r->r++;                                 //move to next row  
            currentRow++;
            //also buzz haptic
            return;
        }else{
            //go to next stitch
            // remove first stitch by shifting remaining stitches
            for (int i = 0; i < r->stitchCount - 1; i++) {
                r->stitches[i] = r->stitches[i + 1];
            }
            r->stitchCount--;  // Reduce stitch count
        }
        
    }else{
        r->stitches[0].n--;
    }
    //if not 1, decrement stitch count by 1
    
    //redraw LCD here :D
    LCD_setScreen(WHITE);

    char line[32];
    sprintf(line, "R%d: ", r->r);
    LCD_drawString(10, 10, line, BLACK, WHITE);

    int y = 20;
    for (int i = 0; i < r->stitchCount; i++) {
        sprintf(line, "%s %d", r->stitches[i].type, r->stitches[i].n);
        LCD_drawString(10, y, line, BLACK, WHITE);
        y += 10;
    }
    
}

void main(void) {
    
    uart_init();
    lcd_init();
    
    
    /*char buffer[20];  // Buffer to store one line at a time
    uint16_t i = 0, j = 0;
    char c;
    
    LCD_setScreen(WHITE);
    
    while ((c = pgm_read_byte(&testR1[i])) != '\0') {  // Read from Flash
        if (c == '\n' || j >= sizeof(buffer) - 1) {
            buffer[j] = '\0';  // Null-terminate the line
            
            printf("%s\n", buffer);  // Corrected printf syntax
            parseText(buffer);      // <--- Add this line!

            j = 0;
        } else {
            buffer[j++] = c;
        }
        i++;
    }

    while (1) {
        _delay_ms(2000);  // Simulate 1 stitch per second
        if (currentRow < rowIndex) {
            updateText(&rows[currentRow]);
        } else {
            LCD_drawString(10, 100, "All rows done!", RED, WHITE);
        }
    }*/
    
    while(1){
        LCD_setScreen(BLUE);
    }
}
