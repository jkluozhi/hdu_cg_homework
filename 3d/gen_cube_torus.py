# -*- coding: utf-8 -*-
"""Generate cube + torus + ground plane OBJ file, readable by objload.cpp.
objload.cpp only reads 'v' and 'f' lines (ignores vt/vn), so we emit plain
'f a b c' triangles with 1-based indices. Face winding is validated so normals
point outward (avoids back-face culling holes). The ground plane is single-sided
with its normal pointing +Y (top face only)."""
import math

OUT = "C:/Users/21505/Desktop/ComputerGraphics/cube_torus.obj"

# ground plane: below the objects, large enough to cover both
PLANE_Y = -1.2
PX, PZ = 3.5, 3.3


def sub(a, b):
    return (a[0]-b[0], a[1]-b[1], a[2]-b[2])


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def dot(a, b):
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]


def add(a, b):
    return (a[0]+b[0], a[1]+b[1], a[2]+b[2])


def mul(a, s):
    return (a[0]*s, a[1]*s, a[2]*s)


verts = []   # list of (x, y, z)
tris = []    # list of (i0, i1, i2) 0-based

# ---------------- cube ----------------
cx, cy, cz = -2.0, 0.0, 0.0   # cube centre
h = 1.0                        # half side length
cv = [
    (cx-h, cy-h, cz-h),  # 0
    (cx+h, cy-h, cz-h),  # 1
    (cx+h, cy+h, cz-h),  # 2
    (cx-h, cy+h, cz-h),  # 3
    (cx-h, cy-h, cz+h),  # 4
    (cx+h, cy-h, cz+h),  # 5
    (cx+h, cy+h, cz+h),  # 6
    (cx-h, cy+h, cz+h),  # 7
]
base = len(verts)
verts.extend(cv)
# quads (CCW seen from outside), fan into 2 triangles
cube_quads = [
    (4, 5, 6, 7),   # front +z
    (1, 0, 3, 2),   # back -z
    (5, 1, 2, 6),   # right +x
    (0, 4, 7, 3),   # left -x
    (7, 6, 2, 3),   # top +y
    (4, 0, 1, 5),   # bottom -y
]
for (a, b, c, d) in cube_quads:
    tris.append((base+a, base+b, base+c))
    tris.append((base+a, base+c, base+d))

# ---------------- torus ----------------
tx, ty, tz = 2.0, 0.0, 0.0    # torus centre
R = 1.0                       # major radius
r = 0.4                       # tube radius
N = 64                        # segments around main ring
M = 24                        # segments around tube
tbase = len(verts)
for i in range(N):
    th = i * 2.0 * math.pi / N
    for j in range(M):
        ph = j * 2.0 * math.pi / M
        rc = R + r * math.cos(ph)          # distance from ring axis
        x = rc * math.cos(th)
        y = r * math.sin(ph)
        z = rc * math.sin(th)
        verts.append((tx + x, ty + y, tz + z))
for i in range(N):
    for j in range(M):
        i1 = (i + 1) % N
        j1 = (j + 1) % M
        a = tbase + i * M + j
        b = tbase + i * M + j1
        c = tbase + i1 * M + j1
        d = tbase + i1 * M + j
        tris.append((a, b, c))
        tris.append((a, c, d))

# ---------------- fix winding so every normal points outward ----------------
def outward_ref(t):
    """reference outward direction for a triangle's centroid"""
    g = mul(add(add(verts[t[0]], verts[t[1]]), verts[t[2]]), 1.0 / 3.0)
    # cube: away from cube centre
    # torus: away from nearest ring-axis point
    if g[0] < 0.0:                      # cube is at x<0
        return sub(g, (cx, cy, cz))
    # torus at x>0
    dx, dz = g[0] - tx, g[2] - tz
    d = math.hypot(dx, dz)
    if d < 1e-9:
        return (0.0, 1.0, 0.0)
    ring_pt = (tx + R * dx / d, ty, tz + R * dz / d)
    return sub(g, ring_pt)

fixed = 0
for k, t in enumerate(tris):
    n = cross(sub(verts[t[1]], verts[t[0]]), sub(verts[t[2]], verts[t[0]]))
    ref = outward_ref(t)
    if dot(n, ref) < 0.0:
        tris[k] = (t[0], t[2], t[1])
        fixed += 1

# ---------------- ground plane (single-sided, normal +Y) ----------------
# Ordered CCW as seen from above (+Y): A(-PX,-PZ) -> B(-PX,+PZ)
# -> C(+PX,+PZ) -> D(+PX,-PZ). Cross(A->B, A->C) = (0, +4*PX*PZ, 0).
pbase = len(verts)
verts.extend([
    (-PX, PLANE_Y, -PZ),
    (-PX, PLANE_Y,  PZ),
    ( PX, PLANE_Y,  PZ),
    ( PX, PLANE_Y, -PZ),
])
tris.append((pbase + 0, pbase + 1, pbase + 2))
tris.append((pbase + 0, pbase + 2, pbase + 3))

# ---------------- write OBJ ----------------
with open(OUT, "w") as f:
    f.write("# cube + torus + ground plane\n")
    for v in verts:
        f.write("v %.6f %.6f %.6f\n" % v)
    for (a, b, c) in tris:
        f.write("f %d %d %d\n" % (a + 1, b + 1, c + 1))

print("wrote", OUT)
print("vertices =", len(verts))
print("triangles =", len(tris))
print("winding flipped =", fixed)

# ---------------- verify as objload.cpp would read it ----------------
nv = 0
nf = 0
max_idx = 0
bad = 0
with open(OUT) as f:
    for line in f:
        tok = line.split()
        if not tok:
            continue
        if tok[0] == "v":
            nv += 1
        elif tok[0] == "f":
            for t in tok[1:]:
                vi = int(t.split("/")[0]) - 1   # 1-based -> 0-based
                if vi < 0 or vi >= nv:
                    bad += 1
                if vi > max_idx:
                    max_idx = vi
            nf += 1
pn = cross(sub(verts[pbase + 1], verts[pbase + 0]),
           sub(verts[pbase + 2], verts[pbase + 0]))
print("verify: nv=%d nf=%d max_index=%d bad_index=%d" % (nv, nf, max_idx, bad))
print("plane normal = (%.3f, %.3f, %.3f)  top face visible: %s"
      % (pn[0], pn[1], pn[2], pn[1] > 0))
print("OK" if bad == 0 and pn[1] > 0 else "ERROR")
