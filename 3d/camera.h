#ifndef CAMERA_H
#define CAMERA_H
#include "math3d.h"

extern Vec3   gEye;
extern Vec3   gCenter;
extern Vec3   gUp;
extern double gD;
extern double gNear;
extern int    gPersp;
extern double gYaw, gPitch, gDist;   /* spherical camera state (orbit fixed origin) */

Matrix viewMatrix(void);
int    projectPoint(Vec3 v, double* xp, double* yp);
void   cameraUpdate(void);            /* recompute gEye/gUp from yaw/pitch/dist */
#endif
