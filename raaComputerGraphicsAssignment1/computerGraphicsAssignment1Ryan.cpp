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

#define DRAG 0.999999f
#define GRAVITY 800.0f
#define TIME 0.0166666666666667f

raaCameraInput g_Input;
raaCamera g_Camera;
unsigned long g_ulGrid=0;
int g_aiLastMouse[2];
int g_aiStartMouse[2];
bool g_bExplore=false;
bool g_bFly=false;
int g_currentPlanetId=0;
int g_currentPlanetCount = 0;
enum menuItems {speedNormal, speedFast, speedFaster, tailShort, tailMedium, tailLong, toggleMotion, addTenPlanets, addTwentyPlanets, addFortyPlanets,
				removeTenPlanets, removeTwentyPlanets, removeFortyPlanets, setFiftyPlanets, setHundredPlanets, setHundredFiftyPlanets, saveState};
bool calculateMotion;
int tailMaxLength = 1000;
float timeMultiplier = 1.0f;
int currentTime = 0;
int frameCount = 0;
int previousTime = 0;
float fps = 0.0f;
int ticksPerSecond = 60;
int skipTicks = 1000/ticksPerSecond;
int maxFrameSkip = 10;
int loops = 0;
int nextGameTick = glutGet(GLUT_ELAPSED_TIME);
char lastEvent[100] = "";
struct planetLinePoint
{
	planetLinePoint *m_plpNext;
	planetLinePoint *m_plpPrev;
	float m_fPoint[4];
};
struct planet
{
	planet *m_pNext;
	planet *m_pPrev;
	float m_fSize;
	float m_afStartVel[4];
	float m_afEndVel[4];
	float m_fMass;
	float m_afCol[4]; //x,y,z,1
	float m_afStartPos[4]; //x,y,z,0
	float m_afEndPos[4]; //x,y,z,0
	float m_afForce[4]; //x,y,z,0
	//float m_afForce;
	float m_afAcceleration[4]; //x,y,z,0
	int m_iPlanetId;
	planetLinePoint *m_plpHead;
	planetLinePoint *m_plpTail;
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
void calculatePosition();
void calculateVelocity();
void calculateAcceleration();
void calculateForces(); 
void applyAccelToVel();
void calculateFPS();
void drawText(int x, int y, char* format, ...);
void calculateCollision(planet* currentPlanet, planet* currentOtherPlanet);
void addPlanet(int amount);
void removePlanet(int amount);
void saveModelState();

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

planetLinePoint* createNewPlanetLinePoint(float x, float y, float z)
{
	planetLinePoint *plpPoint = new planetLinePoint;

	plpPoint -> m_plpPrev = 0;
	plpPoint -> m_plpNext = 0;
	plpPoint -> m_fPoint[0] = x;
	plpPoint -> m_fPoint[1] = y;
	plpPoint -> m_fPoint[2] = z;
	plpPoint -> m_fPoint[3] = 0;

	return plpPoint;
}

planet* createNewPlanet()
{
	planet *pPlanet=new planet;

	pPlanet -> m_pPrev = 0;
	pPlanet -> m_pNext = 0;
	pPlanet -> m_plpHead = 0;
	pPlanet -> m_plpTail = 0;

	float vUp[4], vDir[4], vStart[4];

	vecInitPVec(vStart);
	vecInitDVec(vUp);
	vecInitDVec(vDir);

	vecSet(0.0f, 1.0f, 0.0f, vUp);

	// initialise data
	//pPlanet->m_fSize = randFloat(50.0f, 100.0f);
	vecInit(pPlanet->m_afEndVel);
	pPlanet->m_fMass = randFloat(140.0f,150.0f);
	pPlanet->m_fSize = pPlanet->m_fMass;
	vecInitPVec(pPlanet->m_afCol);
	vecSet(randFloat(0.0f, 1.0f),randFloat(0.0f, 1.0f),randFloat(0.0f, 1.0f),pPlanet->m_afCol);
	vecInit(pPlanet->m_afStartPos);
	vecSet(randFloat(-40.0f * (g_pHead->m_fSize / 2), 40.0f * (g_pHead->m_fSize / 2)), randFloat(-40.0f * (g_pHead->m_fSize / 2), 40.0f * (g_pHead->m_fSize / 2)), randFloat(-40.0f * (g_pHead->m_fSize / 2), 40.0f * (g_pHead->m_fSize / 2)), pPlanet->m_afStartPos);
	vecInit(pPlanet->m_afEndPos);
	vecCopy(pPlanet->m_afStartPos, pPlanet->m_afEndPos);

	vecSub(vStart, pPlanet->m_afStartPos, vDir);
	vecNormalise(vDir, vDir);
	//vecCrossProduct(vDir, vUp, pPlanet->m_afStartVel);
	vecNormalise(pPlanet->m_afStartVel, pPlanet->m_afStartVel);
	vecScalarProduct(pPlanet->m_afStartVel, randFloat(-700.0f, 800.0f), pPlanet->m_afStartVel);



	//pPlanet->m_afForce = randFloat(-100.0f, 100.0f);
	vecInit(pPlanet->m_afForce);
	//vecSet(randFloat(-50.0f, 50.0f), randFloat(-50.0f, 50.0f), randFloat(-50.0f, 50.0f), pPlanet->m_afForce);
	//vecSet(50000.0f, 50000.0f, 50000.0f, pPlanet->m_afForce);
	vecInit(pPlanet->m_afAcceleration);

	pPlanet->m_iPlanetId = g_currentPlanetId;

	g_currentPlanetId++;
	g_currentPlanetCount++;

	return pPlanet;
}

planet* createNewStar()
{
	planet *pStar=new planet;

	pStar -> m_pPrev = 0;
	pStar -> m_pNext = 0;

	// initialise data
	//pStar->m_fSize = 500.0f;
	vecInit(pStar->m_afStartVel);
	vecInit(pStar->m_afEndVel);
	pStar->m_fMass = 90000.0f;
	pStar->m_fSize = pStar->m_fMass * 0.01;
	vecInit(pStar->m_afCol);
	vecSet(1.0f, 1.0f, 0.0f, pStar->m_afCol);
	pStar->m_afCol[3] = 1.0f;
	vecInit(pStar->m_afStartPos); // set to world centre
	vecInit(pStar->m_afEndPos);
	//pStar->m_afForce = 0.0f;
	vecInit(pStar->m_afForce);
	vecInit(pStar->m_afAcceleration);
	pStar->m_iPlanetId = g_currentPlanetId;

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
		planetLinePoint *currentPoint = pPlanet ->m_plpHead;
		// release all planet trail memory
		while(currentPoint)
		{
			planetLinePoint *nextPoint = currentPoint ->m_plpNext;
			delete currentPoint;
			currentPoint = nextPoint;
		}
		pPlanet->m_fSize = 0;
		vecInitDVec(pPlanet->m_afStartVel);
		pPlanet->m_fMass = 0;
		vecInitDVec(pPlanet->m_afCol);
		vecInitDVec(pPlanet->m_afStartPos);
		//pPlanet->m_afForce = 0;
		vecInitDVec(pPlanet->m_afForce);

		// free up memory
		delete pE;
		// clean up pointer
		pE=0;
		g_currentPlanetCount--;
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

void pushPlanetLineTail(planet *pPlanet, planetLinePoint *plpPoint)
{
	if(plpPoint != 0 && plpPoint->m_plpNext == 0 && plpPoint->m_plpPrev == 0)
	{
		if(pPlanet -> m_plpHead == 0 && pPlanet -> m_plpTail == 0)
		{
			pPlanet -> m_plpHead = plpPoint;
			pPlanet -> m_plpTail = plpPoint;
		}
		else
		{
			pPlanet -> m_plpTail->m_plpNext = plpPoint;
			plpPoint->m_plpPrev = pPlanet -> m_plpTail;
			plpPoint->m_plpNext = 0;
			pPlanet -> m_plpTail = plpPoint;
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

	glPushMatrix();

	planet *currentPlanet = g_pHead;
	while(currentPlanet)
	{
		planetLinePoint *currentPoint = currentPlanet ->m_plpHead;
		drawSphere(currentPlanet->m_fSize, 20, 20, currentPlanet->m_afStartPos[0], currentPlanet->m_afStartPos[1], currentPlanet->m_afStartPos[2], currentPlanet->m_afCol);

		if(currentPlanet -> m_pPrev != 0) //skip star
		{
			int tailLength = 0;
			glPushAttrib(GL_ALL_ATTRIB_BITS);
			glBegin(GL_LINE_STRIP);
			glEnable(GL_BLEND);
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, currentPlanet->m_afCol);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 5.0f);
			glBlendFunc (GL_ONE, GL_ZERO);
			while(currentPoint)
			{
				glVertex3f(currentPoint->m_fPoint[0], currentPoint->m_fPoint[1], currentPoint->m_fPoint[2]);		
				currentPoint = currentPoint -> m_plpNext;
				tailLength++;
				if(tailLength > tailMaxLength) { // remove head every time over the max length to maintain fixed size tail
					currentPlanet -> m_plpHead = currentPlanet -> m_plpHead -> m_plpNext;
					delete currentPlanet -> m_plpHead -> m_plpPrev;
					currentPlanet -> m_plpHead -> m_plpPrev = 0;
				}
			}
			glDisable(GL_BLEND);
			glEnd();
			glPopAttrib();
		}

		currentPlanet = currentPlanet->m_pNext;
	}

	drawText(5, 10, "FPS: %4.2f", fps);
	drawText(5, 5, "Planets: %d", g_currentPlanetCount);
	//drawText(5, 0, "%s", lastEvent);

	glPopMatrix();
	glFlush(); 

	glutSwapBuffers();
}
void idle()
{    
	loops = 0;
	calculateFPS();
	while(glutGet(GLUT_ELAPSED_TIME) > nextGameTick && loops < maxFrameSkip)
	{
		if(calculateMotion) {
			calculateForces();
			//calculateAcceleration();
			//calculatePosition();
			//calculateVelocity();
			//applyAccelToVel();
		}
		nextGameTick = skipTicks + glutGet(GLUT_ELAPSED_TIME);
		loops++;
	}
	mouseMotion();
	glutPostRedisplay();
}

void calculateFPS()
{
	 //  Increase frame count
    frameCount++;
 
    //  Get the number of milliseconds since glutInit called
    //  (or first call to glutGet(GLUT ELAPSED TIME)).
    currentTime = glutGet(GLUT_ELAPSED_TIME);
 
    //  Calculate time passed
    int timeInterval = currentTime - previousTime;
 
    if(timeInterval > 1000)
    {
        //  calculate the number of frames per second
        fps = frameCount / (timeInterval / 1000.0f);
 
        //  Set time
        previousTime = currentTime;
 
        //  Reset frame count
        frameCount = 0;
    }
}

void drawText(int x, int y, char* format, ...)
{
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, GLUT_WINDOW_WIDTH, 0.0, GLUT_WINDOW_HEIGHT);
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	//  Print the FPS to the window
	va_list args;
	int len;
	int i;
	char * text;
	//char* format = "FPS: %4.2f";

	//  Initialize a variable argument list
	va_start(args, format);

	//  Return the number of characters in the string referenced the list of arguments.
	//  _vscprintf doesn't count terminating '\0' (that's why +1)
	len = _vscprintf(format, args) + 1; 

	//  Allocate memory for a string of the specified size
	text = (char *)malloc(len * sizeof(char));

	//  Write formatted output using a pointer to the list of arguments
	vsprintf_s(text, len, format, args);

	//  End using variable argument list 
	va_end(args);

	//  Specify the raster position for pixel operations.
	glRasterPos2i(x, y);
	//glRasterPos2f ((float) glutGet(GLUT_WINDOW_WIDTH), (float) glutGet(GLUT_WINDOW_HEIGHT));

	//  Draw the characters one by one
    for (i = 0; text[i] != '\0'; i++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, text[i]);

	//  Free the allocated memory for the string
	free(text);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
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
		addPlanet(1);
		break;
	case 'q':
		removePlanet(1);
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

void calculateForces()
{
	float afBigF[4];
	planet *currentPlanet = g_pHead;
	currentPlanet = currentPlanet->m_pNext; //skip the star
	while(currentPlanet)
	{
		vecInitDVec(afBigF);
		//vecCopy(currentPlanet->m_afForce, afBigF);
		float fLittleF;
		planet *currentOtherPlanet = g_pHead;
		planet *collidedPlanet = 0;
		planet *collidedOtherPlanet = 0;
		//currentOtherPlanet = currentOtherPlanet->m_pNext; //skip the star
		while(currentOtherPlanet)
		{
			float afVecResult[4];
			float fVDist = 0.0f;
			float afVDir[4], vUp[4];
			vecInit(afVecResult);
			vecInit(afVDir);
			vecSet(0.0f, 1.0f, 0.0f, vUp);
			if(currentPlanet != currentOtherPlanet) {
				//vecSub(currentPlanet->m_afStartPos, currentOtherPlanet->m_afStartPos, afVDir); // distance between the 2 bodies
				vecSub(currentOtherPlanet->m_afStartPos, currentPlanet->m_afStartPos, afVDir); // distance between the 2 bodies
				fVDist = vecNormalise(afVDir, afVDir); // don't need vec  afVecResult

				//vecCrossProduct(afVDir, vUp, currentPlanet->m_afStartVel);//
				//vecNormalise(currentPlanet->m_afStartVel, currentPlanet->m_afStartVel);//

				fLittleF = GRAVITY * ((currentPlanet->m_fMass * currentOtherPlanet->m_fMass) / (fVDist * fVDist));
				//vecScalarProduct(currentPlanet->m_afStartVel, fLittleF, currentPlanet->m_afStartVel);//
				vecScalarProduct(afVDir, fLittleF, afVDir); //maybe some cros product stuff here to calculate vector direction?
				vecAdd(afBigF, afVDir, afBigF);
				//vecSet(afResult, afResult, afResult, afLittleF); // problem applying forces somewhere here
				//vecAdd(afBigF, afLittleF, afBigF);
				if(fVDist < currentOtherPlanet->m_fSize) {
					collidedPlanet = currentPlanet;
					collidedOtherPlanet = currentOtherPlanet;
				}
			}

			currentOtherPlanet = currentOtherPlanet->m_pNext;
		}

		vecCopy(afBigF, currentPlanet->m_afForce);
		//vecAdd(currentPlanet->m_afForce, afBigF, currentPlanet->m_afForce);

		currentPlanet->m_afAcceleration[0] = currentPlanet->m_afForce[0] / currentPlanet->m_fMass;
		currentPlanet->m_afAcceleration[1] = currentPlanet->m_afForce[1] / currentPlanet->m_fMass;
		currentPlanet->m_afAcceleration[2] = currentPlanet->m_afForce[2] / currentPlanet->m_fMass;


		float afResult[4];
		float afVelResult[4];
		vecInit(afResult);
		vecInit(afVelResult);
		vecScalarProduct(currentPlanet->m_afStartVel, TIME, afVelResult); // ut
		vecScalarProduct(currentPlanet->m_afAcceleration, TIME*TIME, afResult); // at
//		vecVectorProduct(afResult, afResult, afResult); // at squared
		vecScalarProduct(afResult, 0.5f, afResult); // 0.5 * at squared
		vecAdd(afVelResult, afResult, afResult); // ut + at
		vecAdd(currentPlanet->m_afStartPos, afResult, currentPlanet->m_afEndPos); //s = p + the rest
		//memcpy( currentPlanet->m_afEndPos, currentPlanet->m_afStartPos, sizeof( float ) * 4 );//currentPlanet->m_afEndVele //+ (vecAdd(currentPlanet->m_afEndVel, (0.5f * (0 * (TIME * TIME)))));


//		float afResult[4];
		vecSub(currentPlanet->m_afEndPos, currentPlanet->m_afStartPos, afResult); // s-p
		afResult[0] = afResult[0] / TIME;
		afResult[1] = afResult[1] / TIME;
		afResult[2] = afResult[2] / TIME;
		vecCopy(afResult, currentPlanet->m_afEndVel);

		vecCopy(currentPlanet->m_afEndPos, currentPlanet->m_afStartPos); // after all calculations have finished set old pos to new pos

		currentPlanet->m_afStartVel[0] += currentPlanet->m_afAcceleration[0];
        currentPlanet->m_afStartVel[1] += currentPlanet->m_afAcceleration[1];
        currentPlanet->m_afStartVel[2] += currentPlanet->m_afAcceleration[2];

		currentPlanet->m_afStartVel[0] = currentPlanet->m_afStartVel[0] * 0.99999f;
        currentPlanet->m_afStartVel[1] = currentPlanet->m_afStartVel[1] * 0.99999f;
        currentPlanet->m_afStartVel[2] = currentPlanet->m_afStartVel[2] * 0.99999f;

		pushPlanetLineTail(currentPlanet, createNewPlanetLinePoint(currentPlanet->m_afStartPos[0], currentPlanet->m_afStartPos[1], currentPlanet->m_afStartPos[2]));
		//resolve any collisions
		currentPlanet = currentPlanet->m_pNext;
		if(collidedPlanet != 0 && collidedOtherPlanet != 0) {
			calculateCollision(collidedPlanet, collidedOtherPlanet);
		}
		//if planet has gone too far from the star or lost all acceleration then remove from the simulation
		//float fDist = 0.0f;
		//float afDir[4];
		//vecInit(afDir);
		//vecSub(g_pHead->m_afStartPos, currentPlanet->m_afStartPos, afDir);
		//fDist = vecNormalise(afDir, afDir);
		//if(currentPlanet != 0 && fDist > 500000 || vecLength(currentPlanet->m_afAcceleration) == 0.0f)
		//{
		//	remove(currentPlanet);
		//	deletePlanet(currentPlanet);
		//}
	}
}

void calculateCollision(planet* currentPlanet, planet* currentOtherPlanet)
{
	if(currentOtherPlanet ->m_iPlanetId != 0)
	{
		printf("planet %d collided with planet %d!\n", currentPlanet->m_iPlanetId, currentOtherPlanet->m_iPlanetId);
		//lastEvent = "planet " + currentPlanet->m_iPlanetId + " collided with planet " + currentOtherPlanet->m_iPlanetId + "!";
		//drawText(5, 0, "planet %d collided with planet %d!\n", currentPlanet->m_iPlanetId, currentOtherPlanet->m_iPlanetId);
		//planet *pPlanet=new planet;

		//pPlanet -> m_pPrev = 0;
		//pPlanet -> m_pNext = 0;
		//pPlanet -> m_plpHead = 0;
		//pPlanet -> m_plpTail = 0;
		currentPlanet -> m_fMass += currentOtherPlanet->m_fMass;
		//vecInitPVec(pPlanet->m_afCol);
		//(randFloat(0.0f, 1.0f),randFloat(0.0f, 1.0f),randFloat(0.0f, 1.0f),pPlanet->m_afCol);
		currentPlanet -> m_afStartPos[0] = (currentPlanet ->m_afStartPos[0] + currentOtherPlanet ->m_afStartPos[0]) / 2;
		currentPlanet -> m_afStartPos[1] = (currentPlanet ->m_afStartPos[1] + currentOtherPlanet ->m_afStartPos[1]) / 2;
		currentPlanet -> m_afStartPos[2] = (currentPlanet ->m_afStartPos[2] + currentOtherPlanet ->m_afStartPos[2]) / 2;
		currentPlanet -> m_afStartPos[3] = 0;
		//vecInit(pPlanet->m_afEndPos);
		currentPlanet -> m_afForce[0] = (currentPlanet ->m_afForce[0] + currentOtherPlanet ->m_afForce[0]) / 2;
		currentPlanet -> m_afForce[1] = (currentPlanet ->m_afForce[1] + currentOtherPlanet ->m_afForce[1]) / 2;
		currentPlanet -> m_afForce[2] = (currentPlanet ->m_afForce[2] + currentOtherPlanet ->m_afForce[2]) / 2;
		//currentPlanet -> m_afForce[3] = 0;
		currentPlanet -> m_afAcceleration[0] = (currentPlanet ->m_afAcceleration[0] + currentOtherPlanet ->m_afAcceleration[0]) / 2;
		currentPlanet -> m_afAcceleration[1] = (currentPlanet ->m_afAcceleration[1] + currentOtherPlanet ->m_afAcceleration[1]) / 2;
		currentPlanet -> m_afAcceleration[2] = (currentPlanet ->m_afAcceleration[2] + currentOtherPlanet ->m_afAcceleration[2]) / 2;
		//pPlanet -> m_afAcceleration[3] = 0;
		//pPlanet -> m_iPlanetId = g_currentPlanetId;
		//pPlanet -> m_plpHead = 0;
		//pPlanet -> m_plpTail = 0;

		//g_currentPlanetId++;
		//add new merged planet to the list of planets
		//pushTail(pPlanet);
		//remove the two colliding planets
		//remove(currentPlanet);
		//deletePlanet(currentPlanet);
		remove(currentOtherPlanet);
		deletePlanet(currentOtherPlanet);
	}
	else
	{
		printf("planet %d collided with the sun and was burnt to a crisp! The planet is no more.\n", currentPlanet->m_iPlanetId);
		//lastEvent = "planet " + currentPlanet->m_iPlanetId + " collided with planet " + currentOtherPlanet->m_iPlanetId + "!";
		//drawText(5, 0, "planet %d collided with planet %d!\n", currentPlanet->m_iPlanetId, currentOtherPlanet->m_iPlanetId);
		currentOtherPlanet -> m_fMass += currentPlanet -> m_fMass;
		remove(currentPlanet);
		deletePlanet(currentPlanet);
	}
}

void calculatePosition()
{
	//s=p+(ut+1/2(at)squared)
	
	planet *currentPlanet = g_pHead;
	while(currentPlanet)
	{
		float afResult[4];
		float afVelResult[4];
		vecInit(afResult);
		vecInit(afVelResult);
		vecScalarProduct(currentPlanet->m_afStartVel, TIME, afVelResult); // ut
		vecScalarProduct(currentPlanet->m_afAcceleration, TIME*TIME, afResult); // at
//		vecVectorProduct(afResult, afResult, afResult); // at squared
		vecScalarProduct(afResult, 0.5f, afResult); // 0.5 * at squared
		vecAdd(afVelResult, afResult, afResult); // ut + at
		vecAdd(currentPlanet->m_afStartPos, afResult, currentPlanet->m_afEndPos); //s = p + the rest
		//memcpy( currentPlanet->m_afEndPos, currentPlanet->m_afStartPos, sizeof( float ) * 4 );//currentPlanet->m_afEndVele //+ (vecAdd(currentPlanet->m_afEndVel, (0.5f * (0 * (TIME * TIME)))));

		currentPlanet = currentPlanet->m_pNext;
	}
}

void calculateVelocity()
{
	planet *currentPlanet = g_pHead;
	while(currentPlanet)
	{
		//v= (s-p)/t
		float afResult[4];
		vecSub(currentPlanet->m_afEndPos, currentPlanet->m_afStartPos, afResult); // s-p
		afResult[0] = afResult[0] / TIME;
		afResult[1] = afResult[1] / TIME;
		afResult[2] = afResult[2] / TIME;
		vecCopy(afResult, currentPlanet->m_afStartVel);

		vecCopy(currentPlanet->m_afEndPos, currentPlanet->m_afStartPos); // after all calculations have finished set old pos to new pos

		currentPlanet = currentPlanet->m_pNext;
	}
}

void calculateAcceleration()
{
	planet *currentPlanet = g_pHead;
	while(currentPlanet)
	{
		//vecNormalise(currentPlanet->m_afForce, &currentPlanet->m_fMass, currentPlanet->m_afAcceleration);
		currentPlanet->m_afAcceleration[0] = currentPlanet->m_afForce[0] / currentPlanet->m_fMass;
		currentPlanet->m_afAcceleration[1] = currentPlanet->m_afForce[1] / currentPlanet->m_fMass;
		currentPlanet->m_afAcceleration[2] = currentPlanet->m_afForce[2] / currentPlanet->m_fMass;

		currentPlanet = currentPlanet->m_pNext;
	}
}

void applyAccelToVel()
{
	planet *currentPlanet = g_pHead;
	while(currentPlanet)
	{
		currentPlanet->m_afStartVel[0] += currentPlanet->m_afAcceleration[0];
		currentPlanet->m_afStartVel[1] += currentPlanet->m_afAcceleration[1];
		currentPlanet->m_afStartVel[2] += currentPlanet->m_afAcceleration[2];

		currentPlanet = currentPlanet->m_pNext;
	}
}

void addPlanet(int amount)
{
	for(int count = 0; count < amount; count++)
	{
		pushTail(createNewPlanet());
	}
}

void removePlanet(int amount)
{
	for(int count = 0; count < amount; count++)
	{
		if(g_pHead != g_pTail) // When tail equals the head only the star is left which can't be removed
			deletePlanet(popTail());
	}
}

void setTotalPlanets(int amount)
{
	if(amount > g_currentPlanetCount)
	{
		addPlanet(amount - g_currentPlanetCount);
	}
	else
	{
		removePlanet(g_currentPlanetCount - amount);
	}
}

void saveModelState()
{
	FILE *file = fopen("output.solar", "wb");
	if (file != NULL) {
		planet *currentPlanet = g_pHead;
		while(currentPlanet)
		{
			fwrite(currentPlanet, sizeof(*currentPlanet), 1, file);
			currentPlanet = currentPlanet -> m_pNext;
		}
		fclose(file);
	}
}

void menu(int op)
{
	switch(op) {
		case toggleMotion: calculateMotion = !calculateMotion; break;
		case tailShort: tailMaxLength = 300; break;
		case tailMedium: tailMaxLength = 1000; break;
		case tailLong: tailMaxLength = 2000; break;
		case speedNormal: skipTicks = 1000 / ticksPerSecond; break;
		case speedFast: skipTicks = 1000 / (ticksPerSecond * 2); break;
		case speedFaster: skipTicks = 1000 / (ticksPerSecond * 4); break;
		case addTenPlanets: addPlanet(10); break;
		case addTwentyPlanets: addPlanet(20); break;
		case addFortyPlanets: addPlanet(40); break;
		case removeTenPlanets: removePlanet(10); break;
		case removeTwentyPlanets: removePlanet(20); break;
		case removeFortyPlanets: removePlanet(40); break;
		case setFiftyPlanets: setTotalPlanets(50); break;
		case setHundredPlanets: setTotalPlanets(100); break;
		case setHundredFiftyPlanets: setTotalPlanets(150); break;
		case saveState: saveModelState(); break;
	}
}

void myInit()
{
	initMaths();
	camInit(g_Camera);
	camInputInit(g_Input);
	camInputExplore(g_Input, true);

	pushHead(createNewStar());

	addPlanet(10);

	calculateMotion = true;

	int setPlanetCountSubMenu = glutCreateMenu(menu);
	glutAddMenuEntry("50", setFiftyPlanets);
	glutAddMenuEntry("100", setHundredPlanets);
	glutAddMenuEntry("150", setHundredFiftyPlanets);
	int removePlanetSubMenu = glutCreateMenu(menu);
	glutAddMenuEntry("-10", removeTenPlanets);
	glutAddMenuEntry("-20", removeTwentyPlanets);
	glutAddMenuEntry("-40", removeFortyPlanets);
	int addPlanetSubMenu = glutCreateMenu(menu);
	glutAddMenuEntry("+10", addTenPlanets);
	glutAddMenuEntry("+20", addTwentyPlanets);
	glutAddMenuEntry("+40", addFortyPlanets);
	int simSpeedSubMenu = glutCreateMenu(menu);
	glutAddMenuEntry("Normal (1x)", speedNormal);
	glutAddMenuEntry("Fast (2x)", speedFast);
	glutAddMenuEntry("Faster (4x)", speedFaster);
	int tailLengthSubMenu = glutCreateMenu(menu);
	glutAddMenuEntry("Short", tailShort);
	glutAddMenuEntry("Medium", tailMedium);
	glutAddMenuEntry("Long", tailLong);
	int mainMenu = glutCreateMenu(menu);
	glutAddMenuEntry("Toggle Motion", toggleMotion);
	glutAddMenuEntry("Save State",saveState);
	glutAddSubMenu("Tail Length", tailLengthSubMenu);
	glutAddSubMenu("Sim. Speed", simSpeedSubMenu);
	glutAddSubMenu("Add Planets", addPlanetSubMenu);
	glutAddSubMenu("Remove Planets", removePlanetSubMenu);
	glutAddSubMenu("Set Planet Count", setPlanetCountSubMenu);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	//float afGridColour[]={1.0f, 0.1f, 0.3f, 1.0f};
	//gridInit(g_ulGrid, afGridColour, -50, 50, 500.0f);

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
