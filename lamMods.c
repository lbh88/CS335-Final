#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <X11/Xlib.h>
//#include <X11/Xutil.h>
//#include <GL/gl.h>
//#include <GL/glu.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "log.h"
#include "ppm.h"
#include "fonts.h"
#include "lamMods.h"
#include "nickMods.h"
#define USE_SOUND

#ifdef USE_SOUND
#include <FMOD/fmod.h>
#include <FMOD/wincompat.h>
#include "fmod.h"
#endif //USE_SOUND
#define MakeVector(x, y, z, v) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define VecCopy(a,b) (b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2]
//macros
#define rnd() (((double)rand())/(double)RAND_MAX)
#define random(a) (rand()%a)

int xres, yres;
int kills;
int dead;
GLuint shipTexture;
GLuint introTexture;
GLuint gameOverTexture;
GLuint silhouetteTexture;
Ppmimage *introImage=NULL;
Ppmimage *gameOverImage=NULL;
Ppmimage *shipImage;
int keys[65536];
Stats stats;
Ship ship;
struct timespec shipAnimation;
struct timespec timeCurrent;
struct timespec bulletTimer;
double timeDiff(struct timespec *start, struct timespec *end);
void timeCopy(struct timespec *dest, struct timespec *source);
int show_enemy;
int show_ship;
int bg;
struct timespec deathTimer;
double deathTime;
int play_sounds;

unsigned char *buildAlphaData(Ppmimage *img);

typedef struct t_ship {
  int shape;
  Vec pos;
  Vec lastpos;
  float width;
  float width2;
  float radius;
} Ship;

typedef struct t_stats {
	int health;
	int moveSpeed;
	int fireSpeed;
	int damage;
	int upgrades[10];
} Stats;

typedef struct t_enemy {
  int shape;
  Vec pos;
  Vec lastpos;
  Vec vel;
  float angle;
  float width;
  float width2;
  float radius;
} Enemy;
Enemy enemy;

typedef double Vec[3];

void intro()
{
	introImage	= ppm6GetImage("./images/logo.ppm");	
	glGenTextures(1, &introTexture);
	glBindTexture(GL_TEXTURE_2D, introTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, introImage->width, introImage->height, 0, GL_RGB, GL_UNSIGNED_BYTE, introImage->data);
	
	if (fmod_init()) {
		printf("ERROR - fmod_init()\n\n");
		return;
	}
	if (fmod_createsound("./sounds/burn1.mp3", 0)) {
		printf("ERROR - fmod_createsound()\n\n");
		return;
	}
	fmod_setmode(0,FMOD_LOOP_NORMAL);
//		#ifdef USE_SOUND 
			//fmod_playsound(0);
			fmod_playsound(0);
//		#endif //USE_SOUND
}

void endIntroSong()
{
	
}

void dispShip(Ship ship, GLuint shipTexture)
{
	glPushMatrix();
	glTranslatef(ship.pos[0],ship.pos[1],ship.pos[2]);
	glBindTexture(GL_TEXTURE_2D, shipTexture);
        glBegin(GL_QUADS);
	glEnd();
	glPopMatrix();
}

void dispBG(GLuint backgroundTexture)
{
	glBindTexture(GL_TEXTURE_2D, backgroundTexture);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 1.0f); glVertex2i(0,0);
		glTexCoord2f(0.0f, 0.0f); glVertex2i(0, yres);
		glTexCoord2f(1.0f, 0.0f); glVertex2i(xres, yres);
		glTexCoord2f(1.0f, 1.0f); glVertex2i(xres, 0);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, silhouetteTexture);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);
	glColor4ub(255,255,255,255);
}

void dispIntro()
{
		Rect r;
		//Clear the screen
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		//
		//
		//draw a quad with texture
	//	float wid = 120.0f;
		glColor3f(1.0, 1.0, 1.0);	
		glBindTexture(GL_TEXTURE_2D, introTexture);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f); glVertex2i(0,0);
			glTexCoord2f(0.0f, 0.0f); glVertex2i(0, yres);
			glTexCoord2f(1.0f, 0.0f); glVertex2i(xres, yres);
			glTexCoord2f(1.0f, 1.0f); glVertex2i(xres, 0);
		glEnd();	
		glDisable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glDisable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		r.bot = yres/2 - 50;
		r.left = xres/2;
		r.center = 0;
		unsigned int cref = 0x00ffffff;
		ggprint8b(&r, 16, cref, "S - Start Game");
	
}

void buildShipImage()
{	
	double sa =timeDiff(&shipAnimation,&timeCurrent);
	sa = sa*10;
	int saTime = (int) sa;
	saTime = saTime%4;
	if (stats.health	> 0)
	{
		switch (saTime)
		{
			case 0:
				shipImage  = ppm6GetImage("./images/spaceship.ppm");
				break;
			case 2:
				shipImage  = ppm6GetImage("./images/spaceship2.ppm");
				break;
			default: 
				shipImage  = ppm6GetImage("./images/spaceship1.ppm");
				break;
		}
	}
	else
	{
		deathTime = timeDiff(&deathTimer,&timeCurrent);
		if (deathTime < .4)
		{
			shipImage = ppm6GetImage("./images/explosion.ppm");
		}
		else if (deathTime < .8)
		{
			shipImage = ppm6GetImage("./images/explosion1.ppm");
		}
		else if (deathTime < 1.2 )
		{
			shipImage = ppm6GetImage("./images/explosion2.ppm");
		}
		else if (deathTime < 1.6 )
		{
			shipImage = ppm6GetImage("./images/explosion3.ppm");
		}
		else if (deathTime < 2)
		{
			shipImage = ppm6GetImage("./images/explosion4.ppm");
		}
		
		if (deathTime > 2)
		{
			show_ship ^= 1;
			dead ^= 1;
			printf("playing sawyer\n");
			if (fmod_init()) {
				printf("ERROR - fmod_init()\n\n");
				return;
			}
			if (fmod_createsound("./sounds/sawyer.mp3", 0)) {
				printf("ERROR - fmod_createsound()\n\n");
				return;
			}
			fmod_setmode(0,FMOD_LOOP_OFF);
			fmod_playsound(0);
		}
		
	}
	//
	//create opengl texture elements
	glGenTextures(1, &shipTexture);
	glGenTextures(1, &silhouetteTexture);
		
	int w = shipImage->width;
	int h = shipImage->height;
	//
	glBindTexture(GL_TEXTURE_2D, shipTexture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
							GL_RGB, GL_UNSIGNED_BYTE, shipImage->data);
							
							
	unsigned char *silhouetteData = buildAlphaData(shipImage);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
							GL_RGBA, GL_UNSIGNED_BYTE, silhouetteData);
	free(silhouetteData);

}

void initStats()
{
	stats.health = 5;
	stats.moveSpeed = 4;
	stats.fireSpeed=1;
	stats.damage = 1;
}

void checkMovement()
{
	if (keys[XK_Left])
	{
		ship.pos[0] = ship.pos[0] - 1.0*stats.moveSpeed;
		if (ship.pos[0] <= 0)
			ship.pos[0] = 0;
    }
    if (keys[XK_Right])
    {
	//	VecCopy(ship.pos, ship.lastpos);
		ship.pos[0] = ship.pos[0] + 1.0*stats.moveSpeed;
		if (ship.pos[0] >= xres )
			ship.pos[0] = xres;
	}	
	
    if (keys[XK_Up])
    {
    //  VecCopy(ship.pos, ship.lastpos);
		ship.pos[1] = ship.pos[1] + 1.0*stats.moveSpeed;
		if (ship.pos[1] >= yres )
			ship.pos[1] = yres;
    }
    if (keys[XK_Down])
    {
		ship.pos[1] = ship.pos[1] - 1.0*stats.moveSpeed;
		if (ship.pos[1] <= 0)
			ship.pos[1] = 0;
    }  
    if (keys[XK_space])
    {
		struct timespec bt;
		clock_gettime(CLOCK_REALTIME, &bt);
		double ts = timeDiff(&bulletTimer, &timeCurrent);
			if ( ts > .3 - (stats.fireSpeed/100)*5) {
				timeCopy(&bulletTimer, &bt);
				if (play_sounds)
					fmod_playsound(1);
				shipShootBullet();
			}
	}
}

void init_music(void)
{
#ifdef USE_SOUND
	//FMOD_RESULT result;
	if (fmod_init()) {
		printf("ERROR - fmod_init()\n\n");
		return;
	}
	if (fmod_createsound("./sounds/burn2.mp3", 0)) {
		printf("ERROR - fmod_createsound()\n\n");
		return;
	}
	fmod_setmode(0,FMOD_LOOP_NORMAL);

	
#endif //USE_SOUND
}

void init_sounds(void)
{
#ifdef USE_SOUND
	//FMOD_RESULT result;
	if (fmod_init()) {
		printf("ERROR - fmod_init()\n\n");
		return;
	}
	if (fmod_createsound("./sounds/burn2.mp3", 0)) {
		printf("ERROR - fmod_createsound()\n\n");
		return;
	}
	fmod_setmode(0,FMOD_LOOP_NORMAL);
	
	if (fmod_init()) {
		printf("ERROR - fmod_init()\n\n");
		return;
	}
	if (fmod_createsound("./sounds/shot1.mp3", 1)) {
		printf("ERROR - fmod_createsound()\n\n");
		return;
	}
	fmod_setmode(1,FMOD_LOOP_OFF);
	
	if (fmod_init()) {
		printf("ERROR - fmod_init()\n\n");
		return;
	}
	if (fmod_createsound("./sounds/shot2.mp3", 2)) {
		printf("ERROR - fmod_createsound()\n\n");
		return;
	}
	fmod_setmode(2,FMOD_LOOP_OFF);
	
	if (fmod_init()) {
		printf("ERROR - fmod_init()\n\n");
		return;
	}
	if (fmod_createsound("./sounds/explosion1.mp3", 3)) {
		printf("ERROR - fmod_createsound()\n\n");
		return;
	}
	fmod_setmode(3,FMOD_LOOP_OFF);
	
#endif //USE_SOUND
}



void checkUpgrades()
{
	if (kills == 0)
		return;
	if ( kills%10 == 0 )
	{
		stats.fireSpeed = stats.fireSpeed + 1;
	}
	if ( kills%20 == 0 )
	{
		stats.moveSpeed = stats.moveSpeed + 1;
	}
	if ( kills%50 == 0 )
	{
		stats.health = stats.health+ 1;
	}	
	
}

void enemyMovement()
{
	if (enemy.pos[0] < ship.pos[0])
	{
		enemy.pos[0] += 1;
	}
	else if (enemy.pos[0] > ship.pos[0])
	{
		enemy.pos[0] -= 1;
	}
	
}

void gameOver()
{
	gameOverImage	= ppm6GetImage("./images/gameover.ppm");	
	glGenTextures(1, &gameOverTexture);
	glBindTexture(GL_TEXTURE_2D, gameOverTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, gameOverImage->width, gameOverImage->height, 0, GL_RGB, GL_UNSIGNED_BYTE, gameOverImage->data);
	
/*	if (fmod_init()) {
		printf("ERROR - fmod_init()\n\n");
		return;
	}
	if (fmod_createsound("./sounds/burn1.mp3", 0)) {
		printf("ERROR - fmod_createsound()\n\n");
		return;
	}
	fmod_setmode(0,FMOD_LOOP_NORMAL);
//		#ifdef USE_SOUND 
			//fmod_playsound(0);
			fmod_playsound(0);
//		#endif //USE_SOUND
*/

}

void dispGameOver()
{
//		Rect r;
		//Clear the screen
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		//
		//
		//draw a quad with texture
	//	float wid = 120.0f;
		glColor3f(1.0, 1.0, 1.0);	
		glBindTexture(GL_TEXTURE_2D, gameOverTexture);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f); glVertex2i(0,0);
			glTexCoord2f(0.0f, 0.0f); glVertex2i(0, yres);
			glTexCoord2f(1.0f, 0.0f); glVertex2i(xres, yres);
			glTexCoord2f(1.0f, 1.0f); glVertex2i(xres, 0);
		glEnd();	
		glDisable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glDisable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
//		r.bot = yres/2 - 50;
//		r.left = xres/2;
//		r.center = 0;
		//unsigned int cref = 0x00ffffff;
		//ggprint8b(&r, 16, cref, "S - Start Game");
	
}

void checkDeath()
{
	if (stats.health <= 0 )
	{
		fmod_cleanupIntro(0);
		show_enemy ^= 1;
		bg ^= 1;
		
		clock_gettime(CLOCK_REALTIME, &deathTimer);	
		#ifdef USE_SOUND
		//FMOD_RESULT result;
		if (fmod_init()) {
			printf("ERROR - fmod_init()\n\n");
			return;
		}
		if (fmod_createsound("./sounds/explosion2.mp3", 0)) {
			printf("ERROR - fmod_createsound()\n\n");
			return;
		}
		fmod_setmode(0,FMOD_LOOP_OFF);
		fmod_playsound(0);
		#endif //USE_SOUND
	}
	return;
}
	
void restartGame()
{
	deathTime = 0.0;
	fmod_cleanupIntro(0);
	fmod_cleanupIntro(4);
	ship.pos[0] = 320.0;
	ship.pos[1] = (double)(yres-400);
	
	enemy.pos[0] = random(xres);
	enemy.pos[1] = (double)(yres)+50;
	init_music();
	fmod_playsound(0);
	stats.health = 5;
	show_enemy ^= 1;
	show_ship ^= 1;
	dead ^= 1;
	bg ^= 1;
	
}



