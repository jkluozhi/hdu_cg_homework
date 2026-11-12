#include"camera.h"
#include <math.h>



/* 相机状态：全工程唯一定义处（其他文件通过 camera.h 的 extern 使用） */
Vec3   gEye = { 0, 0, 6 };   /* E */
Vec3   gCenter = { 0, 0, 0 };   /* C */
Vec3   gUp = { 0, 1, 0 };   /* Up */
double gD = -2.0;          /* 投影平面 z_v = d */
int    gPersp = 0;             /* 0 正交，1 透视 */

/* ================= 视图变换 ================= */
/* 由 E / C / Up 构造：n 指向相机后方（E-C），u 向右，v 向上 */
Matrix viewMatrix(void)
{
    Vec3 n = vnorm(vsub(gEye, gCenter));
    Vec3 u = vnorm(vcross(gUp, n));
    Vec3 v = vcross(n, u);

    Matrix M = identity();
    M.data[0][0] = u.x;  M.data[0][1] = u.y;  M.data[0][2] = u.z;  M.data[0][3] = -vdot(u, gEye);
    M.data[1][0] = v.x;  M.data[1][1] = v.y;  M.data[1][2] = v.z;  M.data[1][3] = -vdot(v, gEye);
    M.data[2][0] = n.x;  M.data[2][1] = n.y;  M.data[2][2] = n.z;  M.data[2][3] = -vdot(n, gEye);
    return M;
}


/* ================= 投影 + 齐次除法 ================= */
/* 透视：把点投到 z_v = d 的平面上，x_p = x_v * d / z_v
   正交：直接取视图坐标，结果与 d 无关
   返回 0 表示该点不可投影（z_v ≈ 0，或在投影平面背后），整条线段丢弃 */
int projectPoint(Vec3 v, double* xp, double* yp)
{
    if (!gPersp) {
        *xp = v.x;
        *yp = v.y;
        return 1;
    }
    if (fabs(v.z) < 1e-9) return 0;         /* 落在过眼睛的平面上 */
    if (v.z * gD <= 0)   return 0;          /* 投影平面另一侧 */
    double k = gD / v.z;                    /* 齐次除法的商 */
    *xp = v.x * k;
    *yp = v.y * k;
    return 1;
}