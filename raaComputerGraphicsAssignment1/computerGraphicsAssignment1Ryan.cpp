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
struct planet
{
	planet *m_pNext;
	planet *m_pPrev;
	float m_fSize;
	float m_afVel[4];
	float m_fMass;
	float m_afCol[4]; //x,y,z,1
	float m_afPos[4]; //x,y,z,0
	float m_afForce[4]; //x,y,z,0
	//float m_afForce;
};
planet *g_pHead=0;
planet *g_pTail=0;


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
void updateVelocity();
void applyAcceleration();

planet* createNewPlanet(char data);
planet* deletePlanet(planet *pPlanet);
void pushHead(planet *pPlanet);
void pushTail(planet *pPlanet);
planet* popHead();
planet* popTail();
bool destroy(planet* pPlanet);
void insertBefore(planet* pPlanet, planet* pTarget);
void insertAfter(planet* pPlanet, planet* pTarget);
bool remove(planet* pPlanet);

unsigned int g_uiLastTime=0;

planet* createNewPlanet()
{
	planet *pPlanet=new planet;

	pPlanet -> m_pPrev = 0;
	pPlanet -> m_pNext = 0;

	// initialise data
	pPlanet->m_fSize = randFloat(20.0f, 200.0f);
	vecInit(pPlanet->m_afVel);
	vecSet(randFloat(-10.0f, 10.0f), randFloat(-10.0f, 10.0f), randFloat(-10.0f, 10.0f), pPlanet->m_afVel);
	pPlanet->m_fMass = randFloat(5.0f,10.0f);
	vecInit(pPlanet->m_afCol);
	vecSet(randFloat(0.0f, 1.0f),randFloat(0.0f, 1.0f),randFloat(0.0f, 1.0f),pPlanet->m_afCol);
	vecInit(pPlanet->m_afPos);
	vecSet(randFloat(-5000.0f, 5000.0f), randFloat(-5000.0f, 5000.0f), randFloat(-5000.0f, 5000.0f), pPlanet->m_afPos);
	//pPlanet->m_afForce = randFloat(-100.0f, 100.0f);
	vecInit(pPlanet->m_afForce);
	vecSet(randFloat(-50.0f, 50.0f), randFloat(-50.0f, 50.0f), randFloat(-50.0f, 50.0f), pPlanet->m_afForce);

	return pPlanet;
}

planet* createNewStar()
{
	planet *pStar=new planet;

	pStar -> m_pPrev = 0;
	pStar -> m_pNext = 0;

	// initialise data
	pStar->m_fSize = 600.0f;
	vecInit(pStar->m_afVel);
	pStar->m_fMass = 10000.0f;
	vecInit(pStar->m_afCol);
	vecSet(1.0f, 1.0f, 0.0f, pStar->m_afCol);
	pStar->m_afCol[3] = 1.0f;
	vecInit(pStar->m_afPos);
	//pStar->m_afForce = 0.0f;
	vecInit(pStar->m_afForce);

	return pStar;
}

planet* deletePlanet(planet *pPlanet)
{
	planet *pE=pPlanet;

	if(pE)
	{
		// remove from list
		remove(pE);

		// clean up data
		pPlanet->m_fSize = 0;
		vecInitDVec(pPlanet->m_afVel);
		pPlanet->m_fMass = 0;
		vecInitDVec(pPlanet->m_afCol);
		vecInitDVec(pPlanet->m_afPos);
		//pPlanet->m_afForce = 0;
		vecInitDVec(pPlanet->m_afForce);

		// free up memory
		delete pE;
		// clean up pointer
		pE=0;
	}

	return pE;
}

void pushHead(planet *pPlanet)
{
	if(pPlanet != 0 && pPlanet->m_pNext == 0 && pPlanet->m_pPrev == 0)
	{
		if(g_pHead == 0 && g_pTail == 0)
		{
			g_pHead = pPlanet;
			g_pTail = pPlanet;
		}
		else
		{
			pPlanet->m_pNext = g_pHead;
			pPlanet->m_pPrev = 0;
			g_pHead->m_pPrev = pPlanet;
			g_pHead = pPlanet;
		}
	}
}

void pushTail(planet *pPlanet)
{
	if(pPlanet != 0 && pPlanet->m_pNext == 0 && pPlanet->m_pPrev == 0)
	{
		if(g_pHead == 0 && g_pTail == 0)
		{
			g_pHead = pPlanet;
			g_pTail = pPlanet;
		}
		else
		{
			g_pTail->m_pNext = pPlanet;
			pPlanet->m_pPrev = g_pTail;
			pPlanet->m_pNext = 0;
			g_pTail = pPlanet;
		}
	}
}

planet* popHead()
{
	if(g_pHead == 0 && g_pTail == 0)
	{
		//list is empty
		return 0;
	}

	planet* poppedHead = 0;

	if(g_pHead == g_pTail)
	{
		//only 1 Planet in the list	
		poppedHead = g_pHead;
		g_pHead = 0;
		g_pTail = 0;
		return poppedHead;
	}
	else
	{
		poppedHead = g_pHead;
		g_pHead = poppedHead -> m_pNext;
		g_pHead -> m_pPrev = 0;
		poppedHead -> m_pNext = 0;
		poppedHead -> m_pPrev = 0;
		return poppedHead;
	}
}

planet* popTail()
{
	if(g_pHead == 0 && g_pTail == 0)
	{
		//list is empty
		return 0;
	}

	planet* poppedTail = 0;

	if(g_pHead == g_pTail)
	{
		//only 1 Planet in the list	
		poppedTail = g_pTail;
		g_pHead = 0;
		g_pTail = 0;
		return poppedTail;
	}
	else
	{
		poppedTail = g_pTail;
		g_pTail = poppedTail -> m_pPrev;
		g_pTail -> m_pNext = 0;
		poppedTail -> m_pNext = 0;
		poppedTail -> m_pPrev = 0;
		return poppedTail;
	}
}

bool destroy(planet* pPlanet)
{
	if(pPlanet == 0)
		return false;
	if(pPlanet -> m_pNext == 0 && pPlanet -> m_pPrev == 0 && pPlanet != g_pHead && pPlanet != g_pTail)
	{
		delete pPlanet;
		pPlanet = 0;
		return true;
	}
	else
		return false;
}

void insertBefore(planet* pPlanet, planet* pTarget)
{
	if(pTarget == g_pHead)
	{
		pushHead(pPlanet);
	}
	else
	{
		pTarget -> m_pPrev -> m_pNext = pPlanet;
		pPlanet -> m_pPrev = pTarget -> m_pPrev;
		pTarget -> m_pPrev = pPlanet;
		pPlanet ->m_pNext = pTarget;
	}
}

void insertAfter(planet* pPlanet, planet* pTarget)
{
	if(pTarget == g_pTail)
	{
		pushTail(pPlanet);
	}
	else
	{
		pTarget -> m_pNext -> m_pPrev = pPlanet;
		pPlanet -> m_pNext = pTarget -> m_pNext;
		pTarget -> m_pNext = pPlanet;
		pPlanet -> m_pPrev = pTarget;
	}
}

bool remove(planet* pPlanet)
{
	if(!pPlanet || pPlanet->m_pNext == 0 && pPlanet->m_pPrev == 0)
	{
		//Planet is null or isn't in a list
		return false;
	}
	if(pPlanet == g_pHead)
	{
		popHead();
		return true;
	}
	if(pPlanet == g_pTail)
	{
		popTail();
		return true;
	}
	//if the Planet isn't a head or tail Planet then remove from within the list
	pPlanet->m_pPrev->m_pNext = pPlanet->m_pNext;
	pPlanet->m_pNext->m_pPrev = pPlanet->m_pPrev;
	pPlanet->m_pNext = 0;
	pPlanet->m_pPrev = 0;
	return true;
}

void display()
{
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

	glLoadIdentity();

	camApply(g_Camera);

	//gridDraw(g_ulGrid);

	glPushMatrix();

	// this is a placeholder, you should replace it with instructions to draw your planet system
	planet *currentPlanet = g_pHead;
	while(currentPlanet)
	{
		drawSphere(currentPlanet->m_fSize, 20, 20, currentPlanet->m_afPos[0], currentPlanet->m_afPos[1], currentPlanet->m_afPos[2], currentPlanet->m_afCol);
		currentPlanet = currentPlanet->m_pNext;
	}

	glPopMatrix();
	glFlush(); 

	glutSwapBuffers();
}
void idle()
{
	applyAcceleration();
	updateVelocity();
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
	case 'a':
		pushTail(createNewPlanet());
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

void updateVelocity()
{
	planet *currentPlanet = g_pHead;
	while(currentPlanet)
	{
		currentPlanet->m_afPos[0] += currentPlanet->m_afVel[0];
		currentPlanet->m_afPos[1] += currentPlanet->m_afVel[1];
		currentPlanet->m_afPos[2] += currentPlanet->m_afVel[2];
		currentPlanet = currentPlanet->m_pNext;
	}
}

void applyAcceleration()
{
	planet *currentPlanet = g_pHead;
	while(currentPlanet)
	{
		currentPlanet->m_afVel[0] += currentPlanet->m_afForce[0] / currentPlanet->m_fMass;
		currentPlanet->m_afVel[1] += currentPlanet->m_afForce[1] / currentPlanet->m_fMass;
		currentPlanet->m_afVel[2] += currentPlanet->m_afForce[2] / currentPlanet->m_fMass;
		currentPlanet = currentPlanet->m_pNext;
	}
}

void myInit()
{
	initMaths();
	camInit(g_Camera);
	camInputInit(g_Input);
	camInputExplore(g_Input, true);

	pushHead(createNewStar());

	for(int count = 0; count < 10; count++)
	{
		pushTail(createNewPlanet());
	}

	float afGridColour[]={1.0f, 0.1f, 0.3f, 1.0f};
	gridInit(g_ulGrid, afGridColour, -50, 50, 500.0f);

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
	glutCreateWindow("assignment1SolutionRyan");

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
