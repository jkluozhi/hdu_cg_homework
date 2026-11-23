#include <stdio.h>
#include <math.h>
#include "curves.h"

static int fails = 0;
static void CK(const char* name, double got, double want)
{
    int ok = fabs(got - want) < 1e-9;
    if (!ok) fails++;
    printf("%-34s got=%.6f want=%.6f  %s\n", name, got, want, ok ? "OK" : "FAIL");
}

static Vec3 planeEval(double u, double v, void* aux)
{
    (void)aux;
    Vec3 p = { u, v, 0 };
    return p;
}

int main(void)
{
    /* ---- 1. 三次 Bezier 曲线 ---- */
    Vec3 bp[4] = { {0,0,0},{1,2,0},{3,2,0},{4,0,0} };
    Vec3 p0 = bezierPoint(bp, 3, 0.0);
    Vec3 p1 = bezierPoint(bp, 3, 1.0);
    Vec3 pm = bezierPoint(bp, 3, 0.5);
    CK("Bezier t=0 x", p0.x, 0.0);
    CK("Bezier t=0 y", p0.y, 0.0);
    CK("Bezier t=1 x", p1.x, 4.0);
    CK("Bezier t=1 y", p1.y, 0.0);
    /* 中点 = (P0+3P1+3P2+P3)/8 */
    CK("Bezier t=0.5 x", pm.x, 2.0);
    CK("Bezier t=0.5 y", pm.y, 1.5);

    /* ---- 2. 三次夹持 B 样条曲线（端点插值） ---- */
    Vec3 cp[7] = { {0,0,0},{1,2,0},{2,3,0},{3,2,0},{4,3,0},{5,1,0},{6,0,0} };
    double U[11] = { 0,0,0,0,0.25,0.5,0.75,1,1,1,1 };
    Vec3 b0 = bsplinePoint(cp, 6, U, 3, 0.0);
    Vec3 b1 = bsplinePoint(cp, 6, U, 3, 1.0);
    CK("Bspline u=0 x", b0.x, 0.0);
    CK("Bspline u=0 y", b0.y, 0.0);
    CK("Bspline u=1 x", b1.x, 6.0);
    CK("Bspline u=1 y", b1.y, 0.0);

    /* ---- 3. 双三次 Bezier 曲面（h11=4） ---- */
    Vec3 P[4][4];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            P[i][j].x = (double)i;
            P[i][j].y = (double)j;
            P[i][j].z = (i == 1 && j == 1) ? 4.0 : 0.0;
        }
    Vec3 s00 = bezierSurfacePoint(P, 0.0, 0.0);
    Vec3 s11 = bezierSurfacePoint(P, 1.0, 1.0);
    Vec3 sc  = bezierSurfacePoint(P, 0.5, 0.5);
    CK("BezSurf (0,0) x", s00.x, 0.0);
    CK("BezSurf (1,1) x", s11.x, 3.0);
    CK("BezSurf (1,1) y", s11.y, 3.0);
    /* 中心高度 = B1(0.5)^2 * 4 = 0.375^2 * 4 = 0.5625 */
    CK("BezSurf (0.5,0.5) z", sc.z, 0.5625);

    /* ---- 4. 双三次夹持 B 样条曲面（h22=4）端点插值 ---- */
    const double Us[9] = { 0,0,0,0,0.5,1,1,1,1 };
    const double Vs[9] = { 0,0,0,0,0.6,1,1,1,1 };
    Vec3 Q[5][5];
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++) {
            Q[i][j].x = (double)i;
            Q[i][j].y = (double)j;
            Q[i][j].z = (i == 2 && j == 2) ? 4.0 : 0.0;
        }
    Vec3 q00 = bsplineSurfacePoint(Q, Us, Vs, 3, 0.0, 0.0);
    Vec3 q11 = bsplineSurfacePoint(Q, Us, Vs, 3, 1.0, 1.0);
    Vec3 qc  = bsplineSurfacePoint(Q, Us, Vs, 3, 0.5, 0.5);
    CK("BsSurf (0,0) x", q00.x, 0.0);
    CK("BsSurf (0,0) y", q00.y, 0.0);
    CK("BsSurf (1,1) x", q11.x, 4.0);
    CK("BsSurf (1,1) y", q11.y, 4.0);
    printf("%-34s center z = %.6f (expect > 0)\n", "BsSurf (0.5,0.5) z", qc.z);
    if (!(qc.z > 0.0)) fails++;

    /* ---- 5. 网格生成计数 ---- */
    Vec3* V;  int* F;
    int nf = buildSurfaceMesh(planeEval, 0, 4, 4, &V, &F);
    printf("buildSurfaceMesh nf=%d (expect %d)\n", nf, 2 * 3 * 3);
    if (nf != 2 * 3 * 3) fails++;

    printf("\n%s  (%d failures)\n", fails == 0 ? "ALL PASS" : "SOME FAILED", fails);
    return fails;
}
