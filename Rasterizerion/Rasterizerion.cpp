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

/*///DDA 
void DDA(int x0, int y0, int x1, int y1)
{
	float dy = y1 - y0;
	float dx = x1 - x0;
	float k = dy / dx;
	float xf = x0;
	float yf = y0;

	while (xf <= x1)
	{
		int xr = (floor)(xf + 0.5);
		int yr = (floor)(yf + 0.5);
		glVertex2d(xr, yr);
		printf("(%d, %d)\n", xr, yr);

		xf = xf + 1;
		yf = yf + k;

	}
}
*/
/// <summary>
/// Bresenham
/// </summary>
/// <param name="x0"></param>
/// <param na>
/// Bresenham 直线算法
void Bresenham(int x0, int y0, int x1, int y1)
{
	int dx = x1 - x0;
	int dy = y1 - y0;
	int d = dy + dy - dx;     
	int x = x0;
	int y = y0;

	if (dx >= dy)                   // 0 <= k <= 1
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
	else                            // k > 1：y 为主方向
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
	Bresenham(2, 2, 9, 5);


	
	glEnd();
	glPopMatrix();
	glFlush();
}

//键盘交互事件
void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'x':
	case 'X':
	{
		xmove += 1;
		glutPostRedisplay();
		break;
	}
	case 'y':
	case 'Y':
	{
		ymove += 1;
		glutPostRedisplay();
		break;
	}
	case 27:
		exit(0);
		break;
	}
}

//鼠标交互事件
void mouse(int button, int state, int x, int y)
{
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN)
		{
			xmove += 1;
			glutPostRedisplay();
		}
		break;
	case GLUT_RIGHT_BUTTON:
		if (state == GLUT_DOWN)
		{
			ymove += 1;
			glutPostRedisplay();
		}
		break;
	default:
		break;
	}
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
	glutInitWindowSize(w_width,w_height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("HelloWorld");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMainLoop();
	return 0;
}
