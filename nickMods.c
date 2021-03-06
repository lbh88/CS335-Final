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
#include "lamMods.h"
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
int play_sounds;
int bullCount = 0;
int enemyBullCount = 0;
int invincible;

Ppmimage *enemyImage;
Ppmimage *shipImage[10];
Ppmimage *bulletImage[10];
Ppmimage *enemyBulletImage[3];
GLuint shipTexture[10];
GLuint enemyTexture;
GLuint silhouetteTexture;
GLuint bulletTexture[10];
GLuint enemyBulletTexture[3];
struct timespec timeCurrent;
struct timespec bulletTimer;
struct timespec enemyBulletTimer;
struct timespec invincibleTimer;
double timeDiff(struct timespec *start, struct timespec *end);
unsigned char *buildAlphaData(Ppmimage *img);

void enemyMovement();
void timeCopy(struct timespec *dest, struct timespec *source);

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

typedef struct t_stats {
  int health;
  int moveSpeed;
  int fireSpeed;
  int damage;
  int upgrades[10];
} Stats;
Stats stats;

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
void deleteBullet(Bullet *node);
int kills;
int vic;
int show_enemy;

void shipShootBullet() {
  //shoot a bullet...
  Bullet *b = (Bullet *)malloc(sizeof(Bullet));
  timeCopy(&b->time, &timeCurrent);
  b->explode = 0;
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
  bullCount++;
  b->bulletCount = bullCount;
  printf("Firing bullet: %d\n",b->bulletCount);
}

void enemyShootBullet() {
  enemyBullCount++;
  //if (play_sounds)
  //fmod_playsound(2);
  //shoot a bullet...
  Bullet *a = (Bullet *)malloc(sizeof(Bullet));
  timeCopy(&a->time, &timeCurrent);
  a->pos[0] = enemy.pos[0];
  a->pos[1] = enemy.pos[1];
  a->vel[0] = enemy.vel[0];
  a->vel[1] = enemy.vel[1];
  //convert ship angle to radians
  Flt rad = ((enemy.angle+270.0) / 360.0f) * PI * 2.0;
  //convert angle to a vector
  Flt xdir = cos(rad);
  Flt ydir = sin(rad);
  a->pos[0] -= xdir*20.0f;
  a->pos[1] -= ydir*20.0f;
  a->vel[0] -= xdir*6.0f + rnd()*0.1;
  a->vel[1] -= ydir*6.0f + rnd()*0.1;
  a->color[0] = 1.0f;
  a->color[1] = 1.0f;
  a->color[2] = 1.0f;
  //put bullet into linked list
  a->prev = NULL;
  a->next = ahead;
  if (ahead != NULL)
    ahead->prev = a;
  ahead = a;
}

void deleteEnemy() {
  enemy.pos[0] = random(xres);
  enemy.pos[1] = (double)(yres)+50;
}

void check_enemies() {
  if(enemy.pos[1] > (double)(yres)-600) {
    enemy.pos[1] = enemy.pos[1] - (2.5 + .3*stats.moveSpeed);
    enemyMovement();
    dispEnemy(enemy, enemyTexture);
  }
  if(enemy.pos[0] > ship.pos[0] - 150 && enemy.pos[0] < ship.pos[0] + 150 )
  {
	  double at = timeDiff(&enemyBulletTimer, &timeCurrent);
	  double fire = ( stats.fireSpeed*.3 > 1.35 ) ? 1.35 : stats.fireSpeed*.3;
	  if(at > 1.5 - fire) {
		  if ( play_sounds )
			fmod_playsound(2);		
		clock_gettime(CLOCK_REALTIME, &enemyBulletTimer);
		enemyShootBullet();
		printf("enemy bullet count: %d\n", enemyBullCount);
	  }
	  if(enemy.pos[1] < (double)(yres)-500) {
		deleteEnemy();
		//stats.health--;
		//checkDeath();
	  }
  }
}



void deleteEnemyBullet(Bullet *node)
{
	enemyBullCount--;
  if((node->next == NULL) && (node->prev == NULL)) {
    ahead = NULL;
    free(node);
    node = NULL;
  }

  else if((node->next != NULL) && (node->prev == NULL)) {
    Bullet *tmp = node->next;
    tmp->prev = NULL;
    ahead = node->next;
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

void updateBulletPos() {
  struct timespec bt;
  clock_gettime(CLOCK_REALTIME, &bt);
  Bullet *b = bhead;
  while(b) {
	  if (b->explode == 0)
	  {
		b->pos[0] += b->vel[0];
		b->pos[1] += b->vel[1] + .3 * stats.moveSpeed;
		}
	  if (b->explode == 1)
	  {
		  b->pos[0] += b->vel[0];
		  b->pos[1] -= 2.5;
	  }
		  
    //Check for collision with window edges
    if (b->pos[0] < 0.0) {
	  Bullet *saveb = b->next;
      deleteBullet(b);
      b = saveb;
      continue;
    }
    else if (b->pos[0] > (Flt)xres) {
      Bullet *saveb = b->next;
      deleteBullet(b);
      b = saveb;
      continue;
    }
    else if (b->pos[1] < 0.0) {
      Bullet *saveb = b->next;
      deleteBullet(b);
      b = saveb;
      continue;
    }
    else if (b->pos[1] > (Flt)yres) {
      Bullet *saveb = b->next;
      deleteBullet(b);
      b = saveb;
      continue;
    }
    if (b->explode == 0)
    {
		if (b->pos[0] >= enemy.pos[0]-20 && b->pos[0] <= enemy.pos[0]+20
			&& b->pos[1] >= enemy.pos[1]-30 && b->pos[1] <= enemy.pos[1]+30) {
				
			if (play_sounds)
				fmod_playsound(3);
			printf("Hit enemy -- ");
		  Bullet *saveb = b->next;
		  //deleteBullet(b);
		  b->explode = 1;
		  timeCopy(&b->time, &timeCurrent);
		  b = saveb;
	//      b = saveb;
		  deleteEnemy();
		  kills++;
		  checkVictory();
		  checkUpgrades();    
		  continue;
		}
	}
    b = b->next;
  }
  
}

void updateEnemyBulletPos() {
	  Bullet *a = ahead;
  while(a) {
    a->pos[0] -= a->vel[0];// + .3*stats.moveSpeed;
    a->pos[1] -= a->vel[1] + .3*stats.moveSpeed;
    //Check for collision with window edges
    if (a->pos[1] < 0) {
	  Bullet *savea = a->next;
      deleteEnemyBullet(a);
      a = savea;
      continue;
    }
    else if (a->pos[0] >= ship.pos[0]-20 && a->pos[0] <= ship.pos[0]+20
        && a->pos[1] >= ship.pos[1]-30 && a->pos[1] <= ship.pos[1]+30) {
	  Bullet *savea = a->next;		
      deleteEnemyBullet(a);
      a = savea;
      if(!invincible)
      {
		  stats.health--;
		  timeCopy(&invincibleTimer, &timeCurrent);
		  invincible ^= 1;
		  checkDeath();
	  }
	  continue;
    }
    a = a->next;
  }
  if (ship.pos[0] >= enemy.pos[0]-20 && ship.pos[0] <= enemy.pos[0]+20
      && ship.pos[1] >= enemy.pos[1]-30 && ship.pos[1] <= enemy.pos[1]+30) {
    deleteEnemy();
    kills++;
    checkVictory();
    checkUpgrades();
    if (!invincible)
    {
		stats.health--;
		timeCopy(&invincibleTimer, &timeCurrent);
		invincible ^= 1;
		checkDeath();
	}
  }
}
void draw_enemy(void)
{
  //buildEnemyImage();
  //glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
  glPushMatrix();
  glTranslatef(enemy.pos[0],enemy.pos[1],enemy.pos[2]);
  glEnable(GL_ALPHA_TEST);
  //glAlphaFunc(GL_GREATER, 0.0f);
  glBindTexture(GL_TEXTURE_2D, enemyTexture);
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
	  /*
    glColor3f(0, 0, 255);
    glBegin(GL_POINTS);
    glVertex2f(b->pos[0],      b->pos[1]);
    glVertex2f(b->pos[0]-1.0f, b->pos[1]);
    glVertex2f(b->pos[0]+1.0f, b->pos[1]);
    glVertex2f(b->pos[0],      b->pos[1]-1.0f);
    glVertex2f(b->pos[0],      b->pos[1]+1.0f);
    glColor3f(.8, .8, 255);
    glVertex2f(b->pos[0]-1.0f, b->pos[1]-1.0f);
    glVertex2f(b->pos[0]-1.0f, b->pos[1]+1.0f);
    glVertex2f(b->pos[0]+1.0f, b->pos[1]-1.0f);
    glVertex2f(b->pos[0]+1.0f, b->pos[1]+1.0f);
    glEnd();
    b = b->next;
    */
		double sa;
		int saTime;
		if (b->explode == 0)
		{
			sa =timeDiff(&b->time,&timeCurrent);
			sa = sa*10;
			saTime = (int) sa;
			saTime = saTime%3;
		} else {
			sa = timeDiff(&b->time, &timeCurrent);
			sa = sa*10;
			saTime = (int) sa;
			saTime = saTime %6;
			saTime += 3;

		}
		glPushMatrix();
		glTranslatef(b->pos[0],b->pos[1],b->pos[2]);
		glEnable(GL_ALPHA_TEST);
		//glAlphaFunc(GL_GREATER, 0.0f);
		glBindTexture(GL_TEXTURE_2D, bulletTexture[saTime]);
		glBegin(GL_QUADS);
		int w = bulletImage[saTime]->width;
		glTexCoord2f(0.0f, 0.0f); glVertex2f(-w, w);
		glTexCoord2f(1.0f, 0.0f); glVertex2f( w, w);
		glTexCoord2f(1.0f, 1.0f); glVertex2f( w, -w);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(-w, -w);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
		//glDisable(GL_ALPHA_TEST);
		glPopMatrix();	    
		
		if(timeDiff(&b->time, &timeCurrent) >= .6 && b->explode == 1)
		{
			Bullet *saveb = b->next;
			deleteBullet(b);
			b = saveb;
			continue;
		}		
		b = b->next;
  }
  
  Bullet *a = ahead;
  while(a) {
    double sa;
		int saTime;
		sa =timeDiff(&a->time,&timeCurrent);
		sa = sa*10;
		saTime = (int) sa;
		saTime = saTime%3;
				
		glPushMatrix();
		glTranslatef(a->pos[0],a->pos[1],a->pos[2]);
		glEnable(GL_ALPHA_TEST);
		//glAlphaFunc(GL_GREATER, 0.0f);
		glBindTexture(GL_TEXTURE_2D, enemyBulletTexture[saTime]);
		glBegin(GL_QUADS);
		int w = enemyBulletImage[saTime]->width;
		glTexCoord2f(0.0f, 0.0f); glVertex2f(-w, w);
		glTexCoord2f(1.0f, 0.0f); glVertex2f( w, w);
		glTexCoord2f(1.0f, 1.0f); glVertex2f( w, -w);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(-w, -w);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
		//glDisable(GL_ALPHA_TEST);
		glPopMatrix();	    
		a = a->next;
  }
}

void deleteBullet(Bullet *node)
{
	printf("Deleting bullet: %d", node->bulletCount);
  if((node->next == NULL) && (node->prev == NULL)) {
    bhead = NULL;
    free(node);
    node = NULL;
    printf(" -- only bullet\n");
  }

  else if((node->next != NULL) && (node->prev == NULL)) {
    Bullet *tmp = node->next;
    tmp->prev = NULL;
    bhead = node->next;
    free(node);
    node = NULL;
    printf(" -- first bullet\n");
  }

  else if((node->next == NULL) && (node->prev != NULL)) {
    Bullet *tmp = node->prev;
    tmp->next = NULL;
    free(node);
    node = NULL;
    printf(" -- last bullet\n");
  }

  else {
    Bullet *tmp1, *tmp2;
    tmp1 = node->next;
    tmp2 = node->prev;
    tmp1->prev = tmp2;
    tmp2->next = tmp1;
    free(node);
    node = NULL;
    printf(" -- middle bullet\n");
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

	
	
