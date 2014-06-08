#ifndef _LAMMODS_H_
#define _LAMMODS_H_
typedef struct t_ship Ship;
typedef double Vec[3];
typedef struct t_stats Stats;

extern void dispShip();
extern void dispBG();
extern void buildShipImage();
extern void intro();
extern void dispIntro();
extern void endIntroSong();
extern void initStats();
extern void checkMovement();
extern void checkUpgrades();
extern void enemyMovement();
extern void gameOver();
extern void restartGame();
extern void dispGameOver();
extern void checkDeath();
extern void draw_ship();

#endif
