#define GLUT_DISABLE_ATEXIT_HACK
#include "My_Model3D.h"
#include "My_View_Transform.h"
#include "My_Projection3D.h"
#include "My_Frustum_Clip.h"
#include "My_3D_Display.h"

#include "GLUT.H"

int nearplane_width = 600, nearplane_height = 600;
 

void display()
{
    // 1. 模型的世界坐标由 keyboard() 中的模型变换更新。
    // 2. 视点变换：只产生本帧的临时顶点，不改原模型。
    

    // 3. 自编六平面裁剪，得到本帧新生成的多边形集合。
   

    // 4. 只投影裁剪结果；S 切换固定外部观察模式，裁剪仍依据当前视点。
    
    // 5. 继续用原来的 Bresenham / GL_POINTS 显示。
    glClearColor(1,1,1,0);
    glClear(GL_COLOR_BUFFER_BIT);
    

    glutSwapBuffers();
}

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
}

void keyboard(unsigned char key, int, int)
{
    my_homogeneous_point* points=model.vertices.data();
    const int count=int(model.vertices.size());
    switch (key)
    {
    
    default: return;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(nearplane_width,nearplane_height);
    glutInitWindowPosition(100,100);
    glutCreateWindow("Manual View-Frustum Clipping");
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
