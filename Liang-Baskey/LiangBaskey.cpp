#define GLUT_DISABLE_ATEXIT_HACK
#include "GLUT.H"
#include <math.h>
#include <stdio.h>

#include <string.h>     /* memcpy */

#define ZVALUE 20.0f
#define MAXV   64       
#define MAXXS  64       
#define N 3             
#define NV 19           

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


#define WXMIN  0
#define WYMIN  0
#define WXMAX  6
#define WYMAX  4

/* ============================================================
 * 键盘操作：w/a/s/d 平移，q/e 旋转（绕中心 ±5°），
 *          z/x 缩放（绕中心 ×1.1 / ÷1.1），r 复位
 * ============================================================ */

 /* ================= 基本类型 ================= */
typedef struct { int x, y; } Point;
typedef struct { double x, y; } Vec3;
typedef struct { double data[N][N]; } Matrix;   /* double：int 装不下 cos/sin */

/* ================= 矩阵运算 ================= */
Matrix identity(void) {
    Matrix M = { 0 };
    for (int i = 0; i < N; i++) M.data[i][i] = 1.0;
    return M;
}
Matrix matrixAdd(Matrix A, Matrix B) {
    Matrix C = { 0 };
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            C.data[i][j] = A.data[i][j] + B.data[i][j];
    return C;
}
Matrix matrixSub(Matrix A, Matrix B) {
    Matrix C = { 0 };
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            C.data[i][j] = A.data[i][j] - B.data[i][j];
    return C;
}
Matrix matrixMut(Matrix A, Matrix B) {
    Matrix C = { 0 };
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                C.data[i][j] += A.data[i][k] * B.data[k][j];
    return C;
}

/* ================= 齐次变换矩阵 ================= */
Matrix translation(double tx, double ty) {
    Matrix M = identity();
    M.data[0][2] = tx;  M.data[1][2] = ty;
    return M;
}
Matrix rotation(double deg) {                  
    Matrix M = identity();
    double r = deg * M_PI / 180.0;
    M.data[0][0] = cos(r);  M.data[0][1] = -sin(r);
    M.data[1][0] = sin(r);  M.data[1][1] = cos(r);
    return M;
}
Matrix scaling(double sx, double sy) {
    Matrix M = identity();
    M.data[0][0] = sx;  M.data[1][1] = sy;
    return M;
}
/* 绕任意中心 (cx,cy)：T(c) * M * T(-c) */
Matrix rotationAbout(double deg, double cx, double cy) {
    return matrixMut(translation(cx, cy),
        matrixMut(rotation(deg), translation(-cx, -cy)));
}
Matrix scalingAbout(double sx, double sy, double cx, double cy) {
    return matrixMut(translation(cx, cy),
        matrixMut(scaling(sx, sy), translation(-cx, -cy)));
}


Point gPoly[NV] = {
    {-25,-20},{30,-20},{30,10},{18,22},{32,38},
    {25,55},{35,78},{72,78},{78,62},{70,45},
    {83,28},{75,10},{80,-20},{135,-20},{118,30},
    {135,48},{140,95},{92,110},{68,98}
};
Matrix gTrans;          /* 当前总变换矩阵 */
double gCX, gCY;       

int w_width = 1000;
int w_height = 1000;

void computeCenter(void) {
    double sx = 0, sy = 0;
    for (int i = 0; i < NV; i++) { sx += gPoly[i].x; sy += gPoly[i].y; }
    gCX = sx / NV;  gCY = sy / NV;
}

/* ================= Bresenham 全象限直线 ================= */
void Bresenham(int x0, int y0, int x1, int y1)
{
    int dx = x1 - x0;  if (dx < 0) dx = -dx;
    int dy = y1 - y0;  if (dy < 0) dy = -dy;
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

/* ================= 坐标轴（也用 Bresenham，不用 GL_LINES） ================= */
void draw_coordinate(void)
{
    glColor3f(0.0f, 0.0f, 0.0f);
    Bresenham(-200, 0, 200, 0);
    Bresenham(0, -200, 0, 200);
}

/*  Sutherland-Hodgman 多边形裁剪 */
static int inside(Vec3 p, int edge) {         
    switch (edge) {
    case 0:  return p.x >= WXMIN;
    case 1:  return p.x <= WXMAX;
    case 2:  return p.y >= WYMIN;
    default: return p.y <= WYMAX;
    }
}

static Vec3 intersect(Vec3 S, Vec3 E, int edge) {
    Vec3 I;  double t;
    if (edge == 0 || edge == 1) {                /* 竖直边 x=常数 */
        double xw = (edge == 0) ? WXMIN : WXMAX;
        t = (xw - S.x) / (E.x - S.x);
        I.x = xw;  I.y = S.y + t * (E.y - S.y);
    }
    else {                                     /* 水平边 y=常数 */
        double yw = (edge == 2) ? WYMIN : WYMAX;
        t = (yw - S.y) / (E.y - S.y);
        I.y = yw;  I.x = S.x + t * (E.x - S.x);
    }
    return I;
}

int clipPolygon(const Vec3* in, int nin, Vec3* out)
{
    Vec3 a[MAXV], b[MAXV];
    int n = nin;
    memcpy(a, in, n * sizeof(Vec3));
    Vec3* src = a; Vec3* dst = b;

    for (int e = 0; e < 4 && n > 0; e++) {
        int m = 0;
        for (int i = 0; i < n; i++) {
            Vec3 S = src[i], E = src[(i + 1) % n];
            int inS = inside(S, e), inE = inside(E, e);
            if (inS && inE)        dst[m++] = E;                          /* 全内：收终点 */
            else if (inS)          dst[m++] = intersect(S, E, e);         /* 出：收交点 */
            else if (inE) {
                dst[m++] = intersect(S, E, e);
                dst[m++] = E;
            }                        /* 进：收交点+终点 */
        }
        n = m;
        { Vec3* tmp = src; src = dst; dst = tmp; }  
    }
    memcpy(out, src, n * sizeof(Vec3));
    return n;
}

/* ================= 扫描线多边形填充（double 顶点版） ================= */
void fillPolygon(const Vec3* poly, int n)
{
    if (poly == NULL || n < 3) return;

    double ymin = poly[0].y, ymax = poly[0].y;
    for (int i = 1; i < n; i++) {
        if (poly[i].y < ymin) ymin = poly[i].y;
        if (poly[i].y > ymax) ymax = poly[i].y;
    }

    glBegin(GL_POINTS);
    for (int y = (int)ceil(ymin); y <= (int)floor(ymax); y++)
    {
        double xs[MAXXS];
        int    cnt = 0;

        for (int i = 0; i < n; i++)
        {
            double x1 = poly[i].x, y1 = poly[i].y;
            double x2 = poly[(i + 1) % n].x, y2 = poly[(i + 1) % n].y;

            if (y1 == y2) continue;               

            double ylo, yhi, xlo;
            if (y1 < y2) { ylo = y1; yhi = y2; xlo = x1; }
            else { ylo = y2; yhi = y1; xlo = x2; }

            double dxdy = (x2 - x1) / (y2 - y1);

            if (y >= ylo && y < yhi)               
            {
                if (cnt < MAXXS)
                    xs[cnt++] = xlo + (y - ylo) * dxdy;
            }
        }

        for (int i = 0; i < cnt - 1; i++)           
            for (int j = i + 1; j < cnt; j++)
                if (xs[i] > xs[j])
                {
                    double t = xs[i]; xs[i] = xs[j]; xs[j] = t;
                }

        for (int i = 0; i + 1 < cnt; i += 2)        
        {
            int xa = (int)floor(xs[i] + 0.5);
            int xb = (int)floor(xs[i + 1] + 0.5);
            for (int x = xa; x <= xb; x++)
                glVertex2i(x, y);
        }
    }
    glEnd();
}


int ClipLine(int* x0, int* y0, int* x1, int* y1)
{
    float t0 = 0.0f, t1 = 1.0f, t;
    int xmin = 0, xmax = 6, ymin = 0, ymax = 4;
    int dx = *x1 - *x0;
    int dy = *y1 - *y0;
    int p[4] = { -dx, dx, -dy, dy };
    int q[4] = { *x0 - xmin, xmax - *x0, *y0 - ymin, ymax - *y0 };

    for (int i = 0; i < 4; i++) {
        if (p[i] == 0) {
            if (q[i] < 0) return 0;
            continue;
        }
        t = (float)q[i] / p[i];
        if (p[i] < 0) { if (t > t0) t0 = t; }
        else { if (t < t1) t1 = t; }
        if (t0 > t1) return 0;
    }

    int ox = *x0, oy = *y0;
    *x0 = (int)(ox + t0 * dx);  *y0 = (int)(oy + t0 * dy);
    *x1 = (int)(ox + t1 * dx);  *y1 = (int)(oy + t1 * dy);
    return 1;
}

/* ================= 绘制内容 ================= */
void display(void)
{
    glClearColor(1.f, 1.f, 1.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    draw_coordinate();

    /* 1. 总变换作用到原始顶点 */
    Vec3 pt[NV];
    for (int i = 0; i < NV; i++) {
        pt[i].x = gTrans.data[0][0] * gPoly[i].x + gTrans.data[0][1] * gPoly[i].y + gTrans.data[0][2];
        pt[i].y = gTrans.data[1][0] * gPoly[i].x + gTrans.data[1][1] * gPoly[i].y + gTrans.data[1][2];
    }

    /* 2. 被裁剪多边形轮廓（红） */
    glColor3f(1, 0, 0);
    for (int i = 0; i < NV; i++)
        Bresenham((int)floor(pt[i].x + 0.5), (int)floor(pt[i].y + 0.5),
            (int)floor(pt[(i + 1) % NV].x + 0.5), (int)floor(pt[(i + 1) % NV].y + 0.5));

    /* 3. 裁剪窗口（蓝，4 条边全走 Bresenham） */
    glColor3f(0, 0, 1);
    Bresenham(WXMIN, WYMIN, WXMAX, WYMIN);
    Bresenham(WXMAX, WYMIN, WXMAX, WYMAX);
    Bresenham(WXMAX, WYMAX, WXMIN, WYMAX);
    Bresenham(WXMIN, WYMAX, WXMIN, WYMIN);

    /* 4. 裁剪 + 填充（绿） */
    Vec3 cp[MAXV];
    int nc = clipPolygon(pt, NV, cp);
    if (nc >= 3) {
        glColor3f(0, 0.75f, 0);
        fillPolygon(cp, nc);
    }

    glFlush();
}

/* ================= 键盘交互 ================= */
void keyboard(unsigned char k, int x, int y)
{
    Matrix M = identity();
    switch (k) {
    case 'a': M = translation(-2, 0); break;
    case 'd': M = translation(2, 0); break;
    case 's': M = translation(0, -2); break;
    case 'w': M = translation(0, 2); break;
    case 'q': M = rotationAbout(5, gCX, gCY); break;   /* 逆时针 5° */
    case 'e': M = rotationAbout(-5, gCX, gCY); break;
    case 'z': M = scalingAbout(1.1, 1.1, gCX, gCY); break;
    case 'x': M = scalingAbout(1 / 1.1, 1 / 1.1, gCX, gCY); break;
    case 'r': gTrans = identity(); glutPostRedisplay(); return;
    default: return;
    }
    gTrans = matrixMut(M, gTrans);   /* 左乘：在世界坐标系中追加这次变换 */
    printf("key=%c  center=(%.1f,%.1f)\n", k, gCX, gCY);
    glutPostRedisplay();
}

/* ================= 投影设置 ================= */
void reshape(int w, int h)
{
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-45.0, 165.0, -45.0, 165.0, -ZVALUE, ZVALUE); 

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

/* ================= 主函数 ================= */
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(w_width, w_height);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Polygon Clip & Fill");

    gTrans = identity();
    computeCenter();

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
