#define GLUT_DISABLE_ATEXIT_HACK
#include "GLUT.H"
#include <math.h>
#include "camera.h"
#include "raster.h"
#include "clip3d.h"
#include "objload.h"



/* ============================================================

 *   a/d  平移 x     w/s  平移 y        z/x  等比缩放
 *   1/2/3 选旋转轴   q/e  绕该轴 ±10°
 *   j/l  眼睛 x     u/o  眼睛 y        i/k  眼睛 z
 *   [ / ] 改 d      p    正交 <-> 透视  r    复位
 *   h    面片填充+深度消隐 <-> 纯线框
 *   m    加载 / 隐藏 lily OBJ 模型
 * ============================================================ */




typedef struct {
    const Vec3* v;
    int         nv;
    const int* face;
    int         nf;
    int         n;
    Vec3        color;
    Matrix      model;
} Mesh;



Matrix gModel;                  /* 模型变换（累积） */
int    gAxis = 2;               /* 当前旋转轴：0=x 1=y 2=z */
int    gFill = 1;               /* 1 = 面片填充 + z-buffer 消隐；0 = 纯线框 */

#define OBJ_PATH "C:/Users/21505/Desktop/ComputerGraphics/lily-impeller.obj"
Mesh   gObjMesh;                /* 运行时加载的 OBJ 网格（m 键加载） */
int    gShowObj = 0;            /* 1 = 把 OBJ 网格加入渲染列表 */

/* w_width / w_height、gScale、裁剪窗口、updateMap 都在 raster.cpp，
   本文件通过 raster.h 的 extern 使用 */


/* ================= 显示 ================= */
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    zbufClear();                            /* 深度缓冲清零 */

    Matrix V = viewMatrix();

    /* 本帧参与渲染的网格：只有（已加载且可见的）OBJ 模型 */
    Mesh* active[1];
    int   nActive = 0;
    if (gShowObj && gObjMesh.nv > 0) active[nActive++] = &gObjMesh;

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
            if (area <= 0.0) continue;

            
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

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
