#include "curves.h"



/* ================= 三次 Bezier 曲线：Bernstein 显式公式 =================*/
Vec3 bezierPoint(const Vec3* P, int n, double t)
{
    (void)n;                            

    double u  = 1.0 - t;                
    double b0 = u * u * u;              
    double b1 = 3.0 * t * u * u;        
    double b2 = 3.0 * t * t * u;        
    double b3 = t * t * t;              

    Vec3 out;                           
    out.x = b0 * P[0].x + b1 * P[1].x + b2 * P[2].x + b3 * P[3].x;
    out.y = b0 * P[0].y + b1 * P[1].y + b2 * P[2].y + b3 * P[3].y;
    out.z = b0 * P[0].z + b1 * P[1].z + b2 * P[2].z + b3 * P[3].z;
    return out;
}


/* ================= B 样条：Cox-de Boor =================
 * 曲线定义（通用公式）：C(u) = Σ_{i=0}^{n} N_{i,p}(u) P_i
 * 其中 N_{i,p} 是 p 次 B 样条基函数，由 Cox-de Boor 递推计算：
 *     N_{i,0}(u) = 1   (若 U[i] <= u < U[i+1])，否则 0
 *     N_{i,k}(u) = (u - U[i]) / (U[i+k] - U[i]) * N_{i,k-1}(u)
 *                + (U[i+k+1] - u) / (U[i+k+1] - U[i+1]) * N_{i+1,k-1}(u)
 * 任意 u 处只有 p+1 个基函数非零，故 findSpan + basisFuns 只算这 p+1 个。 */

static int findSpan(int n, int p, double u, const double* U)
{
    if (u >= U[n + 1]) return n;        
    int low = p, high = n + 1;         
    int mid = (low + high) / 2;
    while (u < U[mid] || u >= U[mid + 1]) {
        if (u < U[mid]) high = mid; else low = mid;
        mid = (low + high) / 2;
    }
    return mid;
}

/* 计算区间 [U[i], U[i+1]) 上非零的 p+1 个基函数，结果按 N[0..p] 存放，
 * N[j] 对应控制点下标 (i-p+j) 的权重。 */
static void basisFuns(int i, double u, int p, const double* U, double* N)
{
    double left[8], right[8];           /* left[j]=u-U[i+1-j], right[j]=U[i+j]-u */
    N[0] = 1.0;                         /* 0 次基函数恒为 1 */
    for (int j = 1; j <= p; j++) {
        left[j]  = u - U[i + 1 - j];
        right[j] = U[i + j] - u;
        double saved = 0.0;
        for (int r = 0; r < j; r++) {
            double temp = N[r] / (right[r + 1] + left[j - r]);
            N[r]  = saved + right[r + 1] * temp;
            saved = left[j - r] * temp;
        }
        N[j] = saved;
    }
}


Vec3 bsplinePoint(const Vec3* P, int n, const double* U, int p, double u)
{
    int span = findSpan(n, p, u, U);    
    double N[8];
    basisFuns(span, u, p, U, N);        
    Vec3 out = { 0, 0, 0 };
    for (int j = 0; j <= p; j++) {
        int idx = span - p + j;         
        out.x += N[j] * P[idx].x;
        out.y += N[j] * P[idx].y;
        out.z += N[j] * P[idx].z;
    }
    return out;
}


/* ================= 双三次 Bezier 曲面（张量积） =================
 * 曲面定义：S(u,v) = Σ_i Σ_j B_i^3(u) B_j^3(v) P_ij */
Vec3 bezierSurfacePoint(const Vec3 P[4][4], double u, double v)
{
    Vec3 row[4];
    for (int i = 0; i < 4; i++) {
        Vec3 c[4] = { P[i][0], P[i][1], P[i][2], P[i][3] };
        row[i] = bezierPoint(c, 3, v);  /* v 方向插值 */
    }
    return bezierPoint(row, 3, u);      /* u 方向插值 */
}


/* ================= 双三次夹持 B 样条曲面（张量积） =================
 * 曲面定义：S(u,v) = Σ_i Σ_j N_{i,3}(u) N_{j,3}(v) P_ij
 * 与 Bezier 曲面完全同理：先对 v 方向、再对 u 方向各做一次 B 样条求值。
 * 5x5 控制点，n=4，p=3，U/V 为夹持节点向量（端点插值）。 */
Vec3 bsplineSurfacePoint(const Vec3 P[5][5], const double* U, const double* V,
                         int p, double u, double v)
{
    Vec3 row[5];
    for (int i = 0; i < 5; i++) {
        Vec3 c[5] = { P[i][0], P[i][1], P[i][2], P[i][3], P[i][4] };
        row[i] = bsplinePoint(c, 4, V, p, v);   /* v 方向插值 */
    }
    return bsplinePoint(row, 4, U, p, u);       /* u 方向插值 */
}


/* ================= 采样生成三角网格 =================
 * 把任意参数曲面（ev 回调，可携带 aux 上下文）按 Nu x Nv 均匀采样成顶点，
 * 再把相邻四个顶点拆成两个三角形，得到可交给渲染管线的三角网格。
 * 顶点排列：行优先，V[j*Nu + i] 对应 (u = i/(Nu-1), v = j/(Nv-1))。
 * 每个格子 (i,j) 拆两个三角形 (a,b,c) 与 (b,d,c)，其中：
 *     a = (i,j)   b = (i+1,j)   c = (i,j+1)   d = (i+1,j+1)
 * 该绕序保证三角形法线朝 +z（默认视角下为正面，不会被背面剔除）。
 * 返回三角形数；outV/outF 由调用方持有并负责释放（new[] 分配）。 */
int buildSurfaceMesh(SurfEval ev, void* aux, int Nu, int Nv, Vec3** outV, int** outF)
{
    int nv = Nu * Nv;                   /* 顶点数 */
    int nf = 2 * (Nu - 1) * (Nv - 1);   /* 三角形数：每格 2 个 */
    Vec3* V = new Vec3[nv];
    int*  F = new int[nf * 3];

    /* 1. 采样：u、v 各自在 [0,1] 均匀取 Nu/Nv 份 */
    for (int j = 0; j < Nv; j++) {
        double v = (Nv > 1) ? (double)j / (Nv - 1) : 0.0;
        for (int i = 0; i < Nu; i++) {
            double u = (Nu > 1) ? (double)i / (Nu - 1) : 0.0;
            V[j * Nu + i] = ev(u, v, aux);
        }
    }

    /* 2. 三角化：每个格子 (i,j) 拆 (a,b,c) 与 (b,d,c)，法线朝 +z */
    int t = 0;
    for (int j = 0; j < Nv - 1; j++) {
        for (int i = 0; i < Nu - 1; i++) {
            int a = j * Nu + i;
            int b = j * Nu + (i + 1);
            int c = (j + 1) * Nu + i;
            int d = (j + 1) * Nu + (i + 1);
            F[t * 3 + 0] = a;  F[t * 3 + 1] = b;  F[t * 3 + 2] = c;  t++;
            F[t * 3 + 0] = b;  F[t * 3 + 1] = d;  F[t * 3 + 2] = c;  t++;
        }
    }

    *outV = V;
    *outF = F;
    return nf;
}
