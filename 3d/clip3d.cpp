#include "clip3d.h"
#include "camera.h"     /* gD, gPersp */
#include "raster.h"     /* gWinX, gWinY */



#define GFAR    1000.0
#define MAXVTX  24    
typedef struct { double a, b, c, e; } Plane;

static int buildPlanes(Plane* pl)
{
    double d = gD, wx = gWinX, wy = gWinY;
    int n = 0;
    if (gPersp) {
        pl[n].a = -d; pl[n].b = 0;  pl[n].c = -wx; pl[n].e = 0; n++; 
        pl[n].a =  d; pl[n].b = 0;  pl[n].c = -wx; pl[n].e = 0; n++; 
        pl[n].a = 0;  pl[n].b = -d; pl[n].c = -wy; pl[n].e = 0; n++; 
        pl[n].a = 0;  pl[n].b =  d; pl[n].c = -wy; pl[n].e = 0; n++; 
    } else {
        pl[n].a = 1;  pl[n].b = 0;  pl[n].c = 0; pl[n].e = wx; n++;  
        pl[n].a = -1; pl[n].b = 0;  pl[n].c = 0; pl[n].e = wx; n++;  
        pl[n].a = 0;  pl[n].b = 1;  pl[n].c = 0; pl[n].e = wy; n++;  
        pl[n].a = 0;  pl[n].b = -1; pl[n].c = 0; pl[n].e = wy; n++;  
    }
    pl[n].a = 0; pl[n].b = 0; pl[n].c = -1; pl[n].e = d;    n++;   
    pl[n].a = 0; pl[n].b = 0; pl[n].c =  1; pl[n].e = GFAR; n++;    
    return n;
}

static double peval(const Plane* p, Vec3 v)
{
    return p->a * v.x + p->b * v.y + p->c * v.z + p->e;
}

int clipFrustum(const Vec3* in, int n, Vec3* out, int outCap)
{
    Vec3 bufA[MAXVTX], bufB[MAXVTX];
    Vec3* src = bufA;
    Vec3* dst = bufB;
    Plane pl[6];
    int np = buildPlanes(pl);
    int m = (n > MAXVTX) ? MAXVTX : n;

    for (int i = 0; i < m; i++) src[i] = in[i];

    for (int p = 0; p < np && m > 0; p++) {
        int k = 0;
        for (int i = 0; i < m; i++) {
            Vec3 P = src[i];
            Vec3 Q = src[(i + 1) % m];
            double fp = peval(&pl[p], P);
            double fq = peval(&pl[p], Q);
            int inP = (fp >= 0.0), inQ = (fq >= 0.0);
            if (inP && k < MAXVTX) dst[k++] = P;
            if (inP != inQ && k < MAXVTX) {
                double t = fp / (fp - fq);
                Vec3 I;
                I.x = P.x + t * (Q.x - P.x);
                I.y = P.y + t * (Q.y - P.y);
                I.z = P.z + t * (Q.z - P.z);
                dst[k++] = I;
            }
        }
        { Vec3* tmp = src; src = dst; dst = tmp; }
        m = k;
    }

    if (m > outCap) m = outCap;
    for (int i = 0; i < m; i++) out[i] = src[i];
    return m;
}
