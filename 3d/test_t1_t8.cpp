/* T1-T8 测试（多文件版）：链接 math3d/camera/raster 三个 obj，不链 3d.cpp（避开 main） */
#include <stdio.h>
#include <math.h>
#include "math3d.h"
#include "camera.h"
#include "raster.h"

/* raster.obj 的 drawSegment 引用了 gModel，测试不调用它，这里给个定义 */
Matrix gModel;

static void setCam(double ex, double ey, double ez,
                   double cx, double cy, double cz,
                   double ux, double uy, double uz,
                   double d, int persp)
{
    gEye.x = ex;  gEye.y = ey;  gEye.z = ez;
    gCenter.x = cx;  gCenter.y = cy;  gCenter.z = cz;
    gUp.x = ux;  gUp.y = uy;  gUp.z = uz;
    gD = d;  gPersp = persp;
}

/* 单点走完整管线：model -> world -> view -> 投影 -> 像素 */
static int pipe(Vec3 P, Vec3* world, Vec3* view,
                double* xp, double* yp, double* sx, double* sy)
{
    Matrix V = viewMatrix();
    *world = applyMatrix(gModel, P);
    *view  = applyMatrix(V, *world);
    if (!projectPoint(*view, xp, yp)) return 0;
    toPixels(*xp, *yp, sx, sy);
    return 1;
}

int main(void)
{
    updateMap(1000, 1000);
    printf("窗口 1000x1000 -> gScale = %.4g, 裁剪窗口 x[%.0f,%.0f] y[%.0f,%.0f]\n\n",
           gScale, gClipX0, gClipX1, gClipY0, gClipY1);

    /* ---------------- T1 ---------------- */
    printf("T1  P=(1,2,3); 先 scale(2,1,0.5) 再 translate(1,-1,2)\n");
    {
        Vec3 P = { 1, 2, 3 };
        gModel = matrixMut(translation(1, -1, 2), scaling(2, 1, 0.5));
        Vec3 r = applyMatrix(gModel, P);
        printf("    中间: (2,2,1.5)   最终点 = (%.4g, %.4g, %.4g)\n\n", r.x, r.y, r.z);
        gModel = identity();
    }

    /* ---------------- T2 ---------------- */
    printf("T2  默认相机 E=(0,0,6) C=O Up=(0,1,0)，P=(1,1,1)\n");
    {
        Vec3 P = { 1, 1, 1 }, w, v; double xp, yp, sx, sy;

        setCam(0, 0, 6, 0, 0, 0, 0, 1, 0, -2, 1);
        pipe(P, &w, &v, &xp, &yp, &sx, &sy);
        printf("    透视: view=(%.4g,%.4g,%.4g)  w=z_v/d=%.4g  x_p,y_p=(%.4g,%.4g)  像素=(%.4g,%.4g)\n",
               v.x, v.y, v.z, v.z / gD, xp, yp, sx, sy);

        gPersp = 0;
        pipe(P, &w, &v, &xp, &yp, &sx, &sy);
        printf("    正交: view=(%.4g,%.4g,%.4g)  w=1            x_p,y_p=(%.4g,%.4g)  像素=(%.4g,%.4g)\n\n",
               v.x, v.y, v.z, xp, yp, sx, sy);
    }

    /* ---------------- T3 ---------------- */
    printf("T3  E=(4,0,0) C=O Up=(0,1,0), P=(1,2,3), d=-2\n");
    {
        Vec3 P = { 1, 2, 3 }, w, v; double xp, yp, sx, sy;
        setCam(4, 0, 0, 0, 0, 0, 0, 1, 0, -2, 1);
        pipe(P, &w, &v, &xp, &yp, &sx, &sy);
        printf("    n=(%.3g,%.3g,%.3g) u=(%.3g,%.3g,%.3g)\n",
               vnorm(vsub(gEye, gCenter)).x, vnorm(vsub(gEye, gCenter)).y, vnorm(vsub(gEye, gCenter)).z,
               vnorm(vcross(gUp, vnorm(vsub(gEye, gCenter)))).x,
               vnorm(vcross(gUp, vnorm(vsub(gEye, gCenter)))).y,
               vnorm(vcross(gUp, vnorm(vsub(gEye, gCenter)))).z);
        printf("    view=(%.4g,%.4g,%.4g)  透视 x_p,y_p=(%.4g,%.4g)  像素=(%.4g,%.4g)\n\n",
               v.x, v.y, v.z, xp, yp, sx, sy);
    }

    /* ---------------- T4 ---------------- */
    printf("T4  视图坐标线段 x:-1->1, y=0，分别位于 z_v=-4 和 -8\n");
    for (int mode = 0; mode < 2; mode++) {
        gPersp = mode;
        gD = -2;
        printf("    [%s] d=-2\n", mode ? "透视" : "正交");
        for (int i = 0; i < 2; i++) {
            double z = (i == 0) ? -4.0 : -8.0;
            Vec3 A = { -1, 0, z }, B = { 1, 0, z };
            double xa = 0, ya = 0, xb = 0, yb = 0;
            int oka = projectPoint(A, &xa, &ya);
            int okb = projectPoint(B, &xb, &yb);
            printf("        z_v=%.0f  x_p=%.4g .. %.4g   长度=%.4g  (ok=%d/%d)\n",
                   z, xa, xb, fabs(xb - xa), oka, okb);
        }
    }
    printf("\n");

    /* ---------------- D1 ---------------- */
    printf("D1  d 从 -2 改到 -4（映射固定）\n");
    for (int mode = 0; mode < 2; mode++) {
        gPersp = mode;
        gD = -4;
        printf("    [%s] d=-4\n", mode ? "透视" : "正交");
        for (int i = 0; i < 2; i++) {
            double z = (i == 0) ? -4.0 : -8.0;
            Vec3 A = { -1, 0, z }, B = { 1, 0, z };
            double xa = 0, ya = 0, xb = 0, yb = 0;
            projectPoint(A, &xa, &ya);
            projectPoint(B, &xb, &yb);
            printf("        z_v=%.0f  长度=%.4g\n", z, fabs(xb - xa));
        }
    }
    printf("\n");

    /* ---------------- T5 ---------------- */
    printf("T5  投影平面上的边 (3,1)->(5,1)，gScale=%.4g\n", gScale);
    {
        double x0 = 3 * gScale, y0 = 1 * gScale;
        double x1 = 5 * gScale, y1 = 1 * gScale;

        printf("    default map 像素: (%.4g,%.4g)->(%.4g,%.4g)\n", x0, y0, x1, y1);
        {
            double a = x0, b = y0, c = x1, d = y1;
            int ok = clipLine(&a, &b, &c, &d);
            printf("      窗口 [-500,500]x[-500,500]  ok=%d  裁剪后 (%.4g,%.4g)->(%.4g,%.4g)\n",
                   ok, a, b, c, d);
        }
        /* 800×600 窗口，原点在左下角 -> 换算成原点在中心 */
        gClipX0 = -400;  gClipX1 = 400;
        gClipY0 = -300;  gClipY1 = 300;
        {
            double a = x0, b = y0, c = x1, d = y1;
            int ok = clipLine(&a, &b, &c, &d);
            printf("      窗口 [0,800]x[0,600] 换算 x[-400,400] y[-300,300]  ok=%d\n", ok);
            printf("      裁剪后 (%.4g,%.4g)->(%.4g,%.4g)  长度 %.4g -> %.4g\n\n",
                   a, b, c, d, fabs(x1 - x0), fabs(c - a));
        }
        updateMap(1000, 1000);
    }

    /* ---------------- T6 ---------------- */
    printf("T6  退化情况\n");
    {
        Vec3 P = { 1, 1, 1 }, w, v; double xp, yp, sx, sy;

        setCam(0, 0, 0, 0, 0, 0, 0, 1, 0, -2, 1);
        int ok = pipe(P, &w, &v, &xp, &yp, &sx, &sy);
        printf("    E=C:            n=(0,0,0) -> view=(%.4g,%.4g,%.4g), projectPoint=%d (整段丢弃)\n",
               v.x, v.y, v.z, ok);

        setCam(0, 0, 6, 0, 0, 0, 0, 0, 1, -2, 1);
        ok = pipe(P, &w, &v, &xp, &yp, &sx, &sy);
        printf("    Up 平行于 n:    u=(0,0,0) -> view=(%.4g,%.4g,%.4g), projectPoint=%d, 投到 (%.4g,%.4g)\n",
               v.x, v.y, v.z, ok, xp, yp);

        setCam(0, 0, 6, 0, 0, 0, 0, 1, 0, -2, 1);
        {
            Vec3 Q = { 1, 0, 6 };   /* 正好在过眼睛的平面上 */
            ok = pipe(Q, &w, &v, &xp, &yp, &sx, &sy);
            printf("    z_v=0:          view=(%.4g,%.4g,%.4g), projectPoint=%d (除零，整段丢弃)\n\n",
                   v.x, v.y, v.z, ok);
        }
    }

    /* ---------------- T7 ---------------- */
    printf("T7  视图坐标边 (0,0,-2)->(1,0,1)，透视 d=-2\n");
    {
        Vec3 A = { 0, 0, -2 }, B = { 1, 0, 1 };
        double xa = 0, ya = 0, xb = 0, yb = 0;
        int oka = projectPoint(A, &xa, &ya);
        int okb = projectPoint(B, &xb, &yb);
        printf("    端点A z_v=-2: ok=%d -> (%.4g,%.4g)\n", oka, xa, ya);
        printf("    端点B z_v=+1: ok=%d (在投影平面背后)\n", okb);
        printf("    策略: 任一端点不可投影 -> 整条线段不画\n\n");
    }

    /* ---------------- T8 ---------------- */
    printf("T8  投影范围 [-4,4]x[-3,3] -> 画布 1000x600\n");
    {
        updateMap(1000, 600);
        printf("    gScale = min(1000/8, 600/6) = %.4g\n", gScale);
        printf("    角点映射: (4,3)->(%.4g,%.4g)  (-4,-3)->(%.4g,%.4g)\n",
               4 * gScale, 3 * gScale, -4 * gScale, -3 * gScale);
        printf("    裁剪窗口 x[%.0f,%.0f] y[%.0f,%.0f]  -> 刚好贴合\n",
               gClipX0, gClipX1, gClipY0, gClipY1);
        printf("    x/y 比例均为 %.4g px/单位，圆不会变椭圆\n", gScale);
    }

    return 0;
}
