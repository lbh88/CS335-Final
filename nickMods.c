//Nicholas Jones
//Homework 3
//Project Source Code
//cs335 Spring 2014
//date:		2014
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
#include "nickMods.h"
#include <FMOD/fmod.h>
#include <FMOD/wincompat.h>
#include "fmod.h"

//defined types
typedef double Flt;
typedef double Vec[3];

//macros
#define rnd() (((double)rand())/(double)RAND_MAX)
#define random(a) (rand()%a)
//constants
#define PI 3.141592653589793
#define VecCopy(a,b) (b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2]
int xres, yres;

Ppmimage *enemyImage;
Ppmimage *shipImage;
GLuint shipTexture;
GLuint enemyTexture;
GLuint silhouetteTexture;

unsigned char *buildAlphaData(Ppmimage *img);

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
void buildEnemyImage();
void dispEnemy(Enemy enemy, GLuint enemyTexture); 

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
int kills;

void shipShootBullet() {
  //shoot a bullet...
  Bullet *b = (Bullet *)malloc(sizeof(Bullet));
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

void check_enemies() {
  if(enemy.pos[1] > (double)(yres)-600) {
    enemy.pos[1] = enemy.pos[1]-2.5;
    dispEnemy(enemy, enemyTexture);
  }
}

void deleteEnemy() {
  enemy.pos[0] = random(xres);
  enemy.pos[1] = (double)(yres)+50;
}

void updateBulletPos() {
  struct timespec bt;
  clock_gettime(CLOCK_REALTIME, &bt);
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
    if (b->pos[0] >= enemy.pos[0]-20 && b->pos[0] <= enemy.pos[0]+20
        && b->pos[1] >= enemy.pos[1]-30 && b->pos[1] <= enemy.pos[1]+30) {
      deleteBullet(b);
      deleteEnemy();
      kills++;
    }
    b = b->next;
  }
  if(enemy.pos[1] < (double)(yres)-500)
    deleteEnemy();
}

void draw_enemy(void)
{
  buildEnemyImage();
  //glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
  glPushMatrix();
  glTranslatef(enemy.pos[0],enemy.pos[1],enemy.pos[2]);
  glEnable(GL_ALPHA_TEST);
  //glAlphaFunc(GL_GREATER, 0.0f);
  glBindTexture(GL_TEXTURE_2D, shipTexture);
  glBegin(GL_QUADS);
  float w = enemy.width2;
  glTexCoord2f(0.0f, 0.0f); glVertex2f(-w, w);
  glTexCoord2f(1.0f, 0.0f); glVertex2f( w, w);
  glTexCoord2f(1.0f, 1.0f); glVertex2f( w, -w);
  glTexCoord2f(0.0f, 1.0f); glVertex2f(-w, -w);
  glEnd();
  glBindTexture(GL_TEXTURE_2D, 0);
  //glDisable(GL_ALPHA_TEST);
  glPopMatrix();
}

void initEnemy() {
  enemy.pos[0] = 160.0;
  enemy.pos[1] = (double)(yres)+50;
  VecCopy(enemy.pos, enemy.lastpos);
  enemy.width = 50.0;
  enemy.width2 = enemy.width * 0.5;
  enemy.radius = (float)enemy.width2;
  enemy.shape = 1; //SHIP_FLAT;
}
void initBullet() {
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

void buildEnemyImage()
{
  enemyImage     = ppm6GetImage("./images/enemy.ppm");
  //
  //create opengl texture elements
  glGenTextures(1, &enemyTexture);
  glGenTextures(1, &silhouetteTexture);
  int w = enemyImage->width;
  int h = enemyImage->height;
  //
  glBindTexture(GL_TEXTURE_2D, enemyTexture);
  //
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
      GL_RGB, GL_UNSIGNED_BYTE, enemyImage->data);

  unsigned char *silhouetteData = buildAlphaData(enemyImage);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
      GL_RGBA, GL_UNSIGNED_BYTE, silhouetteData);
  free(silhouetteData);
}

void dispEnemy(Enemy enemy, GLuint enemyTexture)
{
  glPushMatrix();
  glTranslatef(enemy.pos[0],enemy.pos[1],enemy.pos[2]);
  glBindTexture(GL_TEXTURE_2D, enemyTexture);
  glBegin(GL_QUADS);
  glEnd();
  glPopMatrix();
}
