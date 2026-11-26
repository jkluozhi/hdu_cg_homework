#ifndef RASTER_H
#define RASTER_H
#include"camera.h"
#include "math3d.h"

extern double gScale;
extern double gClipX0, gClipY0, gClipX1, gClipY1;
extern double gWinX, gWinY;                 /* 投影窗口半宽/半高（世界单位） */
extern Matrix gModel;
extern int    w_width, w_height;

void toPixels(double xp, double yp, double* sx, double* sy);
int  iround(double v);
int  clipLine(double* x0, double* y0, double* x1, double* y1);
void drawLine(int x0, int y0, int x1, int y1);
void drawSegment(Vec3 a, Vec3 b, Matrix V);      /* 受模型矩阵影响（物体用） */
void drawWorldSegment(Vec3 a, Vec3 b, Matrix V); /* 不受模型矩阵影响（坐标轴用） */

void updateMap(int w, int h);

/* z-buffer（每像素存视图空间 z）+ 带深度测试的三角形填充 */
extern int gZbufOn;      /* 1 = 深度测试；0 = 关闭 z-buffer（画家算法，后画盖先画） */
void zbufClear(void);
void fillTriangleDepth(const double sx[3], const double sy[3],
                       const double zv[3], Vec3 color);
#endif
