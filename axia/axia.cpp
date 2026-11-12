#define GLUT_DISABLE_ATEXIT_HACK
#include "GLUT.H"
#include<math.h>
#define ZVALUE 20.0f
#include <stdio.h>

float xmove = 0;
float ymove = 0;


int w_width = 1000;
int w_height = 1000;
/**/
//绘制坐标系
void draw_coordinate()
{
	glBegin(GL_LINES);
	glColor3f(1.0, 0.0, 0.0); //设置接下来显示的颜色
	glVertex2f(15.0, 0.0);
	glVertex2f(-15.0, 0.0);

	glColor3f(0.0, 1.0, 0.0);
	glVertex2f(0.0, 15.0);
	glVertex2f(0.0, -15.0);
	glEnd();
}

void axia(int a, int b)
{	
	glVertex2i(0, b);
	int dx = 0; int dy = b;
	while(dx <= a && dy >=0)
	{
		if (b * b * dx < a * a * dy)   ///|k| < 1
		{
			dx += 1; float midy = dy - 0.5;
			int d = b * b * dx * dx + a * a * midy * midy - a * a * b * b;
			if (d > 0)
			{
				glVertex2i(dx, dy);
				printf("(%d,%d)\n", dx, dy);
			}
			else
			{
				dy -= 1;
				glVertex2i(dx, dy);
				printf("(%d,%d)\n", dx, dy);
			}
		}
		else
		{
			dy -= 1; float midx = dx + 0.5;
			int d = b * b * midx * midx + a * a * dy * dy - a * a * b * b;
			if (d > 0)
			{
				glVertex2i(dx, dy);
				printf("(%d,%d)\n", dx, dy);
			}
			else
			{
				dx += 1;
				glVertex2i(dx, dy);
				printf("(%d,%d)\n", dx, dy);
			}
		}

	}

}



//绘制内容
void display(void)
{
	glClearColor(1.f, 1.f, 1.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);

	draw_coordinate(); //绘制坐标系 

	glColor3f(0, 0, 1); // 
	glPushMatrix();
	glTranslatef(xmove, ymove, 0);
	glBegin(GL_POINTS); //GL_LINE_LOOP
	axia(6, 4);



	glEnd();
	glPopMatrix();
	glFlush();
}





//投影方式、modelview方式等设置
void reshape(int w, int h)
{
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);  // 用传入的 w/h，别用全局变量

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// 保持简单：以窗口中心为原点，视野范围等于窗口像素尺寸
	glOrtho(-0.05 * w_width, 0.05 * w_width,
		-0.05 * w_height, 0.05 * w_height,
		-ZVALUE, ZVALUE);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}



//主调函数
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowSize(w_width, w_height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("HelloWorld");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutMainLoop();
	return 0;
}
