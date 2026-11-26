#include "objload.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

int loadObj(const char* path, double targetRadius,
            Vec3** outV, int* outNv, int** outF, int* outNf)
{
    std::ifstream file(path);
    if (!file.is_open()) return 0;

    std::vector<Vec3> verts;
    std::vector<int>  faces;             
    std::string line;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            Vec3 p;
            ss >> p.x >> p.y >> p.z;
            verts.push_back(p);
        }
        else if (type == "f") {
            /* tokens look like "2/2/2" -- keep the part before '/' (1-based) */
            int idx[64], n = 0;
            std::string tok;
            while (ss >> tok && n < 64) {
                int vi = 0;
                for (size_t k = 0; k < tok.size() && tok[k] != '/'; k++)
                    vi = vi * 10 + (tok[k] - '0');
                idx[n++] = vi - 1;        /* OBJ indices are 1-based */
            }
            for (int i = 1; i + 1 < n; i++) {   /* fan triangulation */
                faces.push_back(idx[0]);
                faces.push_back(idx[i]);
                faces.push_back(idx[i + 1]);
            }
        }
        /* "vt" / "vn" / "mtllib" / ... : ignored */
    }

    if (verts.empty() || faces.empty()) return 0;

    /* ---- normalize: centre bbox at origin, scale to targetRadius ---- */
    Vec3 mn = verts[0], mx = verts[0];
    for (size_t i = 1; i < verts.size(); i++) {
        const Vec3& p = verts[i];
        if (p.x < mn.x) mn.x = p.x;  if (p.x > mx.x) mx.x = p.x;
        if (p.y < mn.y) mn.y = p.y;  if (p.y > mx.y) mx.y = p.y;
        if (p.z < mn.z) mn.z = p.z;  if (p.z > mx.z) mx.z = p.z;
    }
    Vec3 c = { (mn.x + mx.x) / 2, (mn.y + mx.y) / 2, (mn.z + mx.z) / 2 };
    double r = mx.x - mn.x;
    if (mx.y - mn.y > r) r = mx.y - mn.y;
    if (mx.z - mn.z > r) r = mx.z - mn.z;
    double s = (r > 1e-12) ? (2.0 * targetRadius) / r : 1.0;   /* r is full extent here */
    for (size_t i = 0; i < verts.size(); i++) {
        verts[i].x = (verts[i].x - c.x) * s;
        verts[i].y = (verts[i].y - c.y) * s;
        verts[i].z = (verts[i].z - c.z) * s;
    }

    /* ---- hand out plain arrays ---- */
    size_t nv = verts.size(), nf = faces.size() / 3;
    Vec3* va = new Vec3[nv];
    int*  fa = new int[faces.size()];
    for (size_t i = 0; i < nv; i++) va[i] = verts[i];
    for (size_t i = 0; i < faces.size(); i++) fa[i] = faces[i];

    *outV = va;  *outNv = (int)nv;
    *outF = fa;  *outNf = (int)nf;
    return 1;
}

void freeObjMesh(Vec3* v, int* f)
{
    delete[] v;
    delete[] f;
}
