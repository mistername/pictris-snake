/* 
 * File:   EEPROM.c
 * Author: ryan
 *
 * Created on 29 januari 2019, 18:25
 */
/*
 * 
 */

#include <xc.h>
#include <stdint.h>
uint8_t ReadEEByte(uint8_t address)
{
EEADR=address; // load address of EEPROM to read
EECON1bits.EEPGD=0; // access EEPROM data memory
EECON1bits.CFGS=0; // do not access configuration registers
EECON1bits.RD=1; // initiate read
return EEDATA; // return EEPROM byte
}

// Write Byte to internal EEPROM
void WriteEEByte(uint8_t address, uint8_t data)
{
EECON1bits.WREN=1; // allow EEPROM writes
EEADR=address; // load address of write to EEPROM
EEDATA=data; // load data to write to EEPROM
EECON1bits.EEPGD=0;// access EEPROM data memory
EECON1bits.CFGS=0; // do not access configuration registers
INTCONbits.GIE=0; // disable interrupts for critical EEPROM write sequence
//===============//
EECON2=0x55;
EECON2=0xAA;
EECON1bits.WR=1;
//==============//
INTCONbits.GIE=1; // enable interrupts, critical sequence complete
while (EECON1bits.WR==1); // wait for previous write to complete
EECON1bits.WREN=0; // do not allow EEPROM writes
}

