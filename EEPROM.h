/* 
 * File:   EEPROM.c
 * Author: ryan
 *
 * Created on 29 januari 2019, 18:30
 */
/*
 * 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef __EEPROM_H__
#define	__EEPROM_H__

#include <xc.h> // include processor files - each processor file is guarded.  

uint8_t ReadEEByte(uint8_t);
void    WriteEEByte(uint8_t, uint8_t);
#endif