/* verify cube_torus.obj loads via the real objload.cpp */
#include <stdio.h>
#include "math3d.h"
#include "objload.h"

int main(void)
{
    Vec3* v = 0;  int* f = 0;
    int nv = 0, nf = 0;
    int ok = loadObj("C:/Users/21505/Desktop/ComputerGraphics/cube_torus.obj",
                     2.0, &v, &nv, &f, &nf);
    if (!ok) { printf("LOAD FAILED\n"); return 1; }

    /* bbox after normalization */
    Vec3 mn = v[0], mx = v[0];
    for (int i = 1; i < nv; i++) {
        if (v[i].x < mn.x) mn.x = v[i].x; if (v[i].x > mx.x) mx.x = v[i].x;
        if (v[i].y < mn.y) mn.y = v[i].y; if (v[i].y > mx.y) mx.y = v[i].y;
        if (v[i].z < mn.z) mn.z = v[i].z; if (v[i].z > mx.z) mx.z = v[i].z;
    }
    /* check index range */
    int mnidx = f[0], mxidx = f[0];
    for (int i = 0; i < nf * 3; i++) {
        if (f[i] < mnidx) mnidx = f[i];
        if (f[i] > mxidx) mxidx = f[i];
    }
    printf("loaded: nv=%d nf=%d\n", nv, nf);
    printf("bbox: x [%.3f, %.3f]  y [%.3f, %.3f]  z [%.3f, %.3f]\n",
           mn.x, mx.x, mn.y, mx.y, mn.z, mx.z);
    printf("index range: %d .. %d (expect 0 .. %d)\n", mnidx, mxidx, nv - 1);
    int bad = (mnidx < 0 || mxidx >= nv) ? 1 : 0;
    printf(bad ? "INDEX ERROR\n" : "OK\n");
    freeObjMesh(v, f);
    return bad;
}
