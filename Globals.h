#ifndef _GLOBALS_H
#define _GLOBALS_H

#include <stdbool.h>
#include <stdlib.h>
#include <cmsis_os2.h>

#define RIGHT   0
#define LEFT    1
#define UP			2
#define DOWN		3
#define WALLCOLLIDE		1
#define ZOMBCOLLIDE		3
#define PLAYCOLLIDE		2

struct zombieInitilizer{
	uint16_t x,y;
	uint8_t type;
};

struct sprite {
	uint16_t x, y, oldx, oldy;
	uint8_t health, ammo, size, dir, speed;
	bool isActive, isDeleted;
	unsigned char * bitmap;
};

#endif /* _GLOBALS_H */
