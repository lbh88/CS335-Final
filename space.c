//Lam Ha & Nicholas Jones
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
#include <FMOD/wincompat.h>
#include "fmod.h"
#endif //USE_SOUND

//defined types
typedef double Flt;
typedef double Vec[3];
typedef Flt	Matrix[4][4];

//macros
#define rnd() (((double)rand())/(double)RAND_MAX)
#define random(a) (rand()%a)
#define MakeVector(x, y, z, v) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define VecCopy(a,b) (b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2]
#define VecDot(a,b)	((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VecSub(a,b,c) (c)[0]=(a)[0]-(b)[0]; \
                             (c)[1]=(a)[1]-(b)[1]; \
(c)[2]=(a)[2]-(b)[2]
//constants
const float timeslice = 1.0f;
const float gravity = -0.025f;
#define PI 3.141592653589793
#define ALPHA 1

//X Windows variables
Display *dpy;
Window win;
GLXContext glc;

//function prototypes
void initXWindows(void);
void init_opengl(void);
void cleanupXWindows(void);
void check_resize(XEvent *e);
void check_mouse(XEvent *e);
void check_keys(XEvent *e);
void init();
void init_sounds(void);
void physics(void);
void render(void);
void checkUpgrades(void);
void updateEnemyBulletPos(void);
//void dispBG(void);
//void dispShip(void);

//-----------------------------------------------------------------------------
//Setup timers
const double physicsRate = 1.0 / 60.0;
const double oobillion = 1.0 / 1e9;
struct timespec timeStart, timeCurrent;
struct timespec timePause;
struct timespec shipAnimation;
double physicsCountdown=0.0;
double timeSpan=0.0;
double timeDiff(struct timespec *start, struct timespec *end) {
  return (double)(end->tv_sec - start->tv_sec ) +
    (double)(end->tv_nsec - start->tv_nsec) * oobillion;
}
void timeCopy(struct timespec *dest, struct timespec *source) {
  memcpy(dest, source, sizeof(struct timespec));
}
//-----------------------------------------------------------------------------

int done=0;
int xres=800, yres=600;
int kills = 0;
int keys[65536];
int dead = 0;
double deathTime;

Ppmimage *enemyImage=NULL;
GLuint enemyTexture;
Ppmimage *shipImage[10];
Ppmimage *backgroundImage=NULL;
GLuint shipTexture[10];
GLuint backgroundTexture;
GLuint backgroundTexture2;
GLuint silhouetteTexture;
int show_rain=0;
#ifdef USE_SOUND
int play_sounds = 0;
#endif //USE_SOUND
//

typedef struct t_stats {
	int health;
	int moveSpeed;
	int fireSpeed;
	int damage;
	int upgrades[10];
} Stats;
Stats stats;

typedef struct t_raindrop {
  int type;
  int linewidth;
  int sound;
  Vec pos;
  Vec lastpos;
  Vec vel;
  Vec maxvel;
  Vec force;
  Flt lower_boundry;
  float length;
  float color[4];
  struct t_raindrop *prev;
  struct t_raindrop *next;
} Raindrop;
Raindrop *ihead=NULL;
int ndrops=1;
int totrain=0;
int maxrain=0;	
void delete_rain(Raindrop *node);
void cleanup_raindrops(void);
//
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
int deflection=0;
#endif //USE_SHIP
int bg = 0;
int show_ship = 0;
int disp_intro = 0;

typedef struct t_enemy {
  int shape;
  int fireSpeed;
  int moveSpeed;
  GLuint enemyTexture;
  Vec pos;
  Vec lastpos;
  Vec vel;
  float angle;
  float width;
  float width2;
  float radius;
} Enemy;
Enemy enemy;
int show_enemy = 0;
void buildEnemyImage();
void dispEnemy(Enemy enemy, GLuint enemyTexture); 
void checkMovement();

typedef struct t_bullet {
  Vec pos;
  Vec vel;
  float color[3];
  struct timespec time;
  struct t_bullet *prev;
  struct t_bullet *next;
} Bullet;
Bullet *bhead=NULL;
void deleteBullet(Bullet *node);
struct timespec bulletTimer;
struct timespec deathTimer;

int main(void)
{
  logOpen();
  initXWindows();
  init_opengl();
  init();
  gameOver();
  intro();
  clock_gettime(CLOCK_REALTIME, &timePause);
  clock_gettime(CLOCK_REALTIME, &timeStart);
  clock_gettime(CLOCK_REALTIME, &shipAnimation);
  while(!done) {
    while(XPending(dpy)) {
      XEvent e;
      XNextEvent(dpy, &e);
      check_resize(&e);
      check_mouse(&e);
      check_keys(&e);
    }
    clock_gettime(CLOCK_REALTIME, &timeCurrent);
    timeSpan = timeDiff(&timeStart, &timeCurrent);
    timeCopy(&timeStart, &timeCurrent);
    physicsCountdown += timeSpan;
    while(physicsCountdown >= physicsRate) {
      physics();
      physicsCountdown -= physicsRate;
    }
    if (disp_intro) 
    {
		if(!dead)
			render();
		else
			dispGameOver();
	}
	else
	{
		 dispIntro();
	}
    glXSwapBuffers(dpy, win);
  }
  cleanupXWindows();
  cleanup_fonts();
#ifdef USE_SOUND
  fmod_cleanup();
#endif //USE_SOUND
  logClose();
  return 0;
}

void cleanupXWindows(void)
{
  XDestroyWindow(dpy, win);
  XCloseDisplay(dpy);
}

void set_title(void)
{
  //Set the window title bar.
  XMapWindow(dpy, win);
  XStoreName(dpy, win, "Space Raid");
}

void setup_screen_res(const int w, const int h)
{
  xres = w;
  yres = h;
}

void initXWindows(void)
{
  Window root;
  GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
  //GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
  XVisualInfo *vi;
  Colormap cmap;
  XSetWindowAttributes swa;

  setup_screen_res(640, 480);
  dpy = XOpenDisplay(NULL);
  if(dpy == NULL) {
    printf("\n\tcannot connect to X server\n\n");
    exit(EXIT_FAILURE);
  }
  root = DefaultRootWindow(dpy);
  vi = glXChooseVisual(dpy, 0, att);
  if(vi == NULL) {
    printf("\n\tno appropriate visual found\n\n");
    exit(EXIT_FAILURE);
  } 
  //else {
  //	// %p creates hexadecimal output like in glxinfo
  //	printf("\n\tvisual %p selected\n", (void *)vi->visualid);
  //}
  cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
  swa.colormap = cmap;
  swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
    StructureNotifyMask | SubstructureNotifyMask;
  win = XCreateWindow(dpy, root, 0, 0, xres, yres, 0,
      vi->depth, InputOutput, vi->visual,
      CWColormap | CWEventMask, &swa);
  set_title();
  glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
  glXMakeCurrent(dpy, win, glc);
}

void reshape_window(int width, int height)
{
  //window has been resized.
  setup_screen_res(width, height);
  //
  glViewport(0, 0, (GLint)width, (GLint)height);
  glMatrixMode(GL_PROJECTION); glLoadIdentity();
  glMatrixMode(GL_MODELVIEW); glLoadIdentity();
  glOrtho(0, xres, 0, yres, -1, 1);
  set_title();
}

unsigned char *buildAlphaData(Ppmimage *img)
{
  //add 4th component to RGB stream...
  int i;
  int a,b,c;
  unsigned char *newdata, *ptr;
  unsigned char *data = (unsigned char *)img->data;
  newdata = (unsigned char *)malloc(img->width * img->height * 4);
  ptr = newdata;
  for (i=0; i<img->width * img->height * 3; i+=3) {
    a = *(data+0);
    b = *(data+1);
    c = *(data+2);
    *(ptr+0) = a;
    *(ptr+1) = b;
    *(ptr+2) = c;
    //
    //get the alpha value
    //
    //original code
    //get largest color component...
    //*(ptr+3) = (unsigned char)((
    //		(int)*(ptr+0) +
    //		(int)*(ptr+1) +
    //		(int)*(ptr+2)) / 3);
    //d = a;
    //if (b >= a && b >= c) d = b;
    //if (c >= a && c >= b) d = c;
    //*(ptr+3) = d;
    //
    //new code, suggested by Chris Smith, Fall 2013
    *(ptr+3) = (a|b|c);
    //
    ptr += 4;
    data += 3;
  }
  return newdata;
}

void init_opengl(void)
{
  //OpenGL initialization
  glViewport(0, 0, xres, yres);
  //Initialize matrices
  glMatrixMode(GL_PROJECTION); glLoadIdentity();
  glMatrixMode(GL_MODELVIEW); glLoadIdentity();
  //This sets 2D mode (no perspective)
  glOrtho(0, xres, 0, yres, -1, 1);

  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_FOG);
  glDisable(GL_CULL_FACE);

  //Clear the screen
  glClearColor(1.0, 1.0, 1.0, 1.0);
  //glClear(GL_COLOR_BUFFER_BIT);
  //Do this to allow fonts
  glEnable(GL_TEXTURE_2D);
  initialize_fonts();
  //
  //load the images file into a ppm structure.
  //
  //shipImage		= ppm6GetImage("./images/spaceship.ppm");
  backgroundImage	= ppm6GetImage("./images/stars.ppm");
  //
  //create opengl texture elements
  //glGenTextures(1, &shipTexture);
  glGenTextures(1, &backgroundTexture);
  glBindTexture(GL_TEXTURE_2D, backgroundTexture);
  //
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, backgroundImage->width, backgroundImage->height, 0, GL_RGB, GL_UNSIGNED_BYTE, backgroundImage->data);
  
  backgroundImage	= ppm6GetImage("./images/stars.ppm");
  glGenTextures(1, &backgroundTexture2);
  glBindTexture(GL_TEXTURE_2D, backgroundTexture2);
  //
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, backgroundImage->width, backgroundImage->height, 0, GL_RGB, GL_UNSIGNED_BYTE, backgroundImage->data);
  
  buildShipImage();
  buildEnemyImage();
}

void check_resize(XEvent *e)
{
  //The ConfigureNotify is sent by the
  //server if the window is resized.
  if (e->type != ConfigureNotify)
    return;
  XConfigureEvent xce = e->xconfigure;
  if (xce.width != xres || xce.height != yres) {
    //Window size did change.
    reshape_window(xce.width, xce.height);
  }
}


void init() {
	initStats();
#ifdef USE_SHIP
	ship.pos[0] = 320.0;
	ship.pos[1] = (double)(yres-400);
	VecCopy(ship.pos, ship.lastpos);
	ship.width = 50.0;
	ship.width2 = ship.width * 0.5;
	ship.radius = (float)ship.width2;
	ship.shape = 1; //SHIP_FLAT;
#endif //USE_SHIP
	clock_gettime(CLOCK_REALTIME, &bulletTimer);
	memset(keys, 0, 65536);
	initEnemy();
//	initBullet();
}

void check_mouse(XEvent *e)
{
  //Did the mouse move?
  //Was a mouse button clicked?
  static int savex = 0;
  static int savey = 0;
  //
  if (e->type == ButtonRelease) {
    return;
  }
  if (e->type == ButtonPress) {
    if (e->xbutton.button==1) {
      //Left button is down
    }
    if (e->xbutton.button==3) {
      //Right button is down
    }
  }
  if (savex != e->xbutton.x || savey != e->xbutton.y) {
    //Mouse moved
    savex = e->xbutton.x;
    savey = e->xbutton.y;
  }
}

void check_keys(XEvent *e)
{
  //struct timespec bt;
  //keyboard input?
  static int shift=0;
	  int key = XLookupKeysym(&e->xkey, 0);
	//Log("key: %i\n", key);
	if (e->type == KeyRelease) {
		keys[key]=0;
		if (key == XK_Shift_L || key == XK_Shift_R)
			shift=0;
		return;
	}
	if (e->type == KeyPress) {
		keys[key]=1;
		if (key == XK_Shift_L || key == XK_Shift_R) {
			shift=1;
			return;
		}
	} else {
		return;
	}
	  //
	  switch(key) {
		case XK_c:
		  if (dead){
			  restartGame();
		  }
		break;
		  
		case XK_u:
		  show_ship ^= 1;
		  break;
		case XK_b:
		  bg ^=1;
		  break;
		case XK_p:
		  ship.shape ^= 1;
		  break;
		case XK_r:
		  show_rain ^= 1;
		  break;
		case XK_e:
		  show_enemy ^= 1;
		  break; 
		case XK_equal:
		  if (++ndrops > 60)
			ndrops=60;
		  break;
		case XK_minus:
		  if (--ndrops < 0)
			ndrops = 0;
		  break;
		case XK_s:
		  if (!disp_intro)
		  {
			fmod_cleanupIntro(0);
			disp_intro ^= 1;
			show_ship ^= 1;
			bg ^= 1;
			init_sounds();
	#ifdef USE_SOUND 
			//fmod_playsound(0);
			fmod_playsound(0);
	#endif //USE_SOUND
		  }
		  play_sounds ^= 1;
		  break;
		case XK_w:
		  if (shift) {
			//shrink the ship
			ship.width *= (1.0 / 1.05);
		  } else {
			//enlarge the ship
			ship.width *= 1.05;
		  }
		  //half the width
		  ship.width2 = ship.width * 0.5;
		  ship.radius = (float)ship.width2;
		  break;
		case XK_Escape:
		  done=1;
		  break;
	  }
}

void normalize(Vec vec)
{
  Flt xlen = vec[0];
  Flt ylen = vec[1];
  Flt zlen = vec[2];
  Flt len = xlen*xlen + ylen*ylen + zlen*zlen;
  if (len == 0.0) {
    MakeVector(0.0,0.0,1.0,vec);
    return;
  }
  len = 1.0 / sqrt(len);
  vec[0] = xlen * len;
  vec[1] = ylen * len;
  vec[2] = zlen * len;
}


void physics(void)
{
	//Update ship position

  if (show_enemy) {
    check_enemies();
  }
  //Update ship position
  if(stats.health > 0 && disp_intro)
	checkMovement();
  //Check for collision with window edges
  if (ship.pos[0] < 0.0) {
    ship.pos[0] += (Flt)xres;
  }
  else if (ship.pos[0] > (Flt)xres) {
    ship.pos[0] -= (Flt)xres;
  }
  else if (ship.pos[1] < 0.0) {
    ship.pos[1] += (Flt)yres;
  }
  else if (ship.pos[1] > (Flt)yres) {
    ship.pos[1] -= (Flt)yres;
  }
  //
  //
  //Update bullet positions
  updateBulletPos();
  updateEnemyBulletPos();
  
}

void render(void)
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
  if (bg) {
    dispBG();
  }
  
  if (show_enemy) {
//    dispEnemy(enemy, enemyTexture);
  }

  glDisable(GL_TEXTURE_2D);
  //glColor3f(1.0f, 0.0f, 0.0f);
  //glBegin(GL_QUADS);
  //	glVertex2i(10,10);
  //	glVertex2i(10,60);
  //	glVertex2i(60,60);
  //	glVertex2i(60,10);
  //glEnd();
  //return;
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  
  glDisable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
  //
#ifdef USE_SHIP
  if (show_ship)
    draw_ship();
#endif //USE_SHIP
  if (show_enemy)
      draw_enemy();
  glBindTexture(GL_TEXTURE_2D, 0);
  //
  //
  r.bot = yres - 20;
  r.left = 10;
  r.center = 0;
  unsigned int cref = 0x00ffffff;
  ggprint8b(&r, 16, cref, "U - Display Unit");
  ggprint8b(&r, 16, cref, "B - Display Background");
  ggprint8b(&r, 16, cref, "S - Play music");
  ggprint8b(&r, 16, cref, "Space - Fire");
  
  r.bot = yres - 20;
  r.left = xres-100;
  r.center = 0;
  ggprint8b(&r, 16, cref, "Health: %d", stats.health);
  ggprint8b(&r, 16, cref, "Speed: %d", stats.moveSpeed);
  ggprint8b(&r, 16, cref, "Fire Rate: %d", stats.fireSpeed);
  ggprint8b(&r, 16, cref, "Kills: %d", kills);

  initBullet();
}
