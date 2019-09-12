// Zombie Slayer Final Project
// Authors: Evan Kim (em2kim), Fahim Shahriar (fshahria)
#include <lpc17xx.h>
#include <stdio.h>
#include <cmsis_os2.h>
#include "uart.h"
#include "glcd.h"
#include "globals.h"
#include "Assets.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Game Globals
static struct sprite myPlayer;
static bool inGame = false;
static struct sprite bullets[4];
static struct sprite zombies[10];
static uint32_t playerImmunity, zombiesLeft, round;

// Push button detection
uint32_t pushButton() { 
	uint32_t val = 0;
	val = LPC_GPIO2->FIOPIN;
	val = val >> 10;
	val = !(val & 0x1);
	return val;
}

// Handles Greeting, will set the started bool to true when the button is pressed
// Displays Current round and progression
void roundScreen() {
	uint8_t val = 0;
	GLCD_Clear(Black);
	char *roundInfo = (unsigned char*)malloc(10 * sizeof(char));
	sprintf(roundInfo, "Round %d/9", round+1);
	//GLCD_DisplayString(2,5, 1, "Welcome To");
	GLCD_DisplayString(2,0, 1, "Shootout to Get Out!");
	GLCD_DisplayString(3,5, 1, roundInfo);
	GLCD_DisplayString(15,9, 0, "Press the PUSH BUTTON to spawn...");
	do{
		val = pushButton();
	}while(val == 0);
}


void diedScreen() {
	uint8_t val = 0;
	GLCD_Clear(Black);
	char *roundInfo = (unsigned char*)malloc(10 * sizeof(char));
	sprintf(roundInfo, "Round %d/9", round+1);
	GLCD_DisplayString(2,5, 1, "YOU Have DIED!");
	GLCD_DisplayString(3,5, 1, roundInfo);
	GLCD_DisplayString(15,9, 0, "Press the PUSH BUTTON to respawn...");
	do{
		val = pushButton();
	}while(val == 0);
}

void congratulationsScreen() {
	uint8_t val = 0;
	GLCD_Clear(Black);
	GLCD_DisplayString(2,0, 1, "Congratulations");
	GLCD_DisplayString(2,0, 1, "YoU lIVe to sEe AnOTher DaY");
	GLCD_DisplayString(15,9, 0, "Press the PUSH BUTTON to restart...");
	do{
		val = pushButton();
	}while(val == 0);
}

// Method returns position of the joystick to move the character or fire the weapon
uint32_t GPIO1GetValue (uint32_t bitPosi)
{
    uint32_t val;
    val = LPC_GPIO1->FIOPIN;
    val = val >> bitPosi;
		val = val & 0x1;
    return !val;
}


// Initializer for the player, zombie and bullets sprite structs
void resetGame(){
	GLCD_DrawMap(map[round/3]);
	playerImmunity = osKernelGetTickCount();
	uint8_t i;
	myPlayer.x = 20;
	myPlayer.y = 20;
	myPlayer.bitmap = (unsigned char *)PlayerBitmap;
	myPlayer.size =  20;
	myPlayer.oldx = 20;
	myPlayer.oldy = 20;
	myPlayer.ammo = 16;
	myPlayer.dir = RIGHT;
	myPlayer.speed = 1;
	myPlayer.health = 5;
	
	for (i = 0; i < 4; i++) {
		bullets[i].bitmap = (unsigned char *)BulletBitmap;
		bullets[i].size = 5;
		bullets[i].speed = 1; 
		bullets[i].isActive = false;
		bullets[i].isDeleted = false;
	}
	zombiesLeft = 0;
	for(i = 0; i < 10; i++){
		uint8_t type = zombieInit[round][i].type;
		if(type == 0)
			zombies[i].isActive = false;
		else{
			memcpy(&zombies[i], &zombieTemplates[type-1], sizeof(struct sprite));
			zombies[i].x = zombieInit[round][i].x;
			zombies[i].oldx = zombieInit[round][i].x;
			zombies[i].y = zombieInit[round][i].y;
			zombies[i].oldy = zombieInit[round][i].y;
			zombiesLeft++;
		}
	}
}

// The game thread reads the sprite structs and redraws the sprites 
void gameThread(void *arg) {
	uint8_t i;
	bool finishedRound;
	while(1){
		for(round = 0; round < 9; round ++){
			roundScreen();
			resetGame();
			inGame = true;
			while(inGame){
				GLCD_DisplayString(0,0,0,"Lives:");
				for (uint8_t hIndex = 0; hIndex < 6; hIndex++) {
					if(hIndex <= myPlayer.health)
						GLCD_DisplayString(0, 7+hIndex,0, "O");
					else
						GLCD_DisplayString(0, 7+hIndex,0, " ");
				}
				if(myPlayer.health <= 0){
					diedScreen();
					resetGame();
				}
				if(zombiesLeft <= 0){
					break;
				}
				
				GLCD_DrawSprite(&myPlayer);
				// Update the bullet position until it hits a wall or a zombie
				for (i = 0; i < 10; i++) {
					if (i < 4) {
						if(bullets[i].isActive){
							GLCD_DrawSprite(&bullets[i]);
						}
						else if(bullets[i].isDeleted){
							bullets[i].isDeleted = false;
							GLCD_DeleteSprite(&bullets[i]);
						}
					}
					if (zombies[i].isActive) {
						GLCD_DrawSprite(&zombies[i]);
					}
					else if(zombies[i].isDeleted){
						zombies[i].isDeleted = false;
						GLCD_DeleteSprite(&zombies[i]);
						zombiesLeft--;
					}
				}
			}
		}
		// If the player does not want to play again
		congratulationsScreen();
	}
}

// collision bwtween sprite and wall
bool wallCollide(const struct sprite *s){
	uint16_t x, y;
	uint8_t i; 
	if(s->dir == RIGHT){
		x = s->x + s->speed + s->size;
		y = s->y;
		for(i = 0; i<3; i++){
			if(map[round/3][(y+((i*s->size)/2))/10][x/10])
				return true;
		}
	}
	else if(s->dir == LEFT){
		x = s->x - s->speed;
		y = s->y;
		for(i = 0; i<3; i++){
			if(map[round/3][(y+((i*s->size)/2))/10][x/10])
				return true;
		}
	}
	else if(s->dir == UP){
		x = s->x;
		y = s->y - s->speed;
		for(i = 0; i<3; i++){
			if(map[round/3][y/10][(x+((i*s->size)/2))/10])
				return true;
		}	
	}
	else{ //down
		x = s->x;
		y = s->y + s->speed + s->size;
		for(i = 0; i<3; i++){
			if(map[round/3][y/10][(x+((i*s->size)/2))/10])
				return true;
		}
	}
	return false;
}

// collision between two sprites
bool spriteCollide(const struct sprite *a, const struct sprite *b){
	uint16_t x, y;
	uint8_t i; 
	if(a->dir == RIGHT){
		x = a->x + a->speed;
		y = a->y;
		
	}
	else if(a->dir == LEFT){
		x = a->x - a->speed;
		y = a->y;
	}
	else if(a->dir == UP){
		x = a->x;
		y = a->y - a->speed;
	}
	else{ //down
		x = a->x;
		y = a->y + a->speed;
	}
	if (x < b->x + b->size &&
	x + a->size > b->x &&
	y < b->y + b->size &&
	y + a->size > b->y){
		return true;
	}
	return false;
}

uint8_t playerBulletCollsion(const struct sprite *s){
	uint8_t i;
	if(wallCollide(s))
		return WALLCOLLIDE;
	
	for (i = 0; i < 10; i++) {
		if (zombies[i].isActive) {
			if(spriteCollide(s, &zombies[i]))
				return ZOMBCOLLIDE+i;
		}
	}
	return false;
}

uint8_t zombieCollsion(uint8_t zombie){
	uint8_t i;
	if(wallCollide(&zombies[zombie]))
		return WALLCOLLIDE;
	if(spriteCollide(&zombies[zombie], &myPlayer)){
		if(osKernelGetTickCount() > playerImmunity){
			myPlayer.health --;
			playerImmunity = osKernelGetTickCount() + 1000;
		}
		return PLAYCOLLIDE;
	}
	for (i = 0; i < 10; i++) {
		if (zombies[i].isActive && i != zombie && myPlayer.health > 0) {
			if(spriteCollide(&zombies[zombie], &zombies[i]))
				return ZOMBCOLLIDE+i;
		}
	}
	return false;
}



// Handles Firing Weapon (Depressed center joystick as well as movement)
void characterThread(void *arg) {
	bool canFireAgain = true;
	uint32_t nextTime = 0, ledIndex1 = 28, ledIndex2 = 2;
	uint8_t bulletNum = 0, i, bulletState, pbVal = 0;

	while(1) {
		osDelay(5);
		uint32_t currTime = osKernelGetTickCount();
		// Reload the Weapon
		if (!inGame) {
			osThreadYield();
			continue;
		}
		pbVal = pushButton();
		if (pbVal != 0) {
			LPC_GPIO1->FIOSET |= 0xB0000000;
			LPC_GPIO2->FIOSET |= 0x0000007C;
			//myPlayer.ammo = 16;
			ledIndex1 = 28, ledIndex2 = 2;
			nextTime = currTime + 2000;
		}
		
		if (GPIO1GetValue(23)) {
			myPlayer.dir = UP;
			if(!playerBulletCollsion(&myPlayer)){
				myPlayer.y -= myPlayer.speed;
			}
		} else if (GPIO1GetValue(24)) {
			myPlayer.dir = RIGHT;
			if(!playerBulletCollsion(&myPlayer)){
				myPlayer.x += myPlayer.speed;
			}
		} else if (GPIO1GetValue(25)) {
			myPlayer.dir = DOWN;
			if(!playerBulletCollsion(&myPlayer)){
				myPlayer.y += myPlayer.speed;
			}
		} else if (GPIO1GetValue(26)) {
			myPlayer.dir = LEFT;
			if(!playerBulletCollsion(&myPlayer)){
				myPlayer.x -= myPlayer.speed;
			}
			
		} 
		
		else if (GPIO1GetValue(20) && nextTime < currTime && myPlayer.ammo > 0) {
			bullets[bulletNum].dir = myPlayer.dir;
			bullets[bulletNum].x = myPlayer.x + (myPlayer.size/2);
			bullets[bulletNum].y = myPlayer.y + (myPlayer.size/2);
			bullets[bulletNum].isActive = true;
			bullets[bulletNum].isDeleted = false;
			bulletNum ++;
			nextTime = currTime + 500;
			if (bulletNum == 4)
				bulletNum = 0;
			// Decrement LEDS every other shot
			if(myPlayer.ammo%2 == 1) {
				// Top 3 LEDS
				if(myPlayer.ammo >= 11){
					if (ledIndex1 == 30){ledIndex1++;}
					LPC_GPIO1->FIOCLR |= (1 << ledIndex1);
					ledIndex1++;
				// Bottom 5 LEDS
				} else {
					LPC_GPIO2->FIOCLR |= (1 << ledIndex2);
					ledIndex2++;
				}	
			}
			myPlayer.ammo--;
		}
		for (i = 0; i < 4; i++) {
			if (bullets[i].isActive) {
				bulletState = playerBulletCollsion(&bullets[i]);
				if(bulletState == WALLCOLLIDE){
					bullets[i].isActive = false;
					bullets[i].isDeleted = true;
				}
				else if(bulletState >= ZOMBCOLLIDE){
					zombies[bulletState-ZOMBCOLLIDE].health --;
					bullets[i].isActive = false;
					bullets[i].isDeleted = true;
				}
				else if(bullets[i].dir == UP) {
					bullets[i].y -= bullets[i].speed;
				} else if (bullets[i].dir == RIGHT) {
					bullets[i].x += bullets[i].speed;
				} else if (bullets[i].dir == DOWN) {
					bullets[i].y += bullets[i].speed;
				} else if (bullets[i].dir == LEFT) {
					bullets[i].x -= bullets[i].speed;
				}
			}
		}
	}
}


// Handles zombies following their paths
// RunGame will continuously redraw the zombies while the paths update here
// Zombies can only move one direction at a time (y movement or x movement between nodes)
void zombieThread(void *arg) {
	int16_t delX = 0, delY = 0;
	while(1) {
		osDelay(100);
		if (!inGame) {
			osThreadYield();
			continue;
		}
		
		for (uint8_t zi = 0; zi < 10; zi++) {
			if (zombies[zi].isActive) {
				delX = (int16_t)(myPlayer.x) - (int16_t)(zombies[zi].x);
				delY = (int16_t)(myPlayer.y) - (int16_t)(zombies[zi].y);
				if(zombies[zi].health <= 0){
					zombies[zi].isActive = false;
					zombies[zi].isDeleted = true;
				}
				else if (abs(delX) > abs(delY)) {
					// Move Right
					if (delX < 0) {
						zombies[zi].dir = LEFT;
						if(!zombieCollsion(zi)){
							zombies[zi].x -= zombies[zi].speed;
						}
					// Move Left
					} else {
						zombies[zi].dir = RIGHT;
						if(!zombieCollsion(zi)){
							zombies[zi].x += zombies[zi].speed;
						}
					}
				} else {
					// Move Up 
					if (delY < 0) {
						zombies[zi].dir = UP;
						if(!zombieCollsion(zi)){
							zombies[zi].y -= zombies[zi].speed;
						}
					// Move Down
					} else {
						zombies[zi].dir = DOWN;
						if(!zombieCollsion(zi)){
							zombies[zi].y += zombies[zi].speed;
						}
					}
				}
			}
		} 
	}
}


int main (void) {
	osKernelInitialize();
	
	GLCD_Init();
	osThreadNew(gameThread, NULL, NULL);
	osThreadNew(zombieThread,NULL,NULL);
	osThreadNew(characterThread, NULL, NULL);
	
	LPC_GPIO1->FIODIR |= 0xB0000000;
	LPC_GPIO2->FIODIR |= 0x0000007C;
	
	LPC_GPIO1->FIOSET |= 0xB0000000;
	LPC_GPIO2->FIOSET |= 0x0000007C;
	
	osKernelStart();
	
}
