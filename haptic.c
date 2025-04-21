#define F_CPU               16000000UL
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include "uart.h"
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/twi.h>
#define HAPTIC  0x5A
#define SCL_CLOCK 100000L


void I2C_init(void) {
    TWSR0 = 0x00;
    TWBR0 = ((F_CPU/SCL_CLOCK) - 16) / 2; //baud rate
    TWCR0 = (1 << TWEN);    // set two wire interface
    DDRC |= (1 << PC4); //outp
    DDRC |= (1 << PC5);
}

void I2C_start(void) {
    TWCR0 = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT); //send start condition
    while (!(TWCR0 & (1 << TWINT))); //waits for flag set
}

void I2C_stop(void) {
    TWCR0 = (1 << TWSTO) | (1 << TWINT) | (1 << TWEN);   //send stop condition
    while (TWCR0 & (1 << TWSTO));
}

void I2C_write(uint8_t data) {
    TWDR0 = data; 
    TWCR0 = (1 << TWEN) | (1 << TWINT); // start transmission of address
    while (!(TWCR0 & (1 << TWINT))); // wait for twint flag set, shows sla_w is transmitted
}

void I2C_writeRegister(uint8_t address, uint8_t reg, uint8_t data) {
    I2C_start();
    I2C_write((address << 1) | 0); // Write mode to slave addy
    I2C_write(reg);  //send reg addy
    I2C_write(data);  //send data byte and wait for ak/nack
    I2C_stop();
}

void DRV2605_init(void) {
    I2C_writeRegister(HAPTIC, 0x01, 0x00); // Mode: Internal trigger
    I2C_writeRegister(HAPTIC, 0x03, 0x01); // Library: 1 (LRA, ERM, etc.)
    I2C_writeRegister(HAPTIC, 0x04, 0x03); // Effect: Strong click (ID = 3)
    I2C_writeRegister(HAPTIC, 0x0C, 0x01); // GO: start motor
}

int main(void) {
    I2C_init();
    _delay_ms(100);   
    DRV2605_init();

    while (1) {
        _delay_ms(1000);
        I2C_writeRegister(HAPTIC, 0x04, 0x2F); // add soft bump 60% buzz to wave list
        I2C_writeRegister(HAPTIC, 0x0C, 0x01); // start wave list
    }
}
