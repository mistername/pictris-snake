// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef BUTTONS
#define	BUTTONS

#include <xc.h> // include processor files - each processor file is guarded.  
bool checkLeft(bool);
bool checkRight(bool);
bool checkUp(bool);
bool checkDown(bool);

void start_button(void);
void pauseButtons(void);
void resumeButtons(void);
void debounceButton(volatile bool , volatile int16_t *, volatile bool *);
void checkButtons(void);
#endif	/* XC_HEADER_TEMPLATE_H */