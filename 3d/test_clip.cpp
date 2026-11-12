/* clipFrustum smoke test: console only, no GLUT window needed */
#include <stdio.h>
#include <math.h>
#include "math3d.h"
#include "camera.h"
#include "raster.h"
#include "clip3d.h"

Matrix gModel;   /* raster.cpp references it; unused here */

static void dump(const char* tag, const Vec3* v, int n)
{
    printf("  %s n=%d:", tag, n);
    for (int i = 0; i < n; i++)
        printf("  (%.3g,%.3g,%.3g)", v[i].x, v[i].y, v[i].z);
    printf("\n");
}

int main(void)
{
    Vec3 out[16];
    int n;

    /* default camera state: d=-2, window 4x3 */
    gEye.x = 0; gEye.y = 0; gEye.z = 6;
    gCenter.x = 0; gCenter.y = 0; gCenter.z = 0;
    gUp.x = 0; gUp.y = 1; gUp.z = 0;
    gD = -2.0;

    /* C1: perspective, quad with two verts BEHIND near plane (z=+1),
           two inside (z=-5) -> intersections must lie at z=-2 */
    gPersp = 1;
    {
        Vec3 q[4] = { {-1,-1, 1}, {1,-1, 1}, {1,1,-5}, {-1,1,-5} };
        n = clipFrustum(q, 4, out, 16);
        printf("C1 persp near-crossing quad\n");
        dump("clipped", out, n);
        int okz = 1;
        for (int i = 0; i < n; i++) if (out[i].z > gD + 1e-9) okz = 0;
        printf("  all z <= d=-2 ? %s\n\n", okz ? "YES" : "NO");
    }

    /* C2: perspective, quad far outside the left side plane -> empty */
    {
        Vec3 q[4] = { {-40,-1,-5}, {-30,-1,-5}, {-30,1,-5}, {-40,1,-5} };
        n = clipFrustum(q, 4, out, 16);
        printf("C2 persp fully left of window -> n=%d (expect 0)\n\n", n);
    }

    /* C3: perspective, quad crossing the left edge of the window:
           at z=-2 the window edge is x=-4; x=-6 half must be cut */
    {
        Vec3 q[4] = { {-6,-1,-2}, {-2,-1,-2}, {-2,1,-2}, {-6,1,-2} };
        n = clipFrustum(q, 4, out, 16);
        printf("C3 persp left-edge crossing at z=d\n");
        dump("clipped", out, n);
        printf("  expect min x = -4 at z=-2 (side plane through eye)\n\n");
    }

    /* C4: ortho, quad |x|<=6, z=-5 -> clipped to x=+-4 box */
    gPersp = 0;
    {
        Vec3 q[4] = { {-6,-2,-5}, {6,-2,-5}, {6,2,-5}, {-6,2,-5} };
        n = clipFrustum(q, 4, out, 16);
        printf("C4 ortho box clip\n");
        dump("clipped", out, n);
        printf("  expect x in [-4,4], y in [-2,2]\n\n");
    }

    /* C5: ortho, entirely beyond far plane -> empty */
    {
        Vec3 q[4] = { {-1,-1,-2000}, {1,-1,-2000}, {1,1,-2000}, {-1,1,-2000} };
        n = clipFrustum(q, 4, out, 16);
        printf("C5 beyond far plane -> n=%d (expect 0)\n\n", n);
    }

    /* C6: perspective, the app's real box front face (z=+1 face after
           view transform) must survive with 4 verts */
    gPersp = 1;
    {
        Vec3 q[4] = { {-2,-1,-5}, {2,-1,-5}, {2,1,-5}, {-2,1,-5} };
        n = clipFrustum(q, 4, out, 16);
        printf("C6 fully inside quad -> n=%d (expect 4)\n", n);
        dump("clipped", out, n);
    }
    return 0;
}
