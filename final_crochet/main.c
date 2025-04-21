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


//************************ TOUCH SENSOR *************************************//
#define TAP_THRESHOLD 250
#define DOUBLE_TAP_WINDOW 500                       //ms

uint8_t waiting = 0;
uint8_t doubleTapCount = 0;
uint16_t adc_value;
int tapDetected = 0;

//**************************** HAPTIC ****************************************//
#define HAPTIC  0x5A
#define SCL_CLOCK 100000L

//************************ LCD STUFF ****************************************//
//for loading "text" file
#include <avr/pgmspace.h>
extern const char testR1[];                             //pull testR1

#define MAX_STITCHES    5
#define MAX_ROWS        5

typedef struct{
    char type[10];                                      //type of stich
    int n;                                              //number of stitches
} Stitch;

typedef struct{
    int r;                                              //row #
    Stitch stitches[MAX_STITCHES];                      //array of stitches
    int stitchCount;                                    //current stitch section
} Row;

Row rows[MAX_ROWS];
int rowIndex = 0;                                       //to parse testR1

int currentRow = 0;                                     //for updating rows


//**************************** I2C HANDLING *********************************//
void I2C_init(void) {
    TWSR0 = 0x00;
    TWBR0 = ((F_CPU/SCL_CLOCK) - 16) / 2;               //baud rate
    TWCR0 = (1 << TWEN);                                //set two wire interface
    DDRC |= (1 << PC4);
    DDRC |= (1 << PC5);
}

void I2C_start(void) {
    TWCR0 = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);  //send start condition
    while (!(TWCR0 & (1 << TWINT)));                    //waits for flag set
}

void I2C_stop(void) {
    TWCR0 = (1 << TWSTO) | (1 << TWINT) | (1 << TWEN);   //send stop condition
    while (TWCR0 & (1 << TWSTO));
}

void I2C_write(uint8_t data) {
    TWDR0 = data; 
    TWCR0 = (1 << TWEN) | (1 << TWINT);                 //start transmission of address
    while (!(TWCR0 & (1 << TWINT)));                    //wait for twint flag set, shows sla_w is transmitted
}

void I2C_writeRegister(uint8_t address, uint8_t reg, uint8_t data) {
    I2C_start();
    I2C_write((address << 1) | 0);                      //write mode to slave addy
    I2C_write(reg);                                     //send reg addy
    I2C_write(data);                                    //send data byte and wait for ack/nack
    I2C_stop();
}

void DRV2605_init(void) {
    I2C_writeRegister(HAPTIC, 0x01, 0x00);              // mode: internal trigger
    I2C_writeRegister(HAPTIC, 0x03, 0x01);              // library: 1 (LRA, ERM, etc.)
}

//**************************** LCD HANDLING **********************************//

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
        // remove leading spaces
        while (*token == ' ') token++;

        Stitch *s = &rows[rowIndex].stitches[stitchIndex];
        
        // check if token starts with digit (e.g., "5sc")
        if (isDigit(token[0])) {
            s->n = atoi(token);
            // copy stitch type (after the number)
            int len = 0;
            while (isDigit(token[len])) len++;
            strncpy(s->type, token + len, sizeof(s->type) - 1);
            s->type[sizeof(s->type) - 1] = '\0';
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
    
    ///check if current stitch # = 1
    if(r->stitches[0].n == 1){
        //if 1, check if at end of row or not --> stitchCount
        if(r->stitchCount == 1){                        //check end of row
            currentRow++;
            if (currentRow < rowIndex) {
                r = &rows[currentRow];
            } else {
                LCD_setScreen(WHITE);
                LCD_drawString(10, 100, "All rows done!", RED, WHITE, 1);
                return;
            }

            //buzz haptic
            I2C_writeRegister(HAPTIC, 0x04, 0x2F);
            I2C_writeRegister(HAPTIC, 0x0C, 0x01);
        } else {
            //go to next stitch
            //remove first stitch by shifting remaining stitches
            for (int i = 0; i < r->stitchCount - 1; i++) {
                r->stitches[i] = r->stitches[i + 1];
            }
            r->stitchCount--;
        }
    } else {
        r->stitches[0].n--;
    }

    //update LCD
    LCD_setScreen(WHITE);
    
    char line[32];
    sprintf(line, "Row%d: ", r->r);
    LCD_drawString(50, 50, line, BLACK, WHITE, 2);

    int y = 70;
    for (int i = 0; i < r->stitchCount; i++) {
        sprintf(line, "%s %d", r->stitches[i].type, r->stitches[i].n);
        LCD_drawString(50, y, line, CORNFLOWER, WHITE, 2);
        y += 20;
    }
    
}


//**************************DOUBLE TAP HANDLING ******************************//
uint16_t ADC_read(void) {
    ADCSRA |= (1 << ADSC);                          //start conversion
    while (ADCSRA & (1 << ADSC));                   //wait for conversion to complete
    return ADC;
}

void startTimer(void) {
    TCNT1 = 0;
    TCCR1B |= (1 << CS12);                          //prescaler 256
    TCCR1B &= ~((1 << CS10) | (1 << CS11));
    printf("timer starting\n");
}

void stopTimer(void) {
    TCCR1B &= ~((1 << CS10) | (1 << CS11) | (1 << CS12));
}

ISR(TIMER1_COMPA_vect) {
    waiting = 0;
    stopTimer();
    printf("time out\n");
}

void handleTaps(void) {
    adc_value = ADC_read();
    
    if ((adc_value > 250) && (tapDetected == 0)){
        tapDetected = 1;
        
        if (waiting == 0) {
            printf("single\n");
            waiting = 1;
            _delay_ms(100);
            startTimer();
        } else {
            printf("double\n");
            doubleTapCount++;
            if (currentRow < rowIndex) {
            updateText(&rows[currentRow]);
            } else {
                LCD_setScreen(WHITE);
                LCD_drawString(10, 100, "All rows done!", RED, WHITE, 1);
            }
            waiting = 0;
            stopTimer();
            _delay_ms(500);
        }
        
    }
    
    if (adc_value < TAP_THRESHOLD - 100) {
        tapDetected = 0;
    }
            
}

//****************************** MAIN ***************************************//

void init(){

    uart_init();
    lcd_init();
    
    
    //*************** TOUCH SENSOR ***************//
    TCCR1A = 0;
    TCCR1B = (1 << WGM12);
    
            
    OCR1A = 43749;
    TIMSK1 |= (1 << OCIE1A);
    
    ADMUX = (1 << REFS0);                                // Vref = AVcc (5V), ADC0 selected
    ADCSRA = (1 << ADEN)                                 //enable ADC
           | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); //prescaler = 128
    
    //*************** HAPTIC ***************//
    
    I2C_init();
    _delay_ms(100);   
    DRV2605_init();
    
    sei();
}

void main(void) {
    
    init(); 
    
    // ***************** LCD HANDLING *************************************//
    char buffer[20];  // Buffer to store one line at a time
    uint16_t i = 0, j = 0;
    char c;
    
    LCD_setScreen(WHITE);

    //parse the file for LCD reading 
    while ((c = pgm_read_byte(&testR1[i])) != '\0') {       //read from Flash
        if (c == '\n' || j >= sizeof(buffer) - 1) {
            buffer[j] = '\0';
            
            printf("%s\n", buffer);
            parseText(buffer);

            j = 0;
        } else {
            buffer[j++] = c;
        }
        i++;
    }
    
    updateText(&rows[currentRow]);

    while (1) {
        handleTaps();
        _delay_ms(50);
    }
}