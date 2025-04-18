#define F_CPU 16000000UL

#include <xc.h>
#include <stdio.h>
#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdbool.h>
#include <util/delay.h> 
#include <string.h>


//LED
#define WORM PD5

//BUTTON
#define BUTTON_PIN PD7
#define BUTTON_PORT PORTD
#define BUTTON_PIN_PORT PIND
#define BUTTON_DDR DDRD

volatile int LEDState = 0;                //0-> off, 1->on, 2->resistor


// Pin Change Interrupt for button
ISR(PCINT2_vect) {
    
    static uint8_t prev_state = 1; // Start high due to pull-up
    uint8_t current_state = BUTTON_PIN_PORT & (1 << BUTTON_PIN);
    
    if (prev_state && !current_state) {
        // Detected falling edge (button press)
        //printf("button pressed\n");
        
        if(LEDState == 2){
            LEDState = 0;
            OCR0B = 0;
        } else {
            LEDState++;
            if(LEDState == 1){
                OCR0B = 255;
            }
        }
        
        printf("LED State: %d\n", LEDState);

    }
    

    prev_state = current_state; // Update for next comparison
}


// init ADC
void adc_init() {
    // Set ADC reference to AVcc (5V)
    ADMUX = (1 << REFS0);
    
    // Enable ADC, prescaler 128 --> 16MHz/128 = 125kHz
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    DDRB |= (1 << PB1);

    // Fast PWM mode, non-inverting, OCR0A as TOP
    TCCR0A = (1 << COM0B1) | (1 << WGM01) | (1 << WGM00);
    TCCR0B = (1 << WGM02) | (1 << CS01); // Prescaler = 8
    
    OCR0A = 255;  // 8-bit TOP --> ~7.8kHz PWM at 16MHz / 8
    OCR0B = 0; 
}


uint16_t adc_read(uint8_t channel) {
    // clear previous channel selection bits
    ADMUX &= 0xF0;
    
    // select new channel (PC0 = ADC0)
    ADMUX |= channel;
    
    // start conversion
    ADCSRA |= (1 << ADSC);
    
    // wait for conversion to complete
    while (ADCSRA & (1 << ADSC));
    
    return ADC;
}

// map ADC value to duty cycle (0% to 100% in 10 steps)
uint8_t map_adc(uint16_t adc_value) {
    const uint16_t adc_min = 200;
    const uint16_t adc_max = 1000;

    if (adc_value > adc_max) adc_value = adc_max;
    if (adc_value < adc_min) adc_value = adc_min;

    uint16_t scaled = adc_max - adc_value;
    uint16_t range = adc_max - adc_min;

    // Do all math in 16-bit and cast at the end
    uint16_t pwm_16bit = (scaled * 255UL) / range;

    return (uint8_t)pwm_16bit;
}

void init(){
    // Set LED pin (PD5/OC0B) as output
    DDRD |= (1 << WORM);
    
    // Set up button pin as input with pull-up
    BUTTON_DDR &= ~(1 << BUTTON_PIN);
    BUTTON_PORT |= (1 << BUTTON_PIN);
    
    // Enable pin change interrupt for the button (PCINT23 for PD7)
    PCICR |= (1 << PCIE2);   // Enable PCINT[23:16] interrupts
    PCMSK2 |= (1 << PCINT23); // Enable PCINT23 (PD7)
    
    // Clear any pending interrupt flags
    PCIFR = (1 << PCIF2);
    
    uart_init();
    adc_init();
    
    sei();  // Enable global interrupts

    
}

void main(void) {
    
    init();
    
    uint16_t adc_val;
   
    printf("teehee\r\n");
    
    
    OCR0B = 255;
   
    
    while(1){
        
        //check LED state- off / on handled in button interrupt 
        
        if(LEDState == 2){
            adc_val = adc_read(0);
            printf("adc val: %u\n", adc_val);
            //set the led here
            OCR0B = map_adc(adc_val);
            printf("Mapped: %u\n", map_adc(adc_val));
        }
    }

}