#define GLUT_DISABLE_ATEXIT_HACK

#include "GLUT.H"

#include "raster.h"

#include "camera.h"



double gScale = 100.0;

double gWinX = 4.0, gWinY = 3.0;   /* 投影窗口半宽/半高（世界单位） */

static double* gZbuf = 0; 
static int     gZbufCap = 0;

double gClipX0 = -500, gClipY0 = -500;

double gClipX1 = 500, gClipY1 = 500;
int w_width = 1000;
int w_height = 1000;





/* ================= 2D map ================= */

/* 投影平面坐标（世界单位）-> 屏幕像素（等比缩放，原点在窗口中心） */

void toPixels(double xp, double yp, double* sx, double* sy)

{

    *sx = xp * gScale;

    *sy = yp * gScale;

}



int iround(double v) { return (int)floor(v + 0.5); }



/* ================= Liang-Barsky 线段裁剪 ================= */


int clipLine(double* x0, double* y0, double* x1, double* y1)

{

    double t0 = 0.0, t1 = 1.0;

    double dx = *x1 - *x0;

    double dy = *y1 - *y0;

    double p[4] = { -dx,  dx, -dy,  dy };

    double q[4] = { *x0 - gClipX0, gClipX1 - *x0,

                    *y0 - gClipY0, gClipY1 - *y0 };



    for (int i = 0; i < 4; i++) {

        if (fabs(p[i]) < 1e-12) {

            if (q[i] < 0) return 0;       
            continue;

        }

        double t = q[i] / p[i];

        if (p[i] < 0) { if (t > t0) t0 = t; }

        else { if (t < t1) t1 = t; }

        if (t0 > t1) return 0;

    }



    double ox = *x0, oy = *y0;

    *x0 = ox + t0 * dx;  *y0 = oy + t0 * dy;

    *x1 = ox + t1 * dx;  *y1 = oy + t1 * dy;

    return 1;

}



/* ================= Bresenham 光栅化 ================= */

static int iabs(int v) { return v < 0 ? -v : v; }



void drawLine(int x0, int y0, int x1, int y1)

{

    int dx = iabs(x1 - x0), dy = iabs(y1 - y0);

    int sx = (x0 < x1) ? 1 : -1;

    int sy = (y0 < y1) ? 1 : -1;

    int err = dx - dy;



    glBegin(GL_POINTS);

    for (;;) {

        glVertex2i(x0, y0);

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;

        if (e2 > -dy) { err -= dy; x0 += sx; }

        if (e2 < dx) { err += dx; y0 += sy; }

    }

    glEnd();

}



/* ================= 一般线段：模型/世界 -> 屏幕像素 -> 裁剪 -> 光栅化 ================= */
static void emitSegment(Vec3 va, Vec3 vb)
{
    double xa, ya, xb, yb;

    if (!projectPoint(va, &xa, &ya)) return;            /* 端点不可投影则整段丢弃 */
    if (!projectPoint(vb, &xb, &yb)) return;

    toPixels(xa, ya, &xa, &ya);                          /* -> 屏幕像素 */
    toPixels(xb, yb, &xb, &yb);

    if (!clipLine(&xa, &ya, &xb, &yb)) return;         

    drawLine(iround(xa), iround(ya), iround(xb), iround(yb));
}

void drawSegment(Vec3 a, Vec3 b, Matrix V)
{
    /* model -> view -> 投影 -> 屏幕 -> 裁剪 -> 光栅化 */
    emitSegment(applyMatrix(V, applyMatrix(gModel, a)),
                applyMatrix(V, applyMatrix(gModel, b)));
}

/* 世界坐标线段：跳过模型变换，只有 view -> 投影 -> 光栅化（坐标轴用） */
void drawWorldSegment(Vec3 a, Vec3 b, Matrix V)
{
    emitSegment(applyMatrix(V, a), applyMatrix(V, b));
}



/* 2D map 与裁剪窗口： */
void updateMap(int w, int h)
{
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    w_width = w;  w_height = h;

    /* 把投影范围 [-gWinX,gWinX]x[-gWinY,gWinY] 放进可见区，x/y 用同一比例 */
    double kx = w / (2.0 * gWinX), ky = h / (2.0 * gWinY);
    gScale = (kx < ky) ? kx : ky;

    /* 裁剪窗口 = 整个屏幕（原点在中心） */
    gClipX0 = -0.5 * w;  gClipY0 = -0.5 * h;
    gClipX1 =  0.5 * w;  gClipY1 =  0.5 * h;

    {   /* 深度缓冲按需重新分配，每像素一格 */
        int need = w * h;
        if (need != gZbufCap) {
            delete[] gZbuf;
            gZbuf = new double[need];
            gZbufCap = need;
        }
    }
}

/* ================= z-buffer + 带深度测试的三角形填充 ================= */

void zbufClear(void)
{
    if (!gZbuf) return;
    int n = w_width * w_height;
    for (int i = 0; i < n; i++) gZbuf[i] = -1.0e30;   /* 全部置最远 */
}

/* 边 (a,b) 对点 p 的有向面积 */
static double edge2(double ax, double ay, double bx, double by,
                    double px, double py)
{
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

/* 光栅化一个屏幕空间三角形，逐像素深度测试。
 * sx/sy : 像素坐标（原点在窗口中心）
 * zv    : 各顶点的视图空间 z（负值，越大越靠近眼睛）
 *   透视 -> 屏幕空间线性插值 1/z，再取倒数
 *   正交 -> z 本身在屏幕空间就是线性的
 */
void fillTriangleDepth(const double sx[3], const double sy[3],
                       const double zv[3], Vec3 color)
{
    double area = edge2(sx[0], sy[0], sx[1], sy[1], sx[2], sy[2]);
    if (fabs(area) < 1e-9) return;
    double sgn = (area > 0) ? 1.0 : -1.0;
    double inv = 1.0 / (area * sgn);

    /* 整数包围盒，裁剪到窗口内 */
    double mnx = sx[0], mxx = sx[0], mny = sy[0], mxy = sy[0];
    for (int i = 1; i < 3; i++) {
        if (sx[i] < mnx) mnx = sx[i];
        if (sx[i] > mxx) mxx = sx[i];
        if (sy[i] < mny) mny = sy[i];
        if (sy[i] > mxy) mxy = sy[i];
    }
    int x0 = (int)ceil(mnx);   if (x0 < -w_width / 2)       x0 = -w_width / 2;
    int x1 = (int)floor(mxx);  if (x1 >  w_width / 2 - 1)   x1 =  w_width / 2 - 1;
    int y0 = (int)ceil(mny);   if (y0 < -w_height / 2)      y0 = -w_height / 2;
    int y1 = (int)floor(mxy);  if (y1 >  w_height / 2 - 1)  y1 =  w_height / 2 - 1;
    if (x0 > x1 || y0 > y1 || !gZbuf) return;

    double iz[3] = { 1.0 / zv[0], 1.0 / zv[1], 1.0 / zv[2] };

    glColor3f((float)color.x, (float)color.y, (float)color.z);
    glBegin(GL_POINTS);
    for (int y = y0; y <= y1; y++) {
        int rowBase = (y + w_height / 2) * w_width;
        for (int x = x0; x <= x1; x++) {
            double px = x, py = y;
            double w0 = sgn * edge2(sx[1], sy[1], sx[2], sy[2], px, py);
            double w1 = sgn * edge2(sx[2], sy[2], sx[0], sy[0], px, py);
            double w2 = sgn * edge2(sx[0], sy[0], sx[1], sy[1], px, py);
            if (w0 < -1e-9 || w1 < -1e-9 || w2 < -1e-9) continue;
            double l0 = w0 * inv, l1 = w1 * inv, l2 = w2 * inv;
            double z = gPersp ? 1.0 / (l0 * iz[0] + l1 * iz[1] + l2 * iz[2])
                              :        l0 * zv[0] + l1 * zv[1] + l2 * zv[2];
            int idx = rowBase + (x + w_width / 2);
            if (z <= gZbuf[idx]) continue; 
            gZbuf[idx] = z;
            glVertex2i(x, y);
        }
    }
    glEnd();
}
