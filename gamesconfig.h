// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef GAMECONFIG
#define	GAMECONFIG

#include <xc.h> // include processor files - each processor file is guarded.

//game1
void tetris_timer(void);
void tetris_main(void);
#define GAME1MAIN tetris_main
#define GAME1TIME 800
#define GAME1TIMER tetris_timer
#define GAME1CHOOSESCREEN const uint16_t choosescreen1[] = {0x00FF,0x0081,0x0081,0x008D,0x0099,0x0081,0x0081,0x00FF,};



//game2
void snake_timer(void);
void snake_main(void);
#define GAME2MAIN snake_main
#define GAME2TIME 200
#define GAME2TIMER snake_timer
#define GAME2CHOOSESCREEN const uint16_t choosescreen2[] = {0xFF00,0x8100,0xBD00,0x8500,0x8500,0xAD00,0x8100,0xFF00,};


//general
#define LCM_TIME 800
#define CHOOSESCREEN uint16_t choosescreen[8]; {GAME1CHOOSESCREEN; GAME2CHOOSESCREEN; {uint8_t i; for(i=0;i<8;i++){choosescreen[i] = choosescreen1[i] | choosescreen2[i];}}}

#endif	/* GAMECONFIG */

