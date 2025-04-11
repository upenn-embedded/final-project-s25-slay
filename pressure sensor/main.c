/* 
 * File:   main.c
 * Author: hansikadorai
 *
 * Created on April 10, 2025, 1:44 PM
 */

#define F_CPU               16000000UL
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include "uart.h"
#include <util/delay.h>
#include <avr/interrupt.h>

#define TAP_THRESHOLD 250
#define DOUBLE_TAP_WINDOW 500  // ms

uint8_t waiting = 0;
uint8_t doubleTapCount = 0;
uint16_t adc_value;
int tapDetected = 0;



void Initialize () {
    
    TCCR1A = 0;
    TCCR1B = (1 << WGM12);
    
            
    OCR1A = 43749;
    TIMSK1 |= (1 << OCIE1A);
    
    ADMUX = (1 << REFS0); // Vref = AVcc (5V), ADC0 selected
    ADCSRA = (1 << ADEN)  // Enable ADC
           | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler = 128
}


uint16_t ADC_read(void) {
    ADCSRA |= (1 << ADSC); // Start conversion
    while (ADCSRA & (1 << ADSC)); // Wait for conversion to complete
    return ADC;
}

void startTimer(void) {
    TCNT1 = 0;
    TCCR1B |= (1 << CS12); // Prescaler 256
    TCCR1B &= ~((1 << CS10) | (1 << CS11));
    printf("timer starting????");
}

void stopTimer(void) {
    TCCR1B &= ~((1 << CS10) | (1 << CS11) | (1 << CS12));
}


ISR(TIMER1_COMPA_vect) {
    waiting = 0;
    stopTimer();
    printf("too late");
}

void handleTaps(void) {
    adc_value = ADC_read();
    
    if ((adc_value > 250) && (tapDetected == 0)){
        tapDetected = 1;
        
        if (waiting == 0) {
            printf("single");
            waiting = 1;
            _delay_ms(100);
            startTimer();
        } else {
            printf("double");
            doubleTapCount++;
            waiting = 0;
            stopTimer();
            _delay_ms(500);
        }
        
    }
    
    if (adc_value < TAP_THRESHOLD - 100) {
        tapDetected = 0;
    }
            
}


int main(void) {
    uart_init();
    Initialize();
    sei();
    
    
    
    
    while (1) {
        handleTaps();
        _delay_ms(50);
        
       
        printf("num incr: %u \n", doubleTapCount);
    }
   
}




