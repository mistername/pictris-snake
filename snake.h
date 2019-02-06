// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef SNAKE_HEADER
#define	SNAKE_HEADER

#include <xc.h> // include processor files - each processor file is guarded.  

void snake_button_left(void);
void snake_button_up(void);
void snake_button_right(void);
void snake_button_down(void);
void snake_screen(void);
uint8_t genBerry(uint8_t);
void CreateBerry(void);
void SnakeGraph(void);
void MoveSnakes();
bool snake_timer(void);
void inistialize_snake(void);
void snake_main(void);
#endif	