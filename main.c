#include <stdbool.h>
#include <stdlib.h>
#include <math.h>




#define PLAYER_VELO 5           // player velocity constant
#define BULLET_VELO 10
#define PLAYER_BULLET_VELO 20
#define X_BOUND 319             // screen x bound
#define Y_BOUND 239             // screen y bound
#define NUM_BULLETS 100
#define FIRE_RATE 10            // How many frames between an enemy shooting a bullet, lower = faster firing
#define PLAYER_FIRE_RATE 10
#define NUM_ENEMIES 5          // How many enemies to spawn in a given room

#define PLAYER_WIDTH 8
#define PLAYER_BULLET_WIDTH 3
#define ENEMY_WIDTH 8
#define ENEMY_BULLET_WIDTH 3
#define WALL_WIDTH 8



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
void update_player_bullets();
double squareRoot(double n, float l);
void player_reset_bullet(int i);
void init_enemies();




int pixel_buffer_start; // global variable
int back_buffer;
int playerX;            // Player X position
int playerY;            // Player Y position
int player_velo_x;      // Player directional velocity - will change between +/- VELO and 0 depending on user input
int player_velo_y;      
int E0_FLAG;            // Flag for when E0 bit is received
int F0_FLAG;            // Flag for when F0 bit is received
int curBullet;          // Current bullet in bullet array
int player_curBullet;
int frameCounter_enemy;     // frame counter for bullet firing delay
int frameCounter_player;
int player_aim_h;       // Indicates whether the player is aiming horizontally/vertically using arrow keys
int player_aim_v;       // player will shoot a bullet only when one of these is non-zero

int isPlayerHit;


typedef struct entity {
    double x;
    double y;
    double velo_x;
    double velo_y;
    int isActive;
} Entity;




Entity enemyBullets[NUM_BULLETS];
Entity playerBullets[NUM_BULLETS];
Entity enemies[NUM_ENEMIES];








/*
    to add later: bullet pool, enemy pool
*/




int main(void)
{




    playerX = playerY = 2 * WALL_WIDTH + 1; // Start in top left corner for now
    init_bullets();
    init_enemies();
    curBullet = 0;
    player_curBullet = 0;
    frameCounter_enemy = 0;
    frameCounter_player = 0;
	isPlayerHit = 0;


    player_aim_h = player_aim_v = 0;
   
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
        player_reset_bullet(i);
    }
}




void init_enemies() {
    for (int i = 0; i < NUM_ENEMIES; i++) {
        enemies[i].x = (rand() % (X_BOUND - (2 * ENEMY_WIDTH) - (2 * WALL_WIDTH))) + (2 * WALL_WIDTH);           // Initialize positions to be a random point between the screen bounds
        enemies[i].y = (rand() % (Y_BOUND - (2 * ENEMY_WIDTH) - (2 * WALL_WIDTH))) + (2 * WALL_WIDTH);           // Later: factor in width of enemy
        enemies[i].velo_x = 0;
        enemies[i].velo_y = 0;
        enemies[i].isActive = 1;
    }
}




void reset_bullet(int i) {
    enemyBullets[i].x = 100;
    enemyBullets[i].y = 100;
    enemyBullets[i].velo_x = 0;
    enemyBullets[i].velo_y = 0;
    enemyBullets[i].isActive = 0;
}




void player_reset_bullet(int i) {
    playerBullets[i].x = 0;
    playerBullets[i].y = 0;
    playerBullets[i].velo_x = 0;
    playerBullets[i].velo_y = 0;
    playerBullets[i].isActive = 0;
}




void handle_input() {
    unsigned char byte1 = 0;
    unsigned char byte2 = 0;
    unsigned char byte3 = 0;




    volatile int * PS2_ptr = (int *) 0xFF200100;  // PS/2 port address




    int PS2_data, RVALID;




    PS2_data = *(PS2_ptr);  // read the Data register in the PS/2 port
    RVALID = (PS2_data & 0x8000);   // extract the RVALID field
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
        case 0x1D:                          // W key
            if (F0_FLAG) {
                F0_FLAG = 0;
                // Do not set to 0 if the opposite direction was pressed right after
                if (player_velo_y != 1 * PLAYER_VELO) {
                    player_velo_y = 0;
                }
            }
            else {
                player_velo_y = -1 * PLAYER_VELO;  
            }
            break;
        case 0x1C:                          // A key
            if (F0_FLAG) {
                F0_FLAG = 0;
                if (player_velo_x != 1 * PLAYER_VELO) {
                    player_velo_x = 0;
                }
            }
            else {
                player_velo_x = -1 * PLAYER_VELO;  
            }
            break;
        case 0x1B:                          // S key
            if (F0_FLAG) {
                F0_FLAG = 0;
                if (player_velo_y != -1 * PLAYER_VELO) {
                    player_velo_y = 0;
                }
            }
            else {
                player_velo_y = PLAYER_VELO;    
            }      
            break;
        case 0x23:                          // D key
            if (F0_FLAG) {
                F0_FLAG = 0;
                if (player_velo_x != -1 * PLAYER_VELO) {
                    player_velo_x = 0;
                }
            }
            else {
                player_velo_x = PLAYER_VELO;    
            }
            break;
           




        // ==================================================== Arrow keys ====================================================




        case 0x75:                          // Up arrow
            if (F0_FLAG) {
                // Arrow key released
                F0_FLAG = 0;
                E0_FLAG = 0;            // E0 flag will be up when key release, so need to disable it
                if (player_aim_v != 1) {
                    player_aim_v = 0;
                }
            }
            else if (E0_FLAG) {
                // Arrow key pressed
                E0_FLAG = 0;
                player_aim_v = -1;
				isPlayerHit = 0; 				// ********************************************************** Remove this line later, this is just for testing collisions **********************************************************
            }




            break;
        case 0x6B:                          // Left arrow
            if (F0_FLAG) {
                // Arrow key released
                F0_FLAG = 0;
                E0_FLAG = 0;
                if (player_aim_h != 1) {
                    player_aim_h = 0;
                }
            }
            else if (E0_FLAG) {
                // Arrow key pressed
                E0_FLAG = 0;
                player_aim_h = -1;
            }




            break;
        case 0x72:                          // Down arrow
            if (F0_FLAG) {
                // Arrow key released
                F0_FLAG = 0;
                E0_FLAG = 0;
                if (player_aim_v != -1) {
                    player_aim_v = 0;
                }
            }
            else if (E0_FLAG) {
                // Arrow key pressed
                E0_FLAG = 0;
                player_aim_v = 1;
            }




            break;
        case 0x74:                          // Right arrow
            if (F0_FLAG) {
                // Arrow key released
                F0_FLAG = 0;
                E0_FLAG = 0;
                if (player_aim_h != -1) {
                    player_aim_h = 0;
                }
            }
            else if (E0_FLAG) {
                // Arrow key pressed
                E0_FLAG = 0;
                player_aim_h = 1;
            }




            break;




        // =============================================== Special bytes ===============================================




        case 0xF0:
            F0_FLAG = 1;
            break;
        case 0xE0:
            E0_FLAG = 1;
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
   
    if (playerX <= (2 * WALL_WIDTH))
        playerX = (2 * WALL_WIDTH);
    else if (playerX + (2 * PLAYER_WIDTH - 1) >= X_BOUND - (2 * WALL_WIDTH)) // Later: Add width of sprite to playerX
        playerX = X_BOUND - (2 * WALL_WIDTH) - (2 * PLAYER_WIDTH - 1);
           
    if (playerY <= (2 * WALL_WIDTH))
        playerY = (2 * WALL_WIDTH);
    else if (playerY + (2 * PLAYER_WIDTH - 1) >= Y_BOUND - (2 * WALL_WIDTH)) // Later: Add height of sprite to playerY
        playerY = Y_BOUND -(2 * WALL_WIDTH) - (2 * PLAYER_WIDTH - 1);
   
}




void update_player_bullets() {
    if (frameCounter_player == PLAYER_FIRE_RATE) {
        // Fire bullet in aiming direction
        if (player_aim_h != 0) {
            float deltaX = (playerX + 5 * player_aim_h) - playerX;      // deltaX is between some point (5 pixels) in the aiming direction and the player
            float deltaY = 0;
            float hyp = squareRoot( pow(deltaX, 2) + pow(deltaY, 2), 0.0001);
            playerBullets[player_curBullet].x = playerX + PLAYER_WIDTH;
            playerBullets[player_curBullet].y = playerY + PLAYER_WIDTH;
            playerBullets[player_curBullet].velo_x = BULLET_VELO * (deltaX / hyp);
            playerBullets[player_curBullet].velo_y = BULLET_VELO * (deltaY / hyp);
            playerBullets[player_curBullet].isActive = 1;
       
            player_curBullet = player_curBullet + 1;




           if (player_curBullet >= NUM_BULLETS) player_curBullet = 0;
        }
        else if (player_aim_v != 0) {
            float deltaX = 0;
            float deltaY = (playerY + 5 * player_aim_v) - playerY;
            float hyp = squareRoot( pow(deltaX, 2) + pow(deltaY, 2), 0.0001);
            playerBullets[player_curBullet].x = playerX + PLAYER_WIDTH;                // create bullet at player location to fire
            playerBullets[player_curBullet].y = playerY + PLAYER_WIDTH;
            playerBullets[player_curBullet].velo_x = BULLET_VELO * (deltaX / hyp);
            playerBullets[player_curBullet].velo_y = BULLET_VELO * (deltaY / hyp);
            playerBullets[player_curBullet].isActive = 1;




            player_curBullet = player_curBullet + 1;




            if (player_curBullet >= NUM_BULLETS) player_curBullet = 0;
        }
       
        frameCounter_player = 0;
    }
    else {
        frameCounter_player = frameCounter_player + 1;
    }




    for (int i = 0; i < NUM_BULLETS; i++) {
        if (playerBullets[i].isActive) {
            // update movement
            // check if offscreen
            playerBullets[i].x = playerBullets[i].x + playerBullets[i].velo_x;
            playerBullets[i].y = playerBullets[i].y + playerBullets[i].velo_y;
           
            if (playerBullets[i].x >= X_BOUND || playerBullets[i].x <= 0 || playerBullets[i].y >= Y_BOUND || playerBullets[i].y <= 0) {
                player_reset_bullet(i);
            }
			
			for (int j = 0; j < NUM_ENEMIES; j++) {
				// Detect if player bullet hit enemy
				
				if (enemies[j].isActive) {
					if (playerBullets[i].x + (2 * PLAYER_BULLET_WIDTH) >= enemies[j].x && playerBullets[i].x < enemies[j].x + (2 * ENEMY_WIDTH)) {
						if (playerBullets[i].y + (2 * PLAYER_BULLET_WIDTH) >= enemies[j].y && playerBullets[i].y < enemies[j].y + (2 * ENEMY_WIDTH)) {
							enemies[j].isActive = 0;
							playerBullets[i].isActive = 0;
						}
					}
				}
			}
        }
    }
}


// later: add a desync between enemies firing, don't make them all fire at the same time


void update_bullet() {
    if (frameCounter_enemy == FIRE_RATE) {
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (enemies[i].isActive) {
				// fire bullet - ie. get velocity, set isActive = 1;
				enemyBullets[curBullet].x = enemies[i].x;
				enemyBullets[curBullet].y = enemies[i].y;
				float deltaX = playerX + (PLAYER_WIDTH - 1) - enemies[i].x;			// Aiming at the centre of the player, hence playerWidth - 1 added
				float deltaY = playerY + (PLAYER_WIDTH - 1) - enemies[i].y;					
				float hyp = squareRoot( pow(deltaX, 2) + pow(deltaY, 2), 0.0001);
				enemyBullets[curBullet].velo_x = BULLET_VELO * (deltaX / hyp);
				enemyBullets[curBullet].velo_y = BULLET_VELO * (deltaY / hyp);
				enemyBullets[curBullet].isActive = 1;
				curBullet = curBullet + 1;


				if (curBullet >= NUM_BULLETS) curBullet = 0;
			}
        }
       
        frameCounter_enemy = 0;
    }
    else {
        frameCounter_enemy = frameCounter_enemy + 1;    
    }
    // later: will need to use a for loop for each
    // change enemyBullets.x in deltas to enemy locations
   
    for (int i = 0; i < NUM_BULLETS; i++) {
        if (enemyBullets[i].isActive) {
            // update movement
            // check if offscreen
            enemyBullets[i].x = enemyBullets[i].x + enemyBullets[i].velo_x;
            enemyBullets[i].y = enemyBullets[i].y + enemyBullets[i].velo_y;
           
			// bullet went offscreen
            if (enemyBullets[i].x >= X_BOUND || enemyBullets[i].x <= 0 || enemyBullets[i].y >= Y_BOUND || enemyBullets[i].y <= 0) {
                reset_bullet(i);
            }
			
			// Collision detection
			if (enemyBullets[i].x >= playerX && enemyBullets[i].x < playerX + (2 * PLAYER_WIDTH)) {
				if (enemyBullets[i].y >= playerY && enemyBullets[i].y < playerY + (2 * PLAYER_WIDTH)) {
					// Collision detected
					isPlayerHit = 1;
				}
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
   
    handle_input();     //later: have a separate input handler for menus
    update_player();
   
    update_bullet();
    update_player_bullets();
   
   
}




void draw() {
    clear_screen();
   
    /*
        depending on state, menu or game, draw something different
    */
   
    for (int i = 0; i < 2 * PLAYER_WIDTH; i++) {
        for (int j = 0; j < 2 * PLAYER_WIDTH; j++) {
           if (isPlayerHit) {
			    plot_pixel(playerX + i, playerY + j, 0x0ff0);  
		   }
		   else {
				 plot_pixel(playerX + i, playerY + j, 0xffff);  
		   }
        }
    }
   
    for (int i = 0; i < 2 * ENEMY_WIDTH; i++) {
        for (int j = 0; j < 2 * ENEMY_WIDTH; j++) {
            for (int k = 0; k < NUM_ENEMIES; k++) {
                if (enemies[k].isActive) {
					plot_pixel(enemies[k].x + i, enemies[k].y + j, 0x07e0);
				}
            }  
        }
    }
   
    for (int k = 0; k < NUM_BULLETS; k++) {
        for (int i = 0; i < 2 * ENEMY_BULLET_WIDTH; i++) {
            for (int j = 0; j < 2 * ENEMY_BULLET_WIDTH; j++) {
                if (enemyBullets[k].isActive) {
                    plot_pixel(enemyBullets[k].x + i, enemyBullets[k].y + j, 0xfff1);  
                }
            }
        }
    }




    for (int k = 0; k < NUM_BULLETS; k++) {
        for (int i = 0; i < 2 * PLAYER_BULLET_WIDTH; i++) {
            for (int j = 0; j < 2 * PLAYER_BULLET_WIDTH; j++) {
                if (playerBullets[k].isActive) {
                    plot_pixel(playerBullets[k].x + i, playerBullets[k].y + j, 0x11ff);
                }
            }
        }
    }
	
	// Drawing walls - really poorly done right now ==============================================================================================================================
	
	for (int i = 0; i < 2 * WALL_WIDTH; i++) {
		for (int j = 0; j < Y_BOUND; j++) {
			plot_pixel(i, j, 0xffff);
		}
	}
	
	for (int i = X_BOUND - 2*WALL_WIDTH; i < X_BOUND; i++) {
		for (int j = 0; j < Y_BOUND; j++) {
			plot_pixel(i, j, 0xffff);
		}
	}
	
	for (int i = 2 * WALL_WIDTH; i < X_BOUND - 2 * WALL_WIDTH; i++) {
		for (int j = 0; j < 2 * WALL_WIDTH; j++) {
			plot_pixel(i, j, 0xffff);
		}
	}
	
	for (int i = 2 * WALL_WIDTH; i < X_BOUND - 2 * WALL_WIDTH; i++) {
		for (int j = Y_BOUND - 2 * WALL_WIDTH; j < Y_BOUND; j++) {
			plot_pixel(i, j, 0xffff);
		}
	}
	
	// ===========================================================================================================================================================================



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
