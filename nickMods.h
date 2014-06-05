#ifndef _NICKMODS_H_
#define _NICKMODS_H_
typedef struct t_bullet Bullet;
typedef struct t_enemy Enemy;
typedef double Vec[3];


extern void shipShootBullet();
extern void updateBulletPos();
extern void check_enemies();
extern void draw_enemy();
extern void initEnemy();
extern void initBullet();
extern void deleteBullet(Bullet *node);
extern void buildEnemyImage();
extern void displayEnemy(Enemy enemy, GLuint enemyTexture);

#endif
