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
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Bit manipulation
#define SBIT(reg,bit)	reg |=  (1<<bit)    // Macro defined for Setting  a bit of any register.
#define CBIT(reg,bit)	reg &= ~(1<<bit)    // Macro defined for Clearing a bit of any register.
#define GBIT(reg,bit)   reg &   (1<<bit)    // Macro defined for Getting  a bit of any register.
#define TBIT(reg,bit)   reg ^=  (1<<bit)    // Macro defined for Toggling a bit of any register.
#define VBIT(reg,bit,v) reg  = (reg & ~(1<<bit)) | ((v&1)<<bit)    // Macro defined for setting  a bit of any register to `v'

typedef enum {
    OVERRIDE,
    MERGE,
    INVERT
} mode_t;

typedef enum {
    DOWN,
    UP,
    LEFT,
    RIGHT
} direction_t;

typedef enum {
    CCW,
    CW
} rotation_t;

void set_mS(uint16_t);
uint16_t add_mS(uint16_t);
uint16_t get_mS(void);
void waitms(unsigned);
void *memcpy (void *__restrict, const void *__restrict, size_t);
volatile void *memcpyvol (volatile void *__restrict, const void *__restrict, size_t);
void swap(char*, char*);
void reverse(char str[], int);
char* itoa(int, char*, int);
void clearArray(volatile uint16_t *, size_t );
void mergeObjects(volatile uint16_t * , volatile uint16_t *, mode_t );
bool checkForLeftWall(volatile uint16_t * );
bool checkForRightWall(volatile uint16_t * );
bool collisionDetect(volatile uint16_t * , volatile uint16_t * );
uint8_t pixelCount(volatile uint16_t * );
void removeLine(volatile uint16_t * , uint8_t );
void getNumber(uint8_t , uint16_t * );
uint8_t readHighScore(uint8_t );
void writeHighScore(uint8_t , uint8_t , uint8_t );
void moveObject(uint16_t *, direction_t , uint8_t );
void show_score(uint8_t);
#endif