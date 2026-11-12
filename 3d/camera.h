#ifndef CAMERA_H
#define CAMERA_H
#include "math3d.h"

extern Vec3   gEye;
extern Vec3   gCenter;
extern Vec3   gUp;
extern double gD;
extern int    gPersp;

Matrix viewMatrix(void);
int    projectPoint(Vec3 v, double* xp, double* yp);
#endif
