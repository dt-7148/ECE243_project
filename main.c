#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#define PLAYER_VELO 5			// player velocity constant
#define BULLET_VELO 20
#define X_BOUND 319				// screen x bound
#define Y_BOUND 239				// screen y bound
#define NUM_BULLETS 50
#define FIRE_RATE 0			// How many frames between an enemy shooting a bullet, lower = faster firing

void clear_screen();
void plot_pixel(int, int, short int);
void waitForVsync();
void draw();
void update();
void handle_input();
void update_player();
void update_bullet();
void init_bullets();
void reset_bullet(int i);

int pixel_buffer_start; // global variable
int back_buffer;
int playerX;			// Player X position
int playerY;			// Player Y position
int player_velo_x;		// Player directional velocity - will change between +/- VELO and 0 depending on user input
int player_velo_y;		
int E0_FLAG;			// Flag for when E0 bit is received
int F0_FLAG;			// Flag for when F0 bit is received
int numBulletsActive;	
int curBullet;			// Current bullet in bullet array
int frameCounter;		// frame counter for bullet firing delay 

typedef struct bullet {
	double x;
	double y;
	double velo_x;
	double velo_y;
	int isActive;
} Bullet;

Bullet enemyBullets[NUM_BULLETS];
Bullet playerBullets[NUM_BULLETS];

/*
	to add later: bullet pool, enemy pool
*/

int main(void)
{

	playerX = playerY = 0;
	init_bullets();
	numBulletsActive = 0;
	curBullet = 0;
	frameCounter = 0;
	
    volatile int *pixel_ctrl_ptr = (int *)0xFF203020;
    /* Read location of the pixel buffer from the pixel buffer controller */
    pixel_buffer_start = *pixel_ctrl_ptr;
	
	*(pixel_ctrl_ptr + 1) = 0x200000; // init back buffer to something else to enable double buffering
	back_buffer = *(pixel_ctrl_ptr + 1);
	
	while (1) {
		update();
		draw();
		waitForVsync();
		back_buffer = *(pixel_ctrl_ptr + 1);
	}

}

void init_bullets() {
	for (int i = 0; i < NUM_BULLETS; i++) {
		reset_bullet(i);
	}
}

void reset_bullet(int i) {
	enemyBullets[i].x = 100;
	enemyBullets[i].y = 100;
	enemyBullets[i].velo_x = 0;
	enemyBullets[i].velo_y = 0;
	enemyBullets[i].isActive = 0;
}

void handle_input() {
    unsigned char byte1 = 0;
	unsigned char byte2 = 0;
	unsigned char byte3 = 0;

    volatile int * PS2_ptr = (int *) 0xFF200100;  // PS/2 port address

	int PS2_data, RVALID;

	PS2_data = *(PS2_ptr);	// read the Data register in the PS/2 port
	RVALID = (PS2_data & 0x8000);	// extract the RVALID field
	if (RVALID != 0)
	{
		/* always save the last three bytes received */
		byte1 = byte2;
		byte2 = byte3;
		byte3 = PS2_data & 0xFF;  // byte3 is the data bit - either key scan code or release
	}
	
	/*
		to handle holding down multiple keys, only consider it stopped if the key is released
	*/
	switch (byte3) {
		// ========================================== Player movement ==========================================
		case 0x1D:							// W key
			if (F0_FLAG) {
				F0_FLAG = 0;
				player_velo_y = 0;
			}
			else {
				player_velo_y = -1 * PLAYER_VELO;	
			}
			break;
		case 0x1C:							// A key
			if (F0_FLAG) {
				F0_FLAG = 0;
				player_velo_x = 0;
			}
			else {
				player_velo_x = -1 * PLAYER_VELO;	
			}
			break;
		case 0x1B:							// S key
			if (F0_FLAG) {
				F0_FLAG = 0;
				player_velo_y = 0;
			}
			else {
				player_velo_y = PLAYER_VELO;	
			}		
			break;
		case 0x23:							// D key
			if (F0_FLAG) {
				F0_FLAG = 0;
				player_velo_x = 0;
			}
			else {
				player_velo_x = PLAYER_VELO;	
			}
			break;
			
		case 0xF0:
			F0_FLAG = 1;
			break;
			
		default:
			break;
	}
}

void update_player() {
	// Update player position - only update if they are not at an edge
	// later: do bound check with all terrain elements
   	playerX += player_velo_x;
	playerY += player_velo_y;
	
	if (playerX <= 0)
		playerX = 0;
	else if (playerX >= X_BOUND) // Later: Add width of sprite to playerX
		playerX = X_BOUND;
    	    
    if (playerY <= 0)
		playerY = 0;
	else if (playerY >= Y_BOUND) // Later: Add height of sprite to playerY 
        playerY = Y_BOUND;
	
}


double squareRoot(double n, float l)
{
    // Assuming the sqrt of n as n only
    double x = n;
 
    // The closed guess will be stored in the root
    double root;
 
    // To count the number of iterations
    int count = 0;
 
    while (1) {
        count++;
 
        // Calculate more closed x
        root = 0.5 * (x + (n / x));
 
        // Check for closeness
        if (abs(root - x) < l)
            break;
 
        // Update root
        x = root;
    }
 
    return root;
}



void update_bullet() {
	if (frameCounter == FIRE_RATE) {
		// fire bullet - ie. get velocity, set isActive = 1;
		float deltaX = playerX - enemyBullets[curBullet].x;
		float deltaY = playerY - enemyBullets[curBullet].y;
		float hyp = squareRoot( pow(deltaX, 2) + pow(deltaY, 2), 0.0001);
		enemyBullets[curBullet].velo_x = BULLET_VELO * (deltaX / hyp);
		enemyBullets[curBullet].velo_y = BULLET_VELO * (deltaY / hyp);
		enemyBullets[curBullet].isActive = 1;
		numBulletsActive = numBulletsActive + 1;
		curBullet = curBullet + 1;

		if (curBullet >= NUM_BULLETS) curBullet = 0;
		
		frameCounter = 0;
	}
	else {
		frameCounter = frameCounter + 1;	
	}
	
	for (int i = 0; i < NUM_BULLETS; i++) {
		if (enemyBullets[i].isActive) {
			// update movement
			// check if offscreen
			enemyBullets[i].x = enemyBullets[i].x + enemyBullets[i].velo_x;
			enemyBullets[i].y = enemyBullets[i].y + enemyBullets[i].velo_y;
			
			if (enemyBullets[i].x >= X_BOUND || enemyBullets[i].x <= 0 || enemyBullets[i].y >= Y_BOUND || enemyBullets[i].y <= 0) {
				reset_bullet(i);
				numBulletsActive = numBulletsActive - 1;
			}
			
			
			
		}
	}
}

void update() {
	/*
		read a key input - done, handle_input
			
		update player position
		run player checks
			collision with walls, terrain, exits, bullets
		
		later:
			update bullet position
			check bullet collision
			update enemy health and whatever
			
			change update depending on game state
	*/
	
	handle_input();		//later: have a separate input handler for menus
	update_player();
	
	update_bullet();
	
	
}

void draw() {
	clear_screen();
	
	/*
		depending on state, menu or game, draw something different
	*/
	
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 10; j++) {
			plot_pixel(playerX + i, playerY + j, 0xffff);	
		}
	}
	
	for (int k = 0; k < NUM_BULLETS; k++) {
		for (int i = 0; i < 10; i++) {
			for (int j = 0; j < 10; j++) {
				plot_pixel(enemyBullets[k].x + i, enemyBullets[k].y + j, 0xffff);	
			}
		}
	}
}

void waitForVsync() {
	volatile int *pixel_ctrl_ptr = (int *)0xFF203020;
	int status;
	*pixel_ctrl_ptr = 1;
	status = *(pixel_ctrl_ptr + 3);
	
	while ((status & 0x01) != 0) {
		status = *(pixel_ctrl_ptr + 3);	
	}
}


void plot_pixel(int x, int y, short int line_color)
{
     short int *one_pixel_address;

        one_pixel_address = back_buffer + (y << 10) + (x << 1);

        *one_pixel_address = line_color;

}

void clear_screen() {
	for (int x = 0; x < 320; x++) {
		for (int y = 0; y < 240; y++) {
			plot_pixel(x, y, 0);	
		}
	}
}
