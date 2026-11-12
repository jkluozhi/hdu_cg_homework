#include"math3d.h"
#include<math.h>


/* ================= 矩阵 ================= */
Matrix identity(void) {
    Matrix M = { 0 };
    for (int i = 0; i < 4; i++) M.data[i][i] = 1.0;
    return M;
}
Matrix matrixMut(Matrix A, Matrix B) {
    Matrix C = { 0 };
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                C.data[i][j] += A.data[i][k] * B.data[k][j];
    return C;
}
Matrix translation(double tx, double ty, double tz) {
    Matrix M = identity();
    M.data[0][3] = tx;  M.data[1][3] = ty;  M.data[2][3] = tz;
    return M;
}
Matrix scaling(double sx, double sy, double sz) {
    Matrix M = identity();
    M.data[0][0] = sx;  M.data[1][1] = sy;  M.data[2][2] = sz;
    return M;
}
Matrix rotationX(double deg) {
    Matrix M = identity();
    double r = deg * M_PI / 180.0;
    M.data[1][1] = cos(r);  M.data[1][2] = -sin(r);
    M.data[2][1] = sin(r);  M.data[2][2] = cos(r);
    return M;
}
Matrix rotationY(double deg) {
    Matrix M = identity();
    double r = deg * M_PI / 180.0;
    M.data[0][0] = cos(r);   M.data[0][2] = sin(r);
    M.data[2][0] = -sin(r);  M.data[2][2] = cos(r);
    return M;
}
Matrix rotationZ(double deg) {
    Matrix M = identity();
    double r = deg * M_PI / 180.0;
    M.data[0][0] = cos(r);  M.data[0][1] = -sin(r);
    M.data[1][0] = sin(r);  M.data[1][1] = cos(r);
    return M;
}

/* 仿射矩阵乘点（w = 1，不做除法；除法在投影那一步单独做） */
Vec3 applyMatrix(Matrix M, Vec3 p)
{
    Vec3 r;
    r.x = M.data[0][0] * p.x + M.data[0][1] * p.y + M.data[0][2] * p.z + M.data[0][3];
    r.y = M.data[1][0] * p.x + M.data[1][1] * p.y + M.data[1][2] * p.z + M.data[1][3];
    r.z = M.data[2][0] * p.x + M.data[2][1] * p.y + M.data[2][2] * p.z + M.data[2][3];
    return r;
}

/* ================= 向量工具 ================= */
Vec3 vsub(Vec3 a, Vec3 b) {
    Vec3 r = { a.x - b.x, a.y - b.y, a.z - b.z };
    return r;
}
double vdot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec3 vcross(Vec3 a, Vec3 b) {
    Vec3 r = { a.y * b.z - a.z * b.y,
               a.z * b.x - a.x * b.z,
               a.x * b.y - a.y * b.x };
    return r;
}
Vec3 vnorm(Vec3 a) {
    Vec3 r = { 0, 0, 0 };
    double L = sqrt(vdot(a, a));
    if (L > 1e-9) { r.x = a.x / L; r.y = a.y / L; r.z = a.z / L; }
    return r;
}