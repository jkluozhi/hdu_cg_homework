#ifndef OBJLOAD_H
#define OBJLOAD_H
#include "math3d.h"

/* 简易 OBJ 模型加载器。
 * 仅保留顶点位置（"v"）和面信息（"f"）；忽略纹理坐标（vt）和法线（vn）。
 * 加载时，任意边数的多边形面都会通过扇形三角剖分法转换为三角形。
 * 加载完成后，对模型进行归一化处理：
 * 包围盒中心移动到原点，并进行等比例缩放，
 * 使模型最大的半尺寸等于 targetRadius。
 *
 * 加载成功时返回 1，并将以下两个普通数组的所有权交给调用者：
 *   *outV : 包含 nv 个 Vec3 顶点的数组
 *   *outF : 包含 3*nf 个 int 顶点索引的数组（索引从 0 开始），表示三角形列表。
 * 使用 freeObjMesh() 释放内存。
 * 加载失败时返回 0。
 */
int  loadObj(const char* path, double targetRadius,
             Vec3** outV, int* outNv, int** outF, int* outNf);
void freeObjMesh(Vec3* v, int* f);

#endif
