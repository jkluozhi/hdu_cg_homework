#define GLUT_DISABLE_ATEXIT_HACK
#include "GLUT.H"
#include <math.h>
#include "camera.h"
#include "raster.h"
#include "clip3d.h"
#include "objload.h"

/* 手动声明 GetAsyncKeyState（避免 include <windows.h> 与课程版 GLUT.H 冲突），
   用于 idle 里轮询 WASD 键状态 —— GLUT 的 keyboard 回调在鼠标按钮按住时不触发 */
extern "C" __declspec(dllimport) short __stdcall GetAsyncKeyState(int vKey);
#pragma comment(lib, "user32.lib")

/* ============================================================
 *   OBJ-only 3D -> 2D 手写渲染管线
 *   按住右键 + 移动鼠标 : Orbit 绕原点旋转视角
 *   按住右键 + W/A/S/D  : 相机前后 / 左右绕转
 *   滚轮                : dolly 拉近拉远
 *   m    加载 / 隐藏 OBJ 模型
 *   h    面片填充+深度消隐 <-> 纯线框
 *   b    开启/关闭 z-buffer 深度测试（看每面颜色差异）
 *   n/N  近平面推远/拉近（展示切割截面）
 *   -/=  缩小/放大视景体（投影窗口）
 *   p    正交 <-> 透视      r    复位
 * ============================================================ */

typedef struct {
    Vec3*  v;          /* 顶点数组 */
    int    nv;         /* 顶点数 */
    int*   face;       /* 三角索引，3*nf 个 */
    int    nf;         /* 三角面数 */
    int    n;          /* 每面顶点数，恒为 3 */
    Vec3   color;      /* 统一颜色（faceColor 为空时用） */
    Vec3*  faceColor;  /* 每面一个颜色 */
    Matrix model;      /* 模型变换 */
} ObjMesh;

Matrix gModel;                  /* 模型变换（累积） */
int    gFill = 1;               /* 1 = 面片填充 + z-buffer；0 = 纯线框 */

/* 鼠标/键盘状态（仿 UE 相机控制） */
static int gRmbDown = 0;
static int gLastX = 0, gLastY = 0;

#define OBJ_PATH "C:/Users/21505/Desktop/ComputerGraphics/cube_torus.obj"
static ObjMesh gObj = { 0, 0, 0, 0, 3, {0.75, 0.55, 0.30}, 0, {{0}} };
static int gShowObj = 0;

/* 近平面截面：收集近平面切割产生的交点（视图空间），绘制成橙色截面 cap */
#define MAX_CAP 32768
static Vec3   gCap[MAX_CAP];
static int    gCapN = 0;
static double gCapSx[MAX_CAP], gCapSy[MAX_CAP];   /* 投影后的屏幕坐标 */
static int    gCapOrder[MAX_CAP];
static double gCapCx, gCapCy;                      /* 排序中心 */

/* HSV 转 RGB，用于给每个面生成彩虹色 */
static void hsvToRgb(double h, double s, double v, Vec3* out)
{
    double c = v * s;
    double x = c * (1.0 - fabs(fmod(h * 6.0, 2.0) - 1.0));
    double m = v - c;
    double r = 0, g = 0, b = 0;
    int sector = (int)(h * 6.0) % 6;

    switch (sector) {
    case 0: r = c; g = x; b = 0; break;
    case 1: r = x; g = c; b = 0; break;
    case 2: r = 0; g = c; b = x; break;
    case 3: r = 0; g = x; b = c; break;
    case 4: r = x; g = 0; b = c; break;
    default:r = c; g = 0; b = x; break;
    }
    out->x = r + m;  out->y = g + m;  out->z = b + m;
}

/* 黄金角扫 HSV，相邻面颜色尽量拉开 */
static void assignRainbowFaceColors(ObjMesh* mesh)
{
    if (mesh->faceColor) delete[] mesh->faceColor;
    mesh->faceColor = new Vec3[mesh->nf];
    for (int i = 0; i < mesh->nf; i++) {
        double h = fmod(i * 0.61803398875, 1.0);
        hsvToRgb(h, 0.85, 0.95, &mesh->faceColor[i]);
    }
}

/* 按屏幕极角排序截面点（用于三角扇填充） */
static bool capAngleLess(int a, int b)
{
    double aa = atan2(gCapSy[a] - gCapCy, gCapSx[a] - gCapCx);
    double ab = atan2(gCapSy[b] - gCapCy, gCapSx[b] - gCapCx);
    return aa < ab;
}

/* 手写选择排序（避免 <algorithm>/<stdlib.h> 与课程版 GLUT.H 冲突） */
static void capSort(int* order, int m)
{
    for (int i = 0; i < m - 1; i++) {
        int best = i;
        for (int j = i + 1; j < m; j++)
            if (capAngleLess(order[j], order[best])) best = j;
        if (best != i) { int t = order[i]; order[i] = order[best]; order[best] = t; }
    }
}


/* ================= 显示 ================= */
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    zbufClear();                            /* 深度缓冲清零 */
    gCapN = 0;                              /* 截面点清零 */

    Matrix V = viewMatrix();

    if (gShowObj && gObj.nv > 0) {
        ObjMesh* mesh = &gObj;
        Matrix M = matrixMut(gModel, mesh->model);

        for (int f = 0; f < mesh->nf; f++) {
            const int* idx = &mesh->face[f * mesh->n];

            /* 1. 逐顶点：局部 -> 世界 -> 视图 */
            Vec3 vv[8];
            for (int i = 0; i < mesh->n; i++)
                vv[i] = applyMatrix(V, applyMatrix(M, mesh->v[idx[i]]));

            /* 2. 视锥体六平面裁剪 */
            Vec3 cv[16], capv[16];
            int capn = 0;
            int nc = clipFrustum(vv, mesh->n, cv, 16, capv, 16, &capn);
            if (nc < 3) continue;
            for (int i = 0; i < capn && gCapN < MAX_CAP; i++)
                gCap[gCapN++] = capv[i];

            /* 3. 投影 -> 屏幕像素 */
            double sx[16], sy[16], zv[16];
            int ok = 1;
            for (int i = 0; i < nc; i++) {
                double px, py;
                if (!projectPoint(cv[i], &px, &py)) { ok = 0; break; }
                toPixels(px, py, &sx[i], &sy[i]);
                zv[i] = cv[i].z;
            }
            if (!ok) continue;

            /* 4. 背面剔除（屏幕面积符号） */
            double area = 0.0;
            for (int i = 0; i < nc; i++) {
                int j = (i + 1) % nc;
                area += sx[i] * sy[j] - sx[j] * sy[i];
            }
            if (area <= 0.0) continue;

            Vec3 col = mesh->faceColor ? mesh->faceColor[f] : mesh->color;

            /* 5. z-buffer 逐像素深度测试（三角扇拆三角） */
            if (gFill) {
                for (int i = 1; i + 1 < nc; i++) {
                    double tx[3] = { sx[0], sx[i], sx[i + 1] };
                    double ty[3] = { sy[0], sy[i], sy[i + 1] };
                    double tz[3] = { zv[0], zv[i], zv[i + 1] };
                    fillTriangleDepth(tx, ty, tz, col);
                }
            }

            /* 6. 描边：填充模式用深色线框，线框模式用面颜色 */
            if (gFill) glColor3f(0.12f, 0.12f, 0.12f);
            else       glColor3f((float)col.x, (float)col.y, (float)col.z);
            for (int i = 0; i < nc; i++) {
                int j = (i + 1) % nc;
                drawLine(iround(sx[i]), iround(sy[i]), iround(sx[j]), iround(sy[j]));
            }
        }
    }



    /* 世界坐标轴（固定世界原点，不随模型变换） */
    Vec3 axp = { 2, 0, 0 }, axn = { -2, 0, 0 };
    Vec3 ayp = { 0, 2, 0 }, ayn = { 0, -2, 0 };
    Vec3 azp = { 0, 0, 2 }, azn = { 0, 0, -2 };

    glColor3f(0.85f, 0.15f, 0.15f);  drawWorldSegment(axn, axp, V);   /* X 轴 */
    glColor3f(0.15f, 0.65f, 0.15f);  drawWorldSegment(ayn, ayp, V);   /* Y 轴 */
    glColor3f(0.15f, 0.25f, 0.85f);  drawWorldSegment(azn, azp, V);   /* Z 轴 */

    glFlush();
}

/* ================= 键盘交互 ================= */
void keyboard(unsigned char k, int x, int y)
{
    switch (k) {
    case 'n': if (gNear > -9.5) gNear -= 0.5; break;   /* 近平面推远 */
    case 'N': if (gNear < -0.5) gNear += 0.5; break;   /* 近平面拉近 */

    case '-': gWinX *= 0.8;  gWinY *= 0.8;  updateMap(w_width, w_height); break;
    case '=': gWinX *= 1.25; gWinY *= 1.25; updateMap(w_width, w_height); break;

    case 'p': gPersp = !gPersp; break;
    case 'h': gFill = !gFill; break;
    case 'b': gZbufOn = !gZbufOn; break;

    case 'm': {
        if (gObj.nv == 0) {                 /* 第一次按：读文件 */
            Vec3* vv = 0;  int* ff = 0;
            if (loadObj(OBJ_PATH, 2.0, &vv, &gObj.nv, &ff, &gObj.nf)) {
                gObj.v = vv;
                gObj.face = ff;
                gObj.n = 3;
                gObj.model = identity();
                assignRainbowFaceColors(&gObj);
                gShowObj = 1;
            }
        }
        else {
            gShowObj = !gShowObj;           /* 之后按：显示 <-> 隐藏 */
        }
        break;
    }

    case 'r':
        gModel = identity();
        gD = -2.0;  gPersp = 0;  gFill = 1;
        gNear = -2.0;
        gZbufOn = 1;
        gWinX = 4.0;  gWinY = 3.0;  updateMap(w_width, w_height);
        gYaw = 0.0;  gPitch = 0.0;  gDist = 6.0;  cameraUpdate();
        break;

    default: return;
    }

    glutPostRedisplay();
}

/* ================= 鼠标交互（仿 UE） ================= */
void mouse(int btn, int state, int x, int y)
{
    if (btn == GLUT_RIGHT_BUTTON) {
        if (state == GLUT_DOWN) { gRmbDown = 1; gLastX = x; gLastY = y; }
        else { gRmbDown = 0; }
    }
    else if (btn == 3) { gDist -= 0.5; if (gDist < 0.5) gDist = 0.5; cameraUpdate(); glutPostRedisplay(); }
    else if (btn == 4) { gDist += 0.5; cameraUpdate(); glutPostRedisplay(); }
}

void motion(int x, int y)
{
    if (!gRmbDown) return;
    int dx = x - gLastX, dy = y - gLastY;
    gLastX = x;  gLastY = y;
    if (dx == 0 && dy == 0) return;

    gYaw += -dx * 0.3;
    gPitch += -dy * 0.3;
    if (gPitch > 89.0) gPitch = 89.0;
    if (gPitch < -89.0) gPitch = -89.0;

    cameraUpdate();
    glutPostRedisplay();
}

/* 每帧轮询 WASD（右键按住时生效） */
void idle(void)
{
    if (!gRmbDown) return;
    int changed = 0;
    if (GetAsyncKeyState('W') & 0x8000) { gDist -= 0.3; if (gDist < 0.5) gDist = 0.5; changed = 1; }
    if (GetAsyncKeyState('S') & 0x8000) { gDist += 0.3; changed = 1; }
    if (GetAsyncKeyState('A') & 0x8000) { gYaw += 2.0;  changed = 1; }
    if (GetAsyncKeyState('D') & 0x8000) { gYaw -= 2.0;  changed = 1; }
    if (changed) {
        cameraUpdate();
        glutPostRedisplay();
    }
}

/* ================= 投影设置 ================= */
void reshape(int w, int h)
{
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-w / 2, w / 2, -h / 2, h / 2);
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
    glutCreateWindow("OBJ-only 3D to 2D Pipeline");

    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    gModel = identity();

    /* 启动时自动加载 OBJ，默认每面彩虹色 */
    {
        Vec3* vv = 0;  int* ff = 0;
        if (loadObj(OBJ_PATH, 2.0, &vv, &gObj.nv, &ff, &gObj.nf)) {
            gObj.v = vv;
            gObj.face = ff;
            gObj.n = 3;
            gObj.model = identity();
            assignRainbowFaceColors(&gObj);
            gShowObj = 1;
        }
    }

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(idle);

    glutMainLoop();
    return 0;
}
