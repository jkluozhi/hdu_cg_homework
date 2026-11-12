#ifndef CLIP3D_H
#define CLIP3D_H
#include "math3d.h"

/* Clip a view-space polygon against the view frustum (Sutherland-Hodgman).
 * in  : input vertices, n of them (view space)
 * out : output buffer, capacity outCap
 * returns number of output vertices (0 = fully clipped away) */
int clipFrustum(const Vec3* in, int n, Vec3* out, int outCap);

#endif
