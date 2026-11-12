#ifndef OBJLOAD_H
#define OBJLOAD_H
#include "math3d.h"

/* Minimal OBJ loader.
 * Keeps only vertex positions ("v") and faces ("f"); vt/vn are skipped.
 * Faces of any arity are fan-triangulated on load.
 * After loading, the mesh is normalized: bbox centred at the origin,
 * uniformly scaled so its largest half-extent equals targetRadius.
 *
 * On success returns 1 and hands ownership of two plain arrays:
 *   *outV : nv Vec3 vertices
 *   *outF : 3*nf int vertex indices (0-based), triangle list
 * Free with freeObjMesh(). Returns 0 on failure. */
int  loadObj(const char* path, double targetRadius,
             Vec3** outV, int* outNv, int** outF, int* outNf);
void freeObjMesh(Vec3* v, int* f);

#endif
