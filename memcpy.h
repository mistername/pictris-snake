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
#ifndef __MEMCPY__
#define	__MEMCPY__

#include <xc.h> // include processor files - each processor file is guarded.  

void *memcpy (void *__restrict, const void *__restrict, size_t);
volatile void *memcpyvol (volatile void *__restrict, volatile const void *__restrict, size_t);
#endif