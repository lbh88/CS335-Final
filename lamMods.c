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
#include "SOIL/SOIL.h"
#define USE_SOUND

#ifdef USE_SOUND
#include <FMOD/fmod.h>
#include <FMOD/wincompat.h>
#include "fmod.h"
#endif //USE_SOUND

int xres, yres;
GLuint shipTexture;
GLuint introTexture;
Ppmimage *introImage=NULL;

typedef struct t_ship {
  int shape;
  Vec pos;
  Vec lastpos;
  float width;
  float width2;
  float radius;
} Ship;

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
	if (fmod_createsound("./sounds/burn1.wav", 0)) {
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
	glEnable( GL_BLEND); 
	shipTexture = SOIL_load_OGL_texture
	(
		"./images/spaceship.png",
		SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_POWER_OF_TWO|SOIL_FLAG_MIPMAPS|SOIL_FLAG_MULTIPLY_ALPHA
	);
	if ( 0 == shipTexture )
	{
		printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
	}

}
