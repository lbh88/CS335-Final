#ifndef _LAMMODS_H_
#define _LAMMODS_H_
typedef struct t_ship Ship;
typedef double Vec[3];
typedef struct t_stats Stats;

extern void dispShip(Ship ship, GLuint shipTexture);
extern void dispBG(GLuint shipTexture);
extern void buildShipImage();
extern void intro();
extern void dispIntro();
extern void endIntroSong();
extern void initStats();

#endif
