#ifndef _LAMMODS_H_
#define _LAMMODS_H_
typedef struct t_ship Ship;
typedef double Vec[3];

extern void dispShip(Ship ship, GLuint shipTexture);
extern void dispBG(GLuint shipTexture);

#endif
