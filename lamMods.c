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

int xres, yres;
GLuint shipTexture;

typedef struct t_ship {
  int shape;
  Vec pos;
  Vec lastpos;
  float width;
  float width2;
  float radius;
} Ship;

typedef double Vec[3];

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
	
	/* check for an error during the load process */
	if ( 0 == shipTexture )
	{
		printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
	}
//	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	
}
