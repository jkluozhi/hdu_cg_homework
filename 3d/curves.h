#ifndef CURVES_H
#define CURVES_H
#include "math3d.h"

/* 三次 Bezier 曲线单点求值（Bernstein 显式公式）。
 * B(t) = (1-t)^3 P0 + 3t(1-t)^2 P1 + 3t^2(1-t) P2 + t^3 P3
 * P 为 n+1 个控制点，n 为次数（本工程恒为 3），t 在 [0,1]。 */
Vec3 bezierPoint(const Vec3* P, int n, double t);

/* B 样条单点求值（Cox-de Boor）。
 * P 为 n+1 个控制点，p 为次数，U 为节点向量（夹持），u 在 [0,1]。 */
Vec3 bsplinePoint(const Vec3* P, int n, const double* U, int p, double u);

/* 双三次 Bezier 曲面单点求值，控制网格 P[4][4]，u/v 在 [0,1]。 */
Vec3 bezierSurfacePoint(const Vec3 P[4][4], double u, double v);

/* 双三次夹持 B 样条曲面单点求值，控制网格 P[5][5]，u/v 在 [0,1]。 */
Vec3 bsplineSurfacePoint(const Vec3 P[5][5], const double* U, const double* V,
                         int p, double u, double v);

/* 由参数曲面采样生成三角网格。
 * ev 在 (u,v) 处求值（可携带 aux 上下文）；Nu*Nv 个顶点，2*(Nu-1)*(Nv-1) 个三角形。
 * 绕序保证法线朝向 +z。返回三角形数；outV/outF 由调用方持有并负责释放。 */
typedef Vec3 (*SurfEval)(double u, double v, void* aux);
int buildSurfaceMesh(SurfEval ev, void* aux, int Nu, int Nv, Vec3** outV, int** outF);

#endif
