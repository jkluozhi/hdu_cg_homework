#define GLUT_DISABLE_ATEXIT_HACK
#include "GLUT.H"
#include <math.h>
#include <stdio.h>

#define ZVALUE 20.0f
#define MAXV   64     /* 多边形最大顶点数 */
#define MAXXS  64     /* 单条扫描线最多交点数 */

float xmove = 0;
float ymove = 0;

int w_width = 1000;
int w_height = 1000;

typedef struct { int x, y; } Point;

/* ================= 坐标系 ================= */
void draw_coordinate(void)
{
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(15.0f, 0.0f);
    glVertex2f(-15.0f, 0.0f);

    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex2f(0.0f, 15.0f);
    glVertex2f(0.0f, -15.0f);
    glEnd();
}

/* ================= Bresenham（保留你原来的实现） ================= */
void Bresenham(int x0, int y0, int x1, int y1)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int x = x0;
    int y = y0;

    if (dx >= dy)                       /* 0 <= k <= 1 */
    {
        int d = 2 * dy - dx;
        for (int i = 0; i <= dx; i++)
        {
            glVertex2i(x, y);
            printf("(%d, %d)\n", x, y);
            if (d >= 0) { y += 1; d -= 2 * dx; }
            d += 2 * dy;
            x += 1;
        }
    }
    else                                /* k > 1 */
    {
        int d = 2 * dx - dy;
        for (int i = 0; i <= dy; i++)
        {
            glVertex2i(x, y);
            printf("(%d, %d)\n", x, y);
            if (d >= 0) { x += 1; d -= 2 * dy; }
            d += 2 * dx;
            y += 1;
        }
    }
}

/* ============================================================
 * 扫描线多边形填充（不使用 malloc，全部用栈上定长数组）
 * poly : 顶点数组（整数坐标，首尾自动相连）
 * n    : 顶点个数
 * 说明 : 内部用 glBegin(GL_POINTS) 里逐点 glVertex2i 绘制，
 *        同时 printf 输出每个像素坐标。
 * ============================================================ */
void fillPolygon(const Point* poly, int n)
{
    if (poly == NULL || n < 3) return;

    /* ---- 1. 求 y 范围 ---- */
    int ymin = poly[0].y;
    int ymax = poly[0].y;
    for (int i = 1; i < n; i++)
    {
        if (poly[i].y < ymin) ymin = poly[i].y;
        if (poly[i].y > ymax) ymax = poly[i].y;
    }

    /* ---- 2. 逐条扫描线求交点、排序、配对填充 ---- */
    glBegin(GL_POINTS);
    for (int y = ymin; y <= ymax; y++)
    {
        double xs[MAXXS];
        int    cnt = 0;

        /* 2.1 收集与 y 扫描线相交的所有非水平边的 x 交点 */
        for (int i = 0; i < n; i++)
        {
            int x1 = poly[i].x, y1 = poly[i].y;
            int x2 = poly[(i + 1) % n].x, y2 = poly[(i + 1) % n].y;

            if (y1 == y2) continue;                 /* 忽略水平边 */

            /* 保证 (xlo, ylo) 是下端 */
            int ylo, yhi, xlo;
            if (y1 < y2) { ylo = y1; yhi = y2; xlo = x1; }
            else { ylo = y2; yhi = y1; xlo = x2; }

            double dxdy = (double)(x2 - x1) / (double)(y2 - y1);

            /* 半开区间 [ylo, yhi)：上端点不计，避免顶点被数两次 */
            if (y >= ylo && y < yhi)
            {
                if (cnt < MAXXS)
                    xs[cnt++] = xlo + (y - ylo) * dxdy;
            }
        }

        /* 2.2 交点按 x 升序排序（简单冒泡，点数很少） */
        for (int i = 0; i < cnt - 1; i++)
            for (int j = i + 1; j < cnt; j++)
                if (xs[i] > xs[j])
                {
                    double t = xs[i];
                    xs[i] = xs[j];
                    xs[j] = t;
                }

        /* 2.3 两两配对填充（含左右端点） */
        for (int i = 0; i + 1 < cnt; i += 2)
        {
            int xa = (int)floor(xs[i] + 0.5);   /* 等价 round */
            int xb = (int)floor(xs[i + 1] + 0.5);
            for (int x = xa; x <= xb; x++)
            {
                glVertex2i(x, y);
                printf("(%d, %d)\n", x, y);
            }
        }
    }
    glEnd();
}

/* ================= 绘制内容 ================= */
void display(void)
{
    glClearColor(1.f, 1.f, 1.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    draw_coordinate();              /* 画坐标轴 */

    /* 定义多边形（放在窗口中心附近，方便观察） */
    Point poly[] = {
        {  2,  2 },
        {  9,  3 },
        {  7,  8 },
        {  4, 10 },
        {  1,  6 }
    };
    int n = (int)(sizeof(poly) / sizeof(poly[0]));

    glPushMatrix();
    glTranslatef(xmove, ymove, 0);

    /* ① 填充：蓝色点 */
    glColor3f(0.0f, 0.0f, 1.0f);
    fillPolygon(poly, n);

    /* ② 边框：红色线条，方便看清多边形的范围 */
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < n; i++)
        glVertex2i(poly[i].x, poly[i].y);
    glEnd();

    glPopMatrix();
    glFlush();
}

/* ================= 键盘交互 ================= */
void keyboard(unsigned char key, int x, int y)
{
    (void)x; (void)y;
    switch (key)
    {
    case 'x': case 'X': xmove += 1; glutPostRedisplay(); break;
    case 'y': case 'Y': ymove += 1; glutPostRedisplay(); break;
    case 27:            exit(0);                           break;
    }
}

/* ================= 鼠标交互 ================= */
void mouse(int button, int state, int x, int y)
{
    (void)x; (void)y;
    if (state != GLUT_DOWN) return;

    switch (button)
    {
    case GLUT_LEFT_BUTTON:  xmove += 1; glutPostRedisplay(); break;
    case GLUT_RIGHT_BUTTON: ymove += 1; glutPostRedisplay(); break;
    default: break;
    }
}

/* ================= 投影设置 ================= */
void reshape(int w, int h)
{
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-0.05 * w_width, 0.05 * w_width,
        -0.05 * w_height, 0.05 * w_height,
        -ZVALUE, ZVALUE);

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
    glutCreateWindow("Scanline Fill");

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);

    glutMainLoop();
    return 0;
}