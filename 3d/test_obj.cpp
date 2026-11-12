/* objload smoke test: load the real file, print counts and bbox */
#include <stdio.h>
#include "math3d.h"
#include "objload.h"

int main(void)
{
    Vec3* v = 0;  int* f = 0;  int nv = 0, nf = 0;
    const char* path = "C:/Users/21505/Desktop/ComputerGraphics/lily-impeller.obj";

    if (!loadObj(path, 2.0, &v, &nv, &f, &nf)) {
        printf("loadObj FAILED\n");
        return 1;
    }
    printf("loaded: nv=%d  nf=%d (expect 20465 / 32315)\n", nv, nf);

    Vec3 mn = v[0], mx = v[0];
    for (int i = 1; i < nv; i++) {
        if (v[i].x < mn.x) mn.x = v[i].x;  if (v[i].x > mx.x) mx.x = v[i].x;
        if (v[i].y < mn.y) mn.y = v[i].y;  if (v[i].y > mx.y) mx.y = v[i].y;
        if (v[i].z < mn.z) mn.z = v[i].z;  if (v[i].z > mx.z) mx.z = v[i].z;
    }
    printf("normalized bbox:\n");
    printf("  x: %.4g .. %.4g (centre %.4g)\n", mn.x, mx.x, (mn.x + mx.x) / 2);
    printf("  y: %.4g .. %.4g (centre %.4g)\n", mn.y, mx.y, (mn.y + mx.y) / 2);
    printf("  z: %.4g .. %.4g (centre %.4g)\n", mn.z, mx.z, (mn.z + mx.z) / 2);

    /* index range check */
    int lo = 0, hi = 0;
    for (int i = 0; i < 3 * nf; i++) {
        if (f[i] < lo) lo = f[i];
        if (f[i] > hi) hi = f[i];
    }
    printf("index range: %d .. %d (must be 0 .. %d)\n", lo, hi, nv - 1);

    /* first 3 faces */
    for (int t = 0; t < 3 && t < nf; t++)
        printf("  face %d: %d %d %d\n", t, f[3*t], f[3*t+1], f[3*t+2]);

    freeObjMesh(v, f);
    return 0;
}
