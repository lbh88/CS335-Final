//Lam Ha 
//cs335 Spring 2014
//date:		2014
//
//
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
#include "fmod.h"
#include <FMOD/fmod_errors.h>
#include <stdio.h>

//local global variables are defined here
#define MAX_SOUNDS 32
FMOD_SYSTEM  *xsystem;
FMOD_SOUND   *sound[MAX_SOUNDS];
FMOD_CHANNEL *channel = 0;
static int nsounds=0;
#endif //USE_SOUND
#define MakeVector(x, y, z, v) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define VecCopy(a,b) (b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2]
//macros
#define rnd() (((double)rand())/(double)RAND_MAX)
#define random(a) (rand()%a)

#define USE_SHIP
#ifdef USE_SHIP
#define SHIP_FLAT	0
#define SHIP_ROUND 1

typedef struct t_ship {
  int shape;
  Vec pos;
  Vec lastpos;
  Vec vel;
  float angle;
  float width;
  float width2;
  float radius;
} Ship;
Ship ship;
//int show_ship=0;
int deflection;
#endif //USE_SHIP

Vec bgPos1;
Vec bgPos2;

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

typedef struct t_bullet {
  int bulletCount;
  Vec pos;
  Vec vel;
  int explode;
  float color[3];
  struct timespec time;
  struct t_bullet *prev;
  struct t_bullet *next;
} Bullet;
Bullet *bhead;
Bullet *ahead;


int xres, yres;
int kills;
int dead;
int invincible;
GLuint shipTexture[10];
GLuint bulletTexture[10];
GLuint enemyBulletTexture[3];

GLuint introTexture;
GLuint gameOverTexture;
GLuint silhouetteTexture;
GLuint backgroundTexture;
GLuint backgroundTexture2;
Ppmimage *introImage=NULL;
Ppmimage *gameOverImage=NULL;
Ppmimage *shipImage[10];
Ppmimage *bulletImage[10];
Ppmimage *enemyBulletImage[3];

int keys[65536];
Stats stats;
Ship ship;
struct timespec shipAnimation;
struct timespec invincibleTimer;
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
int bullCount;

unsigned char *buildAlphaData(Ppmimage *img);
void deleteAllBullets();


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
	bgPos1[0]= xres/2;
	bgPos1[1]= yres/2;
	bgPos1[2]= 0;
	bgPos2[0]= xres/2;
	bgPos2[1]= yres + yres/2;
	bgPos2[2]= 0;
	
}

void draw_ship(void)
{
	if (stats.health	> 0)
	{
		double sa;
		int saTime;
		if (!invincible)
		{
			sa =timeDiff(&shipAnimation,&timeCurrent);
			sa = sa*10;
			saTime = (int) sa;
			saTime = saTime%3;
		} else {
			sa = timeDiff(&invincibleTimer, &timeCurrent);
			if (sa >= 1.5)
				invincible ^= 1;
			sa = sa*10;
			saTime = (int) sa;
			saTime = saTime %2;
			if (saTime == 1)
				saTime = 9;
		}
		glPushMatrix();
		glTranslatef(ship.pos[0],ship.pos[1],ship.pos[2]);
		glEnable(GL_ALPHA_TEST);
		//glAlphaFunc(GL_GREATER, 0.0f);
		glBindTexture(GL_TEXTURE_2D, shipTexture[saTime]);
		glBegin(GL_QUADS);
		float w = ship.width2;
		glTexCoord2f(0.0f, 0.0f); glVertex2f(-w, w);
		glTexCoord2f(1.0f, 0.0f); glVertex2f( w, w);
		glTexCoord2f(1.0f, 1.0f); glVertex2f( w, -w);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(-w, -w);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
		//glDisable(GL_ALPHA_TEST);
		glPopMatrix();	
	} else {
		int i = 0;
		deathTime = timeDiff(&deathTimer,&timeCurrent);
		if (deathTime < .4)
		{
			i = 3;
		}
		else if (deathTime < .8)
		{
			i = 4;
		}
		else if (deathTime < 1.2 )
		{
			i = 5;
		}
		else if (deathTime < 1.6 )
		{
			i = 6;
		}
		else if (deathTime < 2)
		{
			i = 7;
		}
		else if (deathTime < 2.2)
		{
			i = 8;
		}
		else if (deathTime >=2.2)
		{
			i = 9;
		}
		if (deathTime >= 2.2)
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
		glPushMatrix();
		glTranslatef(ship.pos[0],ship.pos[1],ship.pos[2]);
		glEnable(GL_ALPHA_TEST);
		//glAlphaFunc(GL_GREATER, 0.0f);
		glBindTexture(GL_TEXTURE_2D, shipTexture[i]);
		glBegin(GL_QUADS);
		float w = ship.width2;
		glTexCoord2f(0.0f, 0.0f); glVertex2f(-w, w);
		glTexCoord2f(1.0f, 0.0f); glVertex2f( w, w);
		glTexCoord2f(1.0f, 1.0f); glVertex2f( w, -w);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(-w, -w);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
		//glDisable(GL_ALPHA_TEST);
		glPopMatrix();
	}
}

void dispBG()
{

	glBindTexture(GL_TEXTURE_2D, backgroundTexture);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 1.0f); glVertex2i(bgPos1[0]-xres/2, bgPos1[1]-yres/2);
		glTexCoord2f(0.0f, 0.0f); glVertex2i(bgPos1[0]-xres/2, bgPos1[1]+yres/2);
		glTexCoord2f(1.0f, 0.0f); glVertex2i(bgPos1[0]+xres/2, bgPos1[1]+yres/2);
		glTexCoord2f(1.0f, 1.0f); glVertex2i(bgPos1[0]+xres/2, bgPos1[1]-yres/2);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, silhouetteTexture);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);
	glColor4ub(255,255,255,255);
	
	glBindTexture(GL_TEXTURE_2D, backgroundTexture2);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 1.0f); glVertex2i(bgPos2[0]-xres/2, bgPos2[1]-yres/2);
		glTexCoord2f(0.0f, 0.0f); glVertex2i(bgPos2[0]-xres/2, bgPos2[1]+yres/2);
		glTexCoord2f(1.0f, 0.0f); glVertex2i(bgPos2[0]+xres/2, bgPos2[1]+yres/2);
		glTexCoord2f(1.0f, 1.0f); glVertex2i(bgPos2[0]+xres/2, bgPos2[1]-yres/2);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, silhouetteTexture);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);
	glColor4ub(255,255,255,255);
	
	bgPos1[1] = bgPos1[1] - (.01* stats.moveSpeed);
	bgPos2[1] = bgPos2[1] - (.01* stats.moveSpeed);
	
	if (bgPos1[1] < (0-yres/2))
	{
		bgPos1[1]= yres + yres/2;
		bgPos1[0] = xres/2;
	}
	if (bgPos2[1] < (0-yres/2))
	{
		bgPos2[1]= yres + yres/2;
		bgPos2[0] = xres/2;
	}
	

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

void buildBulletImage()
{	
	bulletImage[0]  = ppm6GetImage("./images/shipBullet.ppm");
	bulletImage[1]  = ppm6GetImage("./images/shipBullet1.ppm");
	bulletImage[2]  = ppm6GetImage("./images/shipBullet2.ppm");
	bulletImage[3]  = ppm6GetImage("./images/shipBullet3.ppm");
	bulletImage[4]  = ppm6GetImage("./images/shipBullet4.ppm");
	bulletImage[5]  = ppm6GetImage("./images/shipBullet5.ppm");
	bulletImage[6]  = ppm6GetImage("./images/shipBullet6.ppm");
	bulletImage[7]  = ppm6GetImage("./images/shipBullet7.ppm");
	bulletImage[8]  = ppm6GetImage("./images/shipBullet8.ppm");
	printf("before ship bullet\n");
	enemyBulletImage[0]  = ppm6GetImage("./images/enemyBullet.ppm");
	enemyBulletImage[1]  = ppm6GetImage("./images/enemyBullet1.ppm");
	enemyBulletImage[2]  = ppm6GetImage("./images/enemyBullet2.ppm");

	int i = 0; //index for for loop
	for ( i = 0 ; i <= 8; i++)
	{
		glGenTextures(1, &bulletTexture[i]);
		glGenTextures(1, &silhouetteTexture);
			
		int w = bulletImage[i]->width;
		int h = bulletImage[i]->height;
		//
		glBindTexture(GL_TEXTURE_2D, bulletTexture[i]);
		//
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
								GL_RGB, GL_UNSIGNED_BYTE, bulletImage[i]->data);
		unsigned char *silhouetteData = buildAlphaData(bulletImage[i]);	
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
								GL_RGBA, GL_UNSIGNED_BYTE, silhouetteData);
		free(silhouetteData);
	}
	printf("after ship bullet\n");
	for ( i = 0 ; i <= 2; i++)
	{
		glGenTextures(1, &enemyBulletTexture[i]);
		glGenTextures(1, &silhouetteTexture);
			
		int w = enemyBulletImage[i]->width;
		int h = enemyBulletImage[i]->height;
		//
		glBindTexture(GL_TEXTURE_2D, enemyBulletTexture[i]);
		//
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
								GL_RGB, GL_UNSIGNED_BYTE, enemyBulletImage[i]->data);
		unsigned char *silhouetteData = buildAlphaData(enemyBulletImage[i]);	
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
								GL_RGBA, GL_UNSIGNED_BYTE, silhouetteData);
		free(silhouetteData);
	}
}

void buildShipImage()
{	
	shipImage[0]  = ppm6GetImage("./images/spaceship.ppm");
	shipImage[1]  = ppm6GetImage("./images/spaceship1.ppm");
	shipImage[2]  = ppm6GetImage("./images/spaceship2.ppm");
	shipImage[3]  = ppm6GetImage("./images/explosion.ppm");
	shipImage[4]  = ppm6GetImage("./images/explosion1.ppm");
	shipImage[5]  = ppm6GetImage("./images/explosion2.ppm");
	shipImage[6]  = ppm6GetImage("./images/explosion3.ppm");
	shipImage[7]  = ppm6GetImage("./images/explosion4.ppm");
	shipImage[8]  = ppm6GetImage("./images/explosion5.ppm");
	shipImage[9]  = ppm6GetImage("./images/explosion6.ppm");

	int i = 0; //index for for loop
	for ( i = 0 ; i <= 9; i++)
	{
		glGenTextures(1, &shipTexture[i]);
		glGenTextures(1, &silhouetteTexture);
			
		int w = shipImage[i]->width;
		int h = shipImage[i]->height;
		//
		glBindTexture(GL_TEXTURE_2D, shipTexture[i]);
		//
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
								GL_RGB, GL_UNSIGNED_BYTE, shipImage[i]->data);
								
								
		unsigned char *silhouetteData = buildAlphaData(shipImage[i]);	
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
								GL_RGBA, GL_UNSIGNED_BYTE, silhouetteData);
		free(silhouetteData);
	}

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
		double fire = ( stats.fireSpeed*.02 > .15 ) ? .15 : stats.fireSpeed*.02 ;
			if ( ts > .3 - fire) {
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
	if (fmod_init()) {
		printf("ERROR - fmod_init()\n\n");
		return;
	}
	if (fmod_createsound("./sounds/explosion2.mp3", 4)) {
		printf("ERROR - fmod_createsound()\n\n");
		return;
	}
	
	fmod_setmode(4,FMOD_LOOP_OFF);
	
#endif //USE_SOUND
}



void checkUpgrades()
{
	if (kills == 0)
		return;
	if ( kills%5 == 0 )
	{
		stats.fireSpeed = stats.fireSpeed + 1;
	}
	if ( kills%15 == 0 )
	{
		stats.moveSpeed = stats.moveSpeed + 1;
	}
	if ( kills%30 == 0 )
	{
		stats.health = stats.health+ 1;
	}	
	
}

void enemyMovement()
{
	if (enemy.pos[0] < ship.pos[0])
	{
		enemy.pos[0] += 1 + .4*stats.moveSpeed;
	}
	else if (enemy.pos[0] > ship.pos[0])
	{
		enemy.pos[0] -= 1 + .4*stats.moveSpeed;
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
		deleteAllBullets();
		fmod_cleanupIntro(0);
		show_enemy ^= 1;
		bg ^= 1;
		
		clock_gettime(CLOCK_REALTIME, &deathTimer);	
		#ifdef USE_SOUND
/*		//FMOD_RESULT result;
		if (fmod_init()) {
			printf("ERROR - fmod_init()\n\n");
			return;
		}
		if (fmod_createsound("./sounds/explosion2.mp3", 0)) {
			printf("ERROR - fmod_createsound()\n\n");
			return;
		}
		fmod_setmode(0,FMOD_LOOP_OFF); */
		fmod_playsound(4);
		#endif //USE_SOUND
	}
	return;
}
	
void restartGame()
{
	deathTime = 0.0;
	kills = 0;
	fmod_cleanupIntro(0);
	init_music();
	//fmod_cleanupIntro(4);
	ship.pos[0] = 320.0;
	ship.pos[1] = (double)(yres-400);
	
	enemy.pos[0] = random(xres);
	enemy.pos[1] = (double)(yres)+50;
	fmod_playsound(0);
	initStats();
	show_enemy ^= 1;
	show_ship ^= 1;
	dead ^= 1;
	bg ^= 1;
	
}

int fmod_cleanupIntro(int i)
{
	int result = FMOD_Sound_Release(sound[i]);
	nsounds--;
	if (ERRCHECK(result)) {
		return 1;
	}
	return 0;
}

int fmod_playsound(int i)
{
	FMOD_RESULT result;
	//printf("fmod_playsound(%i)...\n",i);
	result = FMOD_System_PlaySound(xsystem, FMOD_CHANNEL_FREE, sound[i], 0, &channel);
	if (ERRCHECK(result)) {
		printf("error fmod_playsound()\n");
		return 1;
	}
	result = FMOD_System_Update(xsystem);
	if (ERRCHECK(result)) {
		printf("error updating system \n");
		return 1;
	}
	return 0;
}

void deleteAllBullets()
{
	Bullet *a = ahead;
	Bullet *b = bhead;
	while (a) 
	{
		Bullet *sa = a->next;
		deleteBullet(a);
		a = sa;
	}
	while (b)
	{
		Bullet  *sb = b->next;
		deleteBullet(b);
		b = sb;
	}
	ahead = NULL;
	bhead = NULL;
	
}

