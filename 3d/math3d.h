#pragma once
#ifndef MATH3D_H
#define MATH3D_H
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct { double x, y, z; } Vec3;
typedef struct { double data[4][4]; } Matrix;

Matrix identity(void);
Matrix matrixMut(Matrix A, Matrix B);
Matrix translation(double tx, double ty, double tz);
Matrix scaling(double sx, double sy, double sz);
Matrix rotationX(double deg);
Matrix rotationY(double deg);
Matrix rotationZ(double deg);
Vec3   applyMatrix(Matrix M, Vec3 p);

Vec3   vsub(Vec3 a, Vec3 b);
double vdot(Vec3 a, Vec3 b);
Vec3   vcross(Vec3 a, Vec3 b);
Vec3   vnorm(Vec3 a);
#endif
