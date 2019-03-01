// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef SCREEN
#define	SCREEN

#include <xc.h>
void set_screen(volatile uint16_t *newData);
bool choosescreen(void);
void pauseMultiplexing(void);
void resumeMultiplexing(void);
void screen_update(void);
void initialise_screen(void);
#endif