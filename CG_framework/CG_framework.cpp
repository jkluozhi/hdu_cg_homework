#define GLUT_DISABLE_ATEXIT_HACK
#include "GLUT.H"

#define ZVALUE 20.0f
float xmove = 0;
float ymove = 0;
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

//绘制内容
void display(void)
{
	glClearColor(1.f, 1.f, 1.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);

	draw_coordinate(); //绘制坐标系 

	glColor3f(0, 0, 1); // 
	glPushMatrix();
	glTranslatef(xmove, ymove, 0);
	glBegin(GL_QUADS); //GL_LINE_LOOP
	glVertex2f(0, 0);
	glVertex2f(0, -3);
	glVertex2f(10, -3);
	glVertex2f(10, 0);
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
	//在默认情况下，视口被设置为占据打开窗口的整个像素矩形，窗口大小和设置视口大小相同，
	//所以为了选择一个更小的绘图区域，就可以用glViewport函数来实现这一变换，在窗口中定义一个像素矩形，
	//最终将图像映射到这个矩形中。例如可以对窗口区域进行划分，在同一个窗口中显示分割屏幕的效果，以显示多个视图。
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glMatrixMode(GL_PROJECTION);//设置投影模式以及视景体大小——投影变换
	glLoadIdentity();
	if (w <= h)
		glOrtho(-16.0, 16.0, -16.0 * (GLfloat)h / (GLfloat)w, 16.0 * (GLfloat)h / (GLfloat)w,
			-ZVALUE, ZVALUE);
	else
		glOrtho(-16.0 * (GLfloat)h / (GLfloat)w, 16.0 * (GLfloat)h / (GLfloat)w, -16.0, 16.0,
			-ZVALUE, ZVALUE);

	glMatrixMode(GL_MODELVIEW); //视点变换
	glLoadIdentity();
}

//主调函数
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowSize(600, 600);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("HelloWorld");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMainLoop();
	return 0;
}
