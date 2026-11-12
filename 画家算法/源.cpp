#define GLUT_DISABLE_ATEXIT_HACK
#include "GLUT.H"
#include <math.h>
#include "camera.h"
#include "raster.h"
/* ============================================================

 *   a/d  平移 x     w/s  平移 y        z/x  等比缩放
 *   1/2/3 选旋转轴   q/e  绕该轴 ±10°
 *   j/l  眼睛 x     u/o  眼睛 y        i/k  眼睛 z
 *   [ / ] 改 d      p    正交 <-> 透视  r    复位
 * ============================================================ */

#define NV 8                    /* 顶点数 */
#define NE 12                   /* 棱数   */
int gAxis = 2;


/* 立方体：8 顶点 (±1, ±1, ±1) */
Vec3 gCube[NV] = {
    {-1,-1,-1},{ 1,-1,-1},{ 1, 1,-1},{-1, 1,-1},
    {-1,-1, 1},{ 1,-1, 1},{ 1, 1, 1},{-1, 1, 1}
};

/* 12 条棱：底面 4 + 顶面 4 + 立柱 4 */
static const int gEdge[NE][2] = {
    {0,1},{1,2},{2,3},{3,0},
    {4,5},{5,6},{6,7},{7,4},
    {0,4},{1,5},{2,6},{3,7}
};


Matrix gModel;                  /* 模型变换（累积） */

/* w_width / w_height、gScale、裁剪窗口、updateMap 都在 raster.cpp，
   本文件通过 raster.h 的 extern 使用 */


   /* ================= 显示 ================= */
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    Matrix V = viewMatrix();

    /* 坐标轴：原点向 ±2 各画一段 */

    Vec3 xp = { 2, 0, 0 }, xn = { -2, 0, 0 };
    Vec3 yp = { 0, 2, 0 }, yn = { 0, -2, 0 };
    Vec3 zp = { 0, 0, 2 }, zn = { 0, 0, -2 };

    /* 世界坐标轴：不参与模型变换，按 wasd/q/e/z/x 时保持不动 */
    glColor3f(0.85f, 0.15f, 0.15f);  drawWorldSegment(xn, xp, V);   /* X 轴 */
    glColor3f(0.15f, 0.65f, 0.15f);  drawWorldSegment(yn, yp, V);   /* Y 轴 */
    glColor3f(0.15f, 0.25f, 0.85f);  drawWorldSegment(zn, zp, V);   /* Z 轴 */

    /* 立方体 12 条棱 */
    glColor3f(0.0f, 0.0f, 0.0f);
    for (int e = 0; e < NE; e++)
        drawSegment(gCube[gEdge[e][0]], gCube[gEdge[e][1]], V);

    glFlush();
}

/* ================= 键盘交互 ================= */
void keyboard(unsigned char k, int x, int y)
{
    const double s = 0.5;
    Matrix M = identity();

    switch (k) {
        /* 模型变换 */
    case 'a': M = translation(-s, 0, 0); break;
    case 'd': M = translation(s, 0, 0); break;
    case 'w': M = translation(0, s, 0); break;
    case 's': M = translation(0, -s, 0); break;
    case 'z': M = scaling(1.1, 1.1, 1.1); break;
    case 'x': M = scaling(1 / 1.1, 1 / 1.1, 1 / 1.1); break;

        /* 轴向旋转 */
    case '1': gAxis = 0; break;
    case '2': gAxis = 1; break;
    case '3': gAxis = 2; break;
    case 'q':
    case 'e': {
        double a = (k == 'q') ? 10.0 : -10.0;
        M = (gAxis == 0) ? rotationX(a)
            : (gAxis == 1) ? rotationY(a)
            : rotationZ(a);
        break;
    }

            /* 相机：移动眼睛 */
    case 'j': gEye.x -= s; break;
    case 'l': gEye.x += s; break;
    case 'u': gEye.y += s; break;
    case 'o': gEye.y -= s; break;
    case 'i': gEye.z -= s; break;
    case 'k': gEye.z += s; break;

        /* 投影 */
    case '[': gD -= 1.0; break;
    case ']': gD += 1.0; break;
    case 'p': gPersp = !gPersp; break;

        /* 复位 */
    case 'r':
        gModel = identity();
        gEye.x = 0;  gEye.y = 0;  gEye.z = 6;
        gCenter.x = 0;  gCenter.y = 0;  gCenter.z = 0;
        gUp.x = 0;  gUp.y = 1;  gUp.z = 0;
        gD = -2.0;  gPersp = 0;  gAxis = 2;
        break;

    default: return;
    }

    gModel = matrixMut(M, gModel);   /* 左乘累积 */
    glutPostRedisplay();
}

/* ================= 投影设置（沿用课程给的 2D 设置） ================= */
void reshape(int w, int h)
{
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-0.5 * w, 0.5 * w, -0.5 * h, 0.5 * h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    updateMap(w, h);
}

/* ================= 主函数 ================= */
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(w_width, w_height);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("3D to 2D Pipeline");

    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

    gModel = identity();

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
