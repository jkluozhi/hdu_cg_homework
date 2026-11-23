#define GLUT_DISABLE_ATEXIT_HACK
#include "GLUT.H"
#include <math.h>
#include "camera.h"
#include "raster.h"
#include "clip3d.h"
#include "objload.h"
#include "curves.h"



/* ============================================================

 *   a/d  平移 x     w/s  平移 y        z/x  等比缩放
 *   1/2/3 选旋转轴   q/e  绕该轴 ±10°
 *   j/l  眼睛 x     u/o  眼睛 y        i/k  眼睛 z
 *   [ / ] 改 d      p    正交 <-> 透视  r    复位
 *   h    面片填充+深度消隐 <-> 纯线框
 *   m    加载 / 隐藏 lily OBJ 模型
 *   5    在 OBJ / Bézier曲线 / B样条曲线 / Bézier曲面 / B样条曲面 间切换
 * ============================================================ */




typedef struct {
    const Vec3* v;
    int         nv;
    const int* face;
    int         nf;
    int         n;
    Vec3        color;
    Matrix      model;
    int         twoSided;   /* 1 = 不剔除背面（曲面用，双面可见） */
} Mesh;



Matrix gModel;                  /* 模型变换（累积） */
int    gAxis = 2;               /* 当前旋转轴：0=x 1=y 2=z */
int    gFill = 1;               /* 1 = 面片填充 + z-buffer 消隐；0 = 纯线框 */

#define OBJ_PATH "C:/Users/21505/Desktop/ComputerGraphics/lily-impeller.obj"
Mesh   gObjMesh;                /* 运行时加载的 OBJ 网格（m 键加载） */
int    gShowObj = 0;            /* 1 = 把 OBJ 网格加入渲染列表 */

/* 曲线/曲面：gMode 用 '5' 键在 5 个对象间轮流切换 */
int    gMode = 0;               /* 0=OBJ 1=Bézier曲线 2=B样条曲线 3=Bézier曲面 4=B样条曲面 */
#define CURVE_N 70
static Vec3  gBezPts[CURVE_N + 1], gBezCtrl[4];   /* Bézier 曲线采样点 / 控制点 */
static Vec3  gBsPts[CURVE_N + 1],  gBsCtrl[7];    /* B 样条曲线采样点 / 控制点 */
static Mesh  gBezierSurf, gBSplineSurf;           /* 两个曲面网格 */
/* w_width / w_height、gScale、裁剪窗口、updateMap 都在 raster.cpp，
   本文件通过 raster.h 的 extern 使用 */


/* ================= 曲线/曲面数据构建 ================= */
static void drawPolyline(const Vec3* pts, int n, Matrix V, float r, float g, float b)
{
    glColor3f(r, g, b);
    for (int i = 0; i + 1 < n; i++)
        drawSegment(pts[i], pts[i + 1], V);
}

typedef struct { Vec3 P[4][4]; } BezSurfAux;
static Vec3 bezSurfEval(double u, double v, void* aux)
{
    BezSurfAux* a = (BezSurfAux*)aux;
    return bezierSurfacePoint(a->P, u, v);
}

typedef struct { Vec3 P[5][5]; const double* U; const double* V; } BsSurfAux;
static Vec3 bsSurfEval(double u, double v, void* aux)
{
    BsSurfAux* a = (BsSurfAux*)aux;
    return bsplineSurfacePoint(a->P, a->U, a->V, 3, u, v);
}

static void buildSurfMesh(Mesh* m, SurfEval ev, void* aux, int Nu, int Nv,
                          Vec3 off, Vec3 color)
{
    Vec3* V = 0;  int* F = 0;
    int nf = buildSurfaceMesh(ev, aux, Nu, Nv, &V, &F);
    for (int i = 0; i < Nu * Nv; i++) {        /* 居中平移 */
        V[i].x += off.x;  V[i].y += off.y;  V[i].z += off.z;
    }
    m->v = V;  m->nv = Nu * Nv;
    m->face = F;  m->nf = nf;  m->n = 3;
    m->color = color;  m->model = identity();  m->twoSided = 1;
}




static void initCurves(void)
{
    /* ---- 三次 Bézier 曲线：控制点 (0,0)(1,2)(3,2)(4,0) ---- */
    Vec3 bp[4] = { {0,0,0},{1,2,0},{3,2,0},{4,0,0} };
    Vec3 offB = { -2, -1, 0 };                 /* 居中偏移：包围盒中心 (2,1,0) */
    for (int i = 0; i < 4; i++) {              
        gBezCtrl[i].x = bp[i].x + offB.x;
        gBezCtrl[i].y = bp[i].y + offB.y;
        gBezCtrl[i].z = bp[i].z + offB.z;
    }
    for (int i = 0; i <= CURVE_N; i++) {
        double t = (double)i / CURVE_N;
        double b0 = (1 - t) * (1 - t) * (1 - t);   
        double b1 = 3 * t * (1 - t) * (1 - t);     
        double b2 = 3 * t * t * (1 - t);           
        double b3 = t * t * t;                     

        gBezPts[i].x = b0 * bp[0].x + b1 * bp[1].x + b2 * bp[2].x + b3 * bp[3].x + offB.x;
        gBezPts[i].y = b0 * bp[0].y + b1 * bp[1].y + b2 * bp[2].y + b3 * bp[3].y + offB.y;
        gBezPts[i].z = 0.0;                        /* 这条曲线全程 z=0 */
    }


    /* ---- 三次夹持 B 样条曲线：7 个控制点，节点 U ---- */
    Vec3 cp[7] = { {0,0,0},{1,2,0},{2,3,0},{3,2,0},{4,3,0},{5,1,0},{6,0,0} };
    double Uc[11] = { 0,0,0,0,0.25,0.5,0.75,1,1,1,1 };
    Vec3 offC = { -3, -1.5, 0 };
    for (int i = 0; i < 7; i++) {
        gBsCtrl[i].x = cp[i].x + offC.x;
        gBsCtrl[i].y = cp[i].y + offC.y;
        gBsCtrl[i].z = cp[i].z + offC.z;
    }
    for (int i = 0; i <= CURVE_N; i++) {
        double t = (double)i / CURVE_N;
        Vec3 p = bsplinePoint(cp, 6, Uc, 3, t);
        gBsPts[i].x = p.x + offC.x;
        gBsPts[i].y = p.y + offC.y;
        gBsPts[i].z = p.z + offC.z;
    }

    /* ---- 双三次 Bézier 曲面：Pij=(i,j,hij)，仅 h11=4 ---- */
    BezSurfAux ba;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            ba.P[i][j].x = (double)i;
            ba.P[i][j].y = (double)j;
            ba.P[i][j].z = (i == 1 && j == 1) ? 4.0 : 0.0;
        }
    Vec3 offS1 = { -1.5, -1.5, -2.0 };
    Vec3 colS1 = { 0.25, 0.55, 0.30 };
    buildSurfMesh(&gBezierSurf, bezSurfEval, &ba, 24, 24, offS1, colS1);

    /* ---- 双三次夹持 B 样条曲面：Pij=(i,j,hij)，仅 h22=4 ---- */
    const double Us[9] = { 0,0,0,0,0.5,1,1,1,1 };
    const double Vs[9] = { 0,0,0,0,0.6,1,1,1,1 };
    BsSurfAux sa;
    sa.U = Us;  sa.V = Vs;
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++) {
            sa.P[i][j].x = (double)i;
            sa.P[i][j].y = (double)j;
            sa.P[i][j].z = (i == 2 && j == 2) ? 4.0 : 0.0;
        }
    Vec3 offS2 = { -2, -2, -2 };
    Vec3 colS2 = { 0.75, 0.45, 0.15 };
    buildSurfMesh(&gBSplineSurf, bsSurfEval, &sa, 24, 24, offS2, colS2);
}

/* ================= 显示 ================= */
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    zbufClear();                            /* 深度缓冲清零 */

    Matrix V = viewMatrix();

    /* 本帧参与渲染的网格：按 gMode 选择 OBJ 或某个曲面 */
    Mesh* active[1];
    int   nActive = 0;
    switch (gMode) {
    case 0: if (gShowObj && gObjMesh.nv > 0) active[nActive++] = &gObjMesh; break;
    case 3: active[nActive++] = &gBezierSurf;  break;
    case 4: active[nActive++] = &gBSplineSurf; break;
    default: break;   /* 曲线模式（1/2）走下面的折线绘制 */
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
            Vec3 cv[16];
            int nc = clipFrustum(vv, mesh->n, cv, 16);
            if (nc < 3) continue;        

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

            
            Vec3 col = mesh->color;

            /* 6. z-buffer 逐像素深度测试 */
            if (gFill) {
                for (int i = 1; i + 1 < nc; i++) {
                    double tx[3] = { sx[0], sx[i], sx[i + 1] };
                    double ty[3] = { sy[0], sy[i], sy[i + 1] };
                    double tz[3] = { zv[0], zv[i], zv[i + 1] };
                    fillTriangleDepth(tx, ty, tz, col);
                }
            }

            /* 7. 描边（OBJ 面太多，跳过描边） */
            if (mesh != &gObjMesh) {
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
    } else if (gMode == 2) {
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

        /* 曲线/曲面切换：OBJ -> Bézier曲线 -> B样条曲线 -> Bézier曲面 -> B样条曲面 */
    case '5':
        gMode = (gMode + 1) % 5;
        gModel = identity();   /* 切换物体时复位累积变换 */
        break;

        /* 填充/消隐开关 */
    case 'h': gFill = !gFill; break;

        /* 加载 / 隐藏 OBJ 模型 */
    case 'm': {
        if (gObjMesh.nv == 0) {                 /* 第一次按：读文件 */
            Vec3* vv = 0;  int* ff = 0;
            if (loadObj(OBJ_PATH, 2.0, &vv, &gObjMesh.nv, &ff, &gObjMesh.nf)) {
                gObjMesh.v    = vv;
                gObjMesh.face = ff;
                gObjMesh.n    = 3;
                gObjMesh.color.x = 0.75;  gObjMesh.color.y = 0.55;  gObjMesh.color.z = 0.30;
                gObjMesh.model = identity();
                gShowObj = 1;
            }
        } else {
            gShowObj = !gShowObj;               /* 之后按：显示 <-> 隐藏 */
        }
        break;
    }

        /* 复位 */
    case 'r':
        gModel = identity();
        gEye.x = 0;  gEye.y = 0;  gEye.z = 6;
        gCenter.x = 0;  gCenter.y = 0;  gCenter.z = 0;
        gUp.x = 0;  gUp.y = 1;  gUp.z = 0;
        gD = -2.0;  gPersp = 0;  gAxis = 2;  gFill = 1;
        break;

    default: return;
    }

    gModel = matrixMut(M, gModel);   /* 左乘累积 */
    glutPostRedisplay();
}

/* ================= 投影设置 ================= */
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

    /* 启动时直接加载 OBJ 模型（失败则之后可按 m 重试） */
    {
        Vec3* vv = 0;  int* ff = 0;
        if (loadObj(OBJ_PATH, 2.0, &vv, &gObjMesh.nv, &ff, &gObjMesh.nf)) {
            gObjMesh.v    = vv;
            gObjMesh.face = ff;
            gObjMesh.n    = 3;
            gObjMesh.color.x = 0.75;  gObjMesh.color.y = 0.55;  gObjMesh.color.z = 0.30;
            gObjMesh.model = identity();
            gShowObj = 1;
        }
    }

    initCurves();           /* 构建两条曲线与两个曲面的几何数据 */

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
