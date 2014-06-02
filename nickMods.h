#ifndef _NICKMODS_H_
#define _NICKMODS_H_
typedef struct t_bullet Bullet;
typedef double Vec[3];

extern void shipShootBullet();
extern void updateShipBulletPosition();
extern void renderShipBullet();
extern void deleteBullet(Bullet *node);

#endif
