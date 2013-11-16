#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>
#include <iostream>
#include <mmsystem.h>

#include "raaCamera/raaCamera.h"
#include "raaUtilities/raaUtilities.h"
#include "raaMaths/raaMaths.h"
#include "raaMaths/raaVector.h"
#include "raaMaths/raaMatrix.h"

raaCameraInput g_Input;
raaCamera g_Camera;
unsigned long g_ulGrid=0;
int g_aiLastMouse[2];
int g_aiStartMouse[2];
bool g_bExplore=false;
bool g_bFly=false;

void gridInit();
void display();
void idle();
void reshape(int iWidth, int iHeight);
void keyboard(unsigned char c, int iXPos, int iYPos);
void keyboardUp(unsigned char c, int iXPos, int iYPos);
void sKeyboard(int iC, int iXPos, int iYPos);
void sKeyboardUp(int iC, int iXPos, int iYPos);
void mouse(int iKey, int iEvent, int iXPos, int iYPos);
void mouseMotion();
void myInit();

unsigned int g_uiLastTime=0;

void display()
{
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

	glLoadIdentity();

	camApply(g_Camera);

	gridDraw(g_ulGrid);

	glPushMatrix();

	// this is a placeholder, you should replace it with instructions to draw your planet system
	drawSphere(20.0f, 20, 20);

	glPopMatrix();
	glFlush(); 

	glutSwapBuffers();
}
void idle()
{
	mouseMotion();
	glutPostRedisplay();
}

void reshape(int iWidth, int iHeight)
{
	glViewport(0, 0, iWidth, iHeight);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30.0f, ((float)iWidth)/((float)iHeight), 1000.1f, 1000000.0f);
	glMatrixMode(GL_MODELVIEW);
	glutPostRedisplay();
}

void keyboard(unsigned char c, int iXPos, int iYPos)
{
	switch(c)
	{
	case 'w':
		camInputTravel(g_Input, tri_pos);
		break;
	case 's':
		camInputTravel(g_Input, tri_neg);
		break;
	}
}

void keyboardUp(unsigned char c, int iXPos, int iYPos)
{
	switch(c)
	{

	case 'w':
	case 's':
		camInputTravel(g_Input, tri_null);
		break;
	case 'f':
		camInputFly(g_Input, !g_Input.m_bFly);
		break;
	}
}

void sKeyboard(int iC, int iXPos, int iYPos)
{
	switch(iC)
	{
	case GLUT_KEY_UP:
		camInputTravel(g_Input, tri_pos);
		break;
	case GLUT_KEY_DOWN:
		camInputTravel(g_Input, tri_neg);
		break;
	}
}

void sKeyboardUp(int iC, int iXPos, int iYPos)
{
	switch(iC)
	{
	case GLUT_KEY_UP:
	case GLUT_KEY_DOWN:
		camInputTravel(g_Input, tri_null);
		break;
	}
}

void mouse(int iKey, int iEvent, int iXPos, int iYPos)
{
	if(iKey==GLUT_LEFT_BUTTON && iEvent==GLUT_DOWN)
	{
		camInputMouse(g_Input, true);
		camInputSetMouseStart(g_Input, iXPos, iYPos);
	}
	else if(iKey==GLUT_LEFT_BUTTON && iEvent==GLUT_UP)
	{
		camInputMouse(g_Input, false);
	}
}

void motion(int iXPos, int iYPos)
{
	if(g_Input.m_bMouse)
	{
		camInputSetMouseLast(g_Input, iXPos, iYPos);
	}
}

void mouseMotion()
{
	camProcessInput(g_Input, g_Camera);
	glutPostRedisplay();
}

void myInit()
{
	initMaths();
	camInit(g_Camera);
	camInputInit(g_Input);
	camInputExplore(g_Input, true);

	float afGridColour[]={1.0f, 0.1f, 0.3f, 1.0f};
	gridInit(g_ulGrid, afGridColour, -500, 500, 50.0f);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);
}


int main(int argc, char* argv[])
{
	glutInit(&argc, (char**)argv); 

	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(512,512);
	glutCreateWindow("raaAssignment1Solution");

	myInit();

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyboardUp);
	glutSpecialFunc(sKeyboard);
	glutSpecialUpFunc(sKeyboardUp);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);





	glutMainLoop();

	return 0;
}
