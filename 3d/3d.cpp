#define GLUT_DISABLE_ATEXIT_HACK
#include "GLUT.H"
#include <math.h>
#include "camera.h"
#include "raster.h"
#include "clip3d.h"
#include "objload.h"
#include "curves.h"

/* 手动声明 GetAsyncKeyState（避免 include <windows.h> 与 GLUT.H 宏冲突），
   用于 idle 里轮询 WASD 键状态 —— GLUT 的 keyboard 回调在鼠标按钮按住时不触发 */
extern "C" __declspec(dllimport) short __stdcall GetAsyncKeyState(int vKey);
#pragma comment(lib, "user32.lib")



/* ============================================================
 *   鼠标+键盘控制
 *     按住右键 + 移动鼠标    Orbit 绕原点旋转视角
 *     按住右键 + W/A/S/D     相机前后 / 左右绕转
 *     滚轮                   dolly 拉近拉远
 *   5    在 OBJ
 *   h    面片填充+深度消隐 <-> 纯线框
 *   b    开启/关闭 z-buffer 深度测试（看每面颜色差异）
 *   n/N   近平面推远/拉近（展示切割截面）
 *   -/=   缩小/放大视景体（投影窗口）
 *   p    正交 <-> 透视      r    复位
 *   m    加载 / 隐藏 lily OBJ 模型
 * ============================================================ */




typedef struct {
    const Vec3* v;
    int         nv;
    const int* face;
    int         nf;
    int         n;
    Vec3        color;
    Vec3* faceColor;   /* 每面一个颜色（NULL = 全部用 color） */
    Matrix      model;
    int         twoSided;   /* 1 = 不剔除背面（曲面用，双面可见） */
} Mesh;



Matrix gModel;                  /* 模型变换（累积） */
int    gFill = 1;               /* 1 = 面片填充 + z-buffer 消隐；0 = 纯线框 */

/* 鼠标/键盘状态（仿 UE 相机控制） */
static int gRmbDown = 0;        /* 右键是否按住 */
static int gLastX = 0, gLastY = 0;

#define OBJ_PATH "C:/Users/21505/Desktop/ComputerGraphics/cube_torus.obj"
Mesh   gObjMesh = { 0, 0, 0, 0, 0, {0,0,0}, 0, {{0}}, 0 };   /* 运行时加载的 OBJ 网格（m 键加载） */
int    gShowObj = 0;            /* 1 = 把 OBJ 网格加入渲染列表 */

/* 曲线/曲面：gMode 用 '5' 键在 5 个对象间轮流切换 */
int    gMode = 0;               /* 0=OBJ 1=Bézier曲线 2=B样条曲线 3=Bézier曲面 4=B样条曲面 */
#define CURVE_N 70
static Vec3  gBezPts[CURVE_N + 1], gBezCtrl[4];   /* Bézier 曲线采样点 / 控制点 */
static Vec3  gBsPts[CURVE_N + 1], gBsCtrl[7];    /* B 样条曲线采样点 / 控制点 */
static Mesh  gBezierSurf, gBSplineSurf;           /* 两个曲面网格 */

/* 近平面截面：收集近平面切割产生的交点（视图空间），绘制成截面 cap */
#define MAX_CAP 32768
static Vec3 gCap[MAX_CAP];
static int  gCapN = 0;
static double gCapSx[MAX_CAP], gCapSy[MAX_CAP];   /* 投影后的屏幕坐标 */
static int    gCapOrder[MAX_CAP];
static double gCapCx, gCapCy;                      /* 排序中心 */
/* w_width / w_height、gScale、裁剪窗口、updateMap 都在 raster.cpp，
   本文件通过 raster.h 的 extern 使用 */


   /* ================= 曲线/曲面数据构建 ================= */
static void drawPolyline(const Vec3* pts, int n, Matrix V, float r, float g, float b)
{
    glColor3f(r, g, b);
    for (int i = 0; i + 1 < n; i++)
        drawSegment(pts[i], pts[i + 1], V);
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

/* 绘制近平面截面：把收集到的截面点投影、排序、三角扇填充 */
static void drawClipCap(void)
{
    if (gCapN < 3) return;

    /* 1. 投影到屏幕 */
    int m = 0;
    for (int i = 0; i < gCapN; i++) {
        double px, py;
        if (!projectPoint(gCap[i], &px, &py)) continue;
        toPixels(px, py, &gCapSx[m], &gCapSy[m]);
        gCapOrder[m] = m;
        m++;
    }
    if (m < 3) return;

    /* 2. 求屏幕重心 */
    double cx = 0, cy = 0;
    for (int i = 0; i < m; i++) { cx += gCapSx[i]; cy += gCapSy[i]; }
    gCapCx = cx / m;  gCapCy = cy / m;

    /* 3. 按极角排序 */
    capSort(gCapOrder, m);

    /* 4. 三角扇填充（截面颜色，参与 z-buffer） */
    Vec3 col = { 1.0, 0.45, 0.15 };   /* 橙色截面 */
    for (int i = 1; i + 1 < m; i++) {
        int a = gCapOrder[0], b = gCapOrder[i], c = gCapOrder[i + 1];
        double tx[3] = { gCapSx[a], gCapSx[b], gCapSx[c] };
        double ty[3] = { gCapSy[a], gCapSy[b], gCapSy[c] };
        double tz[3] = { gNear, gNear, gNear };   /* 截面都在近平面 z=gNear 上 */
        fillTriangleDepth(tx, ty, tz, col);
    }
}



/* ================= 显示 ================= */
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    zbufClear();                            /* 深度缓冲清零 */
    gCapN = 0;                              /* 截面点清零 */

    Matrix V = viewMatrix();

    /* 本帧参与渲染的网格：按 gMode 选择 OBJ 或某个曲面 */
    Mesh* active[1];
    int   nActive = 0;
    switch (gMode) {
    case 0: if (gShowObj && gObjMesh.nv > 0) active[nActive++] = &gObjMesh; break;
    case 3: active[nActive++] = &gBezierSurf;  break;
    case 4: active[nActive++] = &gBSplineSurf; break;
    default: break;   /* 曲线模式走下面的折线绘制 */
    }

    for (int m = 0; m < nActive; m++) {
        Mesh* mesh = active[m];
        Matrix M = matrixMut(gModel, mesh->model);

        for (int f = 0; f < mesh->nf; f++) {
            const int* idx = &mesh->face[f * mesh->n];

            /* 1. 逐顶点：局部 -> 世界 -> 视图 */
            Vec3 vv[8];
            for (int i = 0; i < mesh->n; i++)
                vv[i] = applyMatrix(V, applyMatrix(M, mesh->v[idx[i]]));

            /* 2. 视锥体六平面裁剪*/
            Vec3 cv[16], capv[16];
            int capn = 0;
            int nc = clipFrustum(vv, mesh->n, cv, 16, capv, 16, &capn);
            if (nc < 3) continue;
            for (int i = 0; i < capn && gCapN < MAX_CAP; i++)
                gCap[gCapN++] = capv[i];   /* 收集近平面交点 */

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

            /* 4. 背面剔除： */
            double area = 0.0;
            for (int i = 0; i < nc; i++) {
                int j = (i + 1) % nc;
                area += sx[i] * sy[j] - sx[j] * sy[i];
            }
            if (area <= 0.0 && !mesh->twoSided) continue;


            Vec3 col = mesh->faceColor ? mesh->faceColor[f] : mesh->color;

            /* 6. z-buffer 逐像素深度测试 */
            if (gFill) {
                for (int i = 1; i + 1 < nc; i++) {
                    double tx[3] = { sx[0], sx[i], sx[i + 1] };
                    double ty[3] = { sy[0], sy[i], sy[i + 1] };
                    double tz[3] = { zv[0], zv[i], zv[i + 1] };
                    fillTriangleDepth(tx, ty, tz, col);
                }
            }


            if (mesh = &gObjMesh) {
                if (gFill) glColor3f(0.12f, 0.12f, 0.12f);
                else       glColor3f((float)mesh->color.x, (float)mesh->color.y, (float)mesh->color.z);
                for (int i = 0; i < nc; i++) {
                    int j = (i + 1) % nc;
                    drawLine(iround(sx[i]), iround(sy[i]), iround(sx[j]), iround(sy[j]));
                }
            }
        }
    }



    /* 曲线（模式 1/2）：灰色控制多边形 + 彩色曲线本身 */
    if (gMode == 1) {
        drawPolyline(gBezCtrl, 4, V, 0.70f, 0.70f, 0.70f);
        drawPolyline(gBezPts, CURVE_N + 1, V, 0.85f, 0.15f, 0.15f);
    }
    else if (gMode == 2) {
        drawPolyline(gBsCtrl, 7, V, 0.70f, 0.70f, 0.70f);
        drawPolyline(gBsPts, CURVE_N + 1, V, 0.15f, 0.25f, 0.85f);
    }

    /* 世界坐标轴： */
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
        /* 近平面：往眼睛方向推 / 往远处拉，展示切割 */
    case 'n': if (gNear > -9.5) gNear -= 0.5; break;
    case 'N': if (gNear < -0.5) gNear += 0.5; break;

        /* 视景体（投影窗口）缩放：缩小后模型相对变大，移动时容易被裁剪 */
    case '-': gWinX *= 0.8;  gWinY *= 0.8;  updateMap(w_width, w_height); break;
    case '=': gWinX *= 1.25; gWinY *= 1.25; updateMap(w_width, w_height); break;

        /* 投影 */
    case 'p': gPersp = !gPersp; break;

        /* 曲线/曲面切换：OBJ -> Bézier曲线 -> B样条曲线 -> Bézier曲面 -> B样条曲面 */
    case '5':
        gMode = (gMode + 1) % 5;
        gModel = identity();   /* 切换物体时复位累积变换 */
        break;

        /* 填充/消隐开关 */
    case 'h': gFill = !gFill; break;

        /* z-buffer 开关：关掉后能看到每面颜色的差异（画家算法） */
    case 'b': gZbufOn = !gZbufOn; break;

        /* 加载 / 隐藏 OBJ 模型 */
    case 'm': {
        if (gObjMesh.nv == 0) {                 /* 第一次按：读文件 */
            Vec3* vv = 0;  int* ff = 0;
            if (loadObj(OBJ_PATH, 2.0, &vv, &gObjMesh.nv, &ff, &gObjMesh.nf)) {
                gObjMesh.v = vv;
                gObjMesh.face = ff;
                gObjMesh.n = 3;
                gObjMesh.model = identity();
                ///assignRainbowFaceColors(&gObjMesh);   /* 每面不同颜色 */
                gShowObj = 1;
            }
        }
        else {
            gShowObj = !gShowObj;               /* 之后按：显示 <-> 隐藏 */
        }
        break;
    }

            /* 复位 */
    case 'r':
        gModel = identity();
        gD = -2.0;  gPersp = 0;  gFill = 1;
        gNear = -2.0;   /* 近平面复位到与投影窗口重合 */
        gZbufOn = 1;    /* 恢复深度测试 */
        gWinX = 4.0;  gWinY = 3.0;  updateMap(w_width, w_height);   /* 视景体复位 */
        gYaw = 0.0;  gPitch = 0.0;  gDist = 6.0;  cameraUpdate();   /* 相机复位 */
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
    /* 滚轮 dolly：3=上滚(拉近) 4=下滚(拉远) */
    else if (btn == 3) { gDist -= 0.5; if (gDist < 0.5) gDist = 0.5; cameraUpdate(); glutPostRedisplay(); }
    else if (btn == 4) { gDist += 0.5; cameraUpdate(); glutPostRedisplay(); }
}

void motion(int x, int y)
{
    if (!gRmbDown) return;
    int dx = x - gLastX, dy = y - gLastY;
    gLastX = x;  gLastY = y;
    if (dx == 0 && dy == 0) return;

    gYaw += -dx * 0.3;          /* 水平：绕 Y 轴 */
    gPitch += -dy * 0.3;          /* 垂直：绕 X 轴，clamp ±89° */
    if (gPitch > 89.0) gPitch = 89.0;
    if (gPitch < -89.0) gPitch = -89.0;

    cameraUpdate();
    glutPostRedisplay();
}

/* 每帧轮询 WASD 键状态（右键按住时生效）。
   用 GetAsyncKeyState 而非 GLUT keyboard 回调，因为后者在鼠标按钮按住时不触发 */
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

    glViewport(0, 0, w, h);              /* ① 渲染区域 = 整个窗口 */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-w / 2, w / 2, -h / 2, h / 2);    /* ② 定义一个 2D 正交画布 */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();                    /* ③ 模型视图矩阵清空（不做任何变换） */
    updateMap(w, h);                     /* ④ 你自己的映射参数跟着更新 */

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


    {
        Vec3* vv = 0;  int* ff = 0;
        if (loadObj(OBJ_PATH, 2.0, &vv, &gObjMesh.nv, &ff, &gObjMesh.nf)) {
            gObjMesh.v = vv;
            gObjMesh.face = ff;
            gObjMesh.n = 3;
            gObjMesh.model = identity();

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
