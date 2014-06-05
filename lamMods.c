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

int xres, yres;
GLuint shipTexture;
GLuint introTexture;
GLuint silhouetteTexture;
Ppmimage *introImage=NULL;
Ppmimage *shipImage;
int keys[65536];
Stats stats;
Ship ship;
struct timespec shipAnimation;
struct timespec timeCurrent;
struct timespec bulletTimer;
double timeDiff(struct timespec *start, struct timespec *end);

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
	stats.moveSpeed = 3;
	stats.fireSpeed=1;
	stats.damage = 1;
}

void checkMovement()
{
	if (keys[XK_Left])
	{
		ship.pos[0] = ship.pos[0]-1.0*stats.moveSpeed;
		if (ship.pos[0] <= 0)
			ship.pos[0] = 0;
    }
    if (keys[XK_Right])
    {
	//	VecCopy(ship.pos, ship.lastpos);
		ship.pos[0] = ship.pos[0]+1.0*stats.moveSpeed;
		if (ship.pos[0] >= xres )
			ship.pos[0] = xres;
	}	
	
    if (keys[XK_Up])
    {
    //  VecCopy(ship.pos, ship.lastpos);
		ship.pos[1] = ship.pos[1]+1.0*stats.moveSpeed;
		if (ship.pos[1] >= yres )
			ship.pos[1] = yres;
    }
    if (keys[XK_Down])
    {
		ship.pos[1] = ship.pos[1]-1.0*stats.moveSpeed;
		if (ship.pos[1] <= 0)
			ship.pos[1] = 0;
    }
    
    if (keys[XK_space])
    {
			double bt = timeDiff(&bulletTimer, &timeCurrent);
			if ( bt > .3*stats.fireSpeed) {
				clock_gettime(CLOCK_REALTIME, &bulletTimer);
				shipShootBullet(ship);
			}
	}
}
