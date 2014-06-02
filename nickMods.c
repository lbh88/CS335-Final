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
#include "nickMods.h"

#define USE_SOUND

#ifdef USE_SOUND
#include <FMOD/fmod.h>
#include <FMOD/wincompat.h>
#include "fmod.h"
#endif //USE_SOUND

#define rnd() (((double)rand())/(double)RAND_MAX)
#define PI 3.141592653589793

typedef double Vec[3];
typedef double Flt;
/*Ppmimage *shipImage=NULL;
Ppmimage *backgroundImage=NULL;
GLuint shipTexture;
GLuint backgroundTexture;
*/

int xres, yres;

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

typedef struct t_bullet {
  Vec pos;
  Vec vel;
  float color[3];
  struct timespec time;
  struct t_bullet *prev;
  struct t_bullet *next;
} Bullet;
Bullet *bhead;
void deleteBullet(Bullet *node);
struct timespec bulletTimer;

void shipShootBullet(Ship *ship) {  
  //timeCopy(&bulletTimer, &bt);
  //shoot a bullet...
  Bullet *b = (Bullet *)malloc(sizeof(Bullet));
  //timeCopy(&b->time, &bt);
  b->pos[0] = ship.pos[0];
  b->pos[1] = ship.pos[1];
  b->vel[0] = ship.vel[0];
  b->vel[1] = ship.vel[1];
  //convert ship angle to radians
  Flt rad = ((ship.angle+90.0) / 360.0f) * PI * 2.0;
  //convert angle to a vector
  Flt xdir = cos(rad);
  Flt ydir = sin(rad);
  b->pos[0] += xdir*20.0f;
  b->pos[1] += ydir*20.0f;
  b->vel[0] += xdir*6.0f + rnd()*0.1;
  b->vel[1] += ydir*6.0f + rnd()*0.1;
  b->color[0] = 1.0f;
  b->color[1] = 1.0f;
  b->color[2] = 1.0f;
  //put bullet into linked list
  b->prev = NULL;
  b->next = bhead;
  if (bhead != NULL)
    bhead->prev = b;
  bhead = b;
}

void updateShipBulletPosition() {
  //Update bullet positions
  //struct timespec bt;
  //clock_gettime(CLOCK_REALTIME, &bt);
  Bullet *b = bhead;
  while(b) {
    b->pos[0] += b->vel[0];
    b->pos[1] += b->vel[1];
    //Check for collision with window edges
    if (b->pos[0] < 0.0) {
      deleteBullet(b);
    }
    else if (b->pos[0] > (Flt)xres) {
      deleteBullet(b);
    }
    else if (b->pos[1] < 0.0) {
      deleteBullet(b);
    }
    else if (b->pos[1] > (Flt)yres) {
      deleteBullet(b);
    }
    b = b->next;
  }
}

void renderShipBullet() {
  Bullet *b = bhead;
  while(b) {
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_POINTS);
    glVertex2f(b->pos[0],      b->pos[1]);
    glVertex2f(b->pos[0]-1.0f, b->pos[1]);
    glVertex2f(b->pos[0]+1.0f, b->pos[1]);
    glVertex2f(b->pos[0],      b->pos[1]-1.0f);
    glVertex2f(b->pos[0],      b->pos[1]+1.0f);
    glColor3f(0.8, 0.8, 0.8);
    glVertex2f(b->pos[0]-1.0f, b->pos[1]-1.0f);
    glVertex2f(b->pos[0]-1.0f, b->pos[1]+1.0f);
    glVertex2f(b->pos[0]+1.0f, b->pos[1]-1.0f);
    glVertex2f(b->pos[0]+1.0f, b->pos[1]+1.0f);
    glEnd();
    b = b->next;
  }

}


void deleteBullet(Bullet *node)
{
  if((node->next == NULL) && (node->prev == NULL)) {
    bhead = NULL;
    free(node);
    node = NULL;
  }

  else if((node->next != NULL) && (node->prev == NULL)) {
    Bullet *tmp = node->next;
    tmp->prev = NULL;
    bhead = node->next;
    free(node);
    node = NULL;
  }

  else if((node->next == NULL) && (node->prev != NULL)) {
    Bullet *tmp = node->prev;
    tmp->next = NULL;
    free(node);
    node = NULL;
  }

  else {
    Bullet *tmp1, *tmp2;
    tmp1 = node->next;
    tmp2 = node->prev;
    tmp1->prev = tmp2;
    tmp2->next = tmp1;
    free(node);
    node = NULL;
  }
}

