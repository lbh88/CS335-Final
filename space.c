//cs335 Spring 2014
//
//program: lab2_rainforest.c
//author:  Gordon Griesel
//date:    2014
//
//Must add code to remove a raindrop node from the linked-list
//when it stops or leaves the screen.
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
const float gravity = -0.05;
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
//void dispBG(void);
//void dispShip(void);

//-----------------------------------------------------------------------------
//Setup timers
const double physicsRate = 1.0 / 30.0;
const double oobillion = 1.0 / 1e9;
struct timespec timeStart, timeCurrent;
struct timespec timePause;
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

Ppmimage *shipImage=NULL;
Ppmimage *backgroundImage=NULL;
GLuint shipTexture;
GLuint backgroundTexture;
int show_rain=0;
#ifdef USE_SOUND
int play_sounds = 0;
#endif //USE_SOUND
//

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
#define SHIP_FLAT  0
#define SHIP_ROUND 1
typedef struct t_ship {
  int shape;
  Vec pos;
  Vec lastpos;
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
int play_music = 0;

int main(void)
{
  logOpen();
  initXWindows();
  init_opengl();
  init();
  init_sounds();
  clock_gettime(CLOCK_REALTIME, &timePause);
  clock_gettime(CLOCK_REALTIME, &timeStart);
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
    render();
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
  XStoreName(dpy, win, "CS335 - Space");
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
  shipImage    = ppm6GetImage("./images/spaceship.ppm");
  backgroundImage  = ppm6GetImage("./images/stars.ppm");
  //
  //create opengl texture elements
  glGenTextures(1, &shipTexture);
  glGenTextures(1, &backgroundTexture);
  //-------------------------------------------------------------------------
  //
  //ship
  //
  glBindTexture(GL_TEXTURE_2D, shipTexture);
  //
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, shipImage->width, shipImage->height , 0, GL_RGB, GL_UNSIGNED_BYTE,shipImage->data);
  //
  //-------------------------------------------------------------------------
  //
  //Background
  glBindTexture(GL_TEXTURE_2D, backgroundTexture);
  //
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, backgroundImage->width, backgroundImage->height, 0, GL_RGB, GL_UNSIGNED_BYTE, backgroundImage->data);
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

void init_sounds(void)
{
#ifdef USE_SOUND
  //FMOD_RESULT result;
  if (fmod_init()) {
    printf("ERROR - fmod_init()\n\n");
    return;
  }
  if (fmod_createsound("./sounds/burn1.wav", 0)) {
    printf("ERROR - fmod_createsound()\n\n");
    return;
  }
  if (fmod_createsound("./sounds/burn2.wav", 1)) {
    printf("ERROR - fmod_createsound()\n\n");
    return;
  }
  fmod_setmode(0,FMOD_LOOP_OFF);
  //fmod_playsound(0);
  //fmod_playsound(1);
  //fmod_systemupdate();
#endif //USE_SOUND
}

void init() {
#ifdef USE_SHIP
  ship.pos[0] = 320.0;
  ship.pos[1] = (double)(yres-400);
  VecCopy(ship.pos, ship.lastpos);
  ship.width = 50.0;
  ship.width2 = ship.width * 0.5;
  ship.radius = (float)ship.width2;
  ship.shape = 1; //SHIP_FLAT;
#endif //USE_SHIP
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
  //keyboard input?
  static int shift=0;
  int key = XLookupKeysym(&e->xkey, 0);
  if (e->type == KeyRelease) {
    if (key == XK_Shift_L || key == XK_Shift_R)
      shift=0;
    return;
  }
  if (e->type == KeyPress) {
    if (key == XK_Shift_L || key == XK_Shift_R) {
      shift=1;
      return;
    }
  } else {
    return;
  }
  //
  switch(key) {
    case XK_u:
      show_ship ^= 1;
      break;
	case XK_b:
	  bg ^=1;
	  break;
    case XK_p:
      ship.shape ^= 1;
      break;
      /***  THIS IS A GOOD PLACE FOR A SHIP UPGRADE ***/
    case XK_r:
      show_rain ^= 1;
      break;
    case XK_Left:
      VecCopy(ship.pos, ship.lastpos);
      ship.pos[0] = ( ship.pos[0] <= 0 ? ship.pos[0]: ship.pos[0]-10.0);
      break;
    case XK_Right:
      VecCopy(ship.pos, ship.lastpos);
      ship.pos[0] = ( ship.pos[0] >= xres ? ship.pos[0]: ship.pos[0]+10.0);
      break;
    /*case XK_Up:
      VecCopy(ship.pos, ship.lastpos);
      ship.pos[1] += 10.0;
      break;
    case XK_Down:
      VecCopy(ship.pos, ship.lastpos);
      ship.pos[1] -= 10.0;
      break;*/
    case XK_equal:
      if (++ndrops > 60)
        ndrops=60;
      break;
    case XK_minus:
      if (--ndrops < 0)
        ndrops = 0;
      break;
    case XK_s:
      play_sounds ^= 1;
      if (play_sounds) {
		    //fmod_playsound(0);
		    fmod_playsound(1);
      }
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

void cleanup_raindrops(void)
{
  Raindrop *s;
  while(ihead) {
    s = ihead->next;
    free(ihead);
    ihead = s;
  }
  ihead=NULL;
}

void delete_rain(Raindrop *node)
{
  if((node->next == NULL) && (node->prev == NULL)) {
    ihead = NULL;
    free(node);
    node = NULL;
  }

  else if((node->next != NULL) && (node->prev == NULL)) {
    Raindrop *tmp = node->next;
    tmp->prev = NULL;
    ihead = node->next;
    free(node);
    node = NULL;
  }

  else if((node->next == NULL) && (node->prev != NULL)) {
    Raindrop *tmp = node->prev;
    tmp->next = NULL;
    free(node);
    node = NULL;
  }

  else {
    Raindrop *tmp1, *tmp2;
    tmp1 = node->next;
    tmp2 = node->prev;
    tmp1->prev = tmp2;
    tmp2->next = tmp1;
    free(node);
    node = NULL;
  }
  //remove the node from the linked list
}

void create_raindrop(const int n)
{
  //create new rain drops...
  int i;
  for (i=0; i<n; i++) {
    Raindrop *node = (Raindrop *)malloc(sizeof(Raindrop));
    if (node == NULL) {
      Log("error allocating node.\n");
      exit(EXIT_FAILURE);
    }
    node->prev = NULL;
    node->next = NULL;
    node->sound=0;
    node->pos[0] = rnd() * (float)xres;
    node->pos[1] = rnd() * 100.0f + (float)yres;
    VecCopy(node->pos, node->lastpos);
    node->vel[0] = 
      node->vel[1] = 0.0f;
    node->color[0] = rnd() * 0.05f + 0.95f;
    node->color[1] = rnd() * 0.05f + 0.95f;
    node->color[2] = rnd() * 0.05f + 0.95f;
    node->color[3] = rnd() * 0.2f + 0.3f; //alpha
    node->linewidth = random(8)+1;
    //larger linewidth = faster speed
    node->maxvel[1] = (float)(node->linewidth*16);
    node->length = node->maxvel[1] * 0.2f + rnd();
    //
    node->lower_boundry = rnd() * 100.0;
    //
    //put raindrop into linked list
    node->next = ihead;
    if (ihead != NULL)
      ihead->prev = node;
    ihead = node;
    ++totrain;
  }
}

void check_raindrops()
{
  if (random(100) < 50) {
    create_raindrop(ndrops);
  }
  //
  //move rain droplets
  Raindrop *node = ihead;
  while(node) {
    //force is toward the ground
    node->vel[1] += gravity;
    VecCopy(node->pos, node->lastpos);
    if (node->pos[1] > node->lower_boundry) {
      node->pos[0] += node->vel[0] * timeslice;
      node->pos[1] += node->vel[1] * timeslice;
    }
    if (node->pos[1] <= node->lower_boundry) {
      Raindrop *savenode = node->next;
      delete_rain(node);
      node = savenode;
      continue;
    }
    if (fabs(node->vel[1]) > node->maxvel[1])
      node->vel[1] *= 0.96;
    node->vel[0] *= 0.999;
    //
    node = node->next;
  }
  //}
  //
  //check rain droplets
int n=0;
node = ihead;
while(node) {
  n++;
#ifdef USE_SOUND
  if (node->pos[1] < 0.0f) {
    //raindrop hit ground
    if (!node->sound && play_sounds) {
      //small chance that a sound will play
      int r = random(50);
      if (r==1)
        fmod_playsound(0);
      //if (r==2)
      //	fmod_playsound(1);
      //sound plays once per raindrop
      node->sound=1;
    }
  }
#endif //USE_SOUND
#ifdef USE_SHIP
  //collision detection for raindrop on ship
  if (show_ship) {
    if (ship.shape == SHIP_FLAT) {
      if (node->pos[0] >= (ship.pos[0] - ship.width2) &&
          node->pos[0] <= (ship.pos[0] + ship.width2)) {
        if (node->lastpos[1] > ship.lastpos[1] ||
            node->lastpos[1] > ship.pos[1]) {
          if (node->pos[1] <= ship.pos[1] ||
              node->pos[1] <= ship.lastpos[1]) {
            if (node->linewidth > 1) {
              Raindrop *savenode = node->next;
              delete_rain(node);
              node = savenode;
              continue;
            }
          }
        }
      }
    }
    if (ship.shape == SHIP_ROUND) {
      float d0 = node->pos[0] - ship.pos[0];
      float d1 = node->pos[1] - ship.pos[1];
      float distance = sqrt((d0*d0)+(d1*d1));
      //Log("distance: %f  ship.radius: %f\n",
      //							distance,ship.radius);
      if (distance <= ship.radius &&
          node->pos[1] > ship.pos[1]) {
        if (node->linewidth > 1) {
          if (deflection) {
            //deflect raindrop
            double dot;
            Vec v, up = {0,1,0};
            VecSub(node->pos, ship.pos, v);
            normalize(v);
            node->pos[0] =
              ship.pos[0] + v[0] * ship.radius;
            node->pos[1] =
              ship.pos[1] + v[1] * ship.radius;
            dot = VecDot(v,up);
            dot += 1.0;
            node->vel[0] += v[0] * dot * 1.0;
            node->vel[1] += v[1] * dot * 1.0;
          } else {
            Raindrop *savenode = node->next;
            delete_rain(node);
            node = savenode;
            continue;
          }
        }
      }
    }
    //VecCopy(ship.pos, ship.lastpos);
  }
#endif //USE_SHIP
  if (node->pos[1] < -20.0f) {
    //rain drop is below the visible area
    Raindrop *savenode = node->next;
    delete_rain(node);
    node = savenode;
    continue;
  }
  //if (node->next == NULL) break;
  node = node->next;
}
if (maxrain < n)
  maxrain = n;
  //}
  }

void physics(void)
{
  if (show_rain)
    check_raindrops();
}

#ifdef USE_SHIP
void draw_ship(void)
{
  //Log("draw_ship()...\n");
  if (ship.shape == SHIP_FLAT) {
    glColor4f(1.0f, 0.2f, 0.2f, 0.5f);
    glLineWidth(40);
    glBegin(GL_LINES);
    glVertex2f(ship.pos[0]-ship.width2, ship.pos[1]);
    glVertex2f(ship.pos[0]+ship.width2, ship.pos[1]);
    glEnd();
    glLineWidth(1);
  } else {
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    glPushMatrix();
    glTranslatef(ship.pos[0],ship.pos[1],ship.pos[2]);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);
    glBindTexture(GL_TEXTURE_2D, shipTexture);
    glBegin(GL_QUADS);
    float w = ship.width2;
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-w,  w);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( w,  w);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( w, -w);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-w, -w);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_ALPHA_TEST);
    glPopMatrix();
  }
}
#endif //USE_SHIP

void draw_raindrops(void)
{
  //if (ihead) {
  Raindrop *node = ihead;
  while(node) {
    glPushMatrix();
    glTranslated(node->pos[0],node->pos[1],node->pos[2]);
    glColor4fv(node->color);
    glLineWidth(node->linewidth);
    glBegin(GL_LINES);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(0.0f, node->length);
    glEnd();
    glPopMatrix();
    node = node->next;
  }
  //}
  glLineWidth(1);
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
//  float wid = 120.0f;
  glColor3f(1.0, 1.0, 1.0);
  if (bg) {
	dispBG(backgroundTexture);
  }
  if (show_ship) {
	dispShip(ship, shipTexture);
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
  if (show_rain)
    draw_raindrops();
  glDisable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
  //
#ifdef USE_SHIP
  if (show_ship)
    draw_ship();
#endif //USE_SHIP
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
}

