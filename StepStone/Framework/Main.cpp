/*
* Copyright (c) 2006-2007 Erin Catto http://www.box2d.org
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/
#include "Main.h"
#include "Render.h"
#include "LuaLevel.h"
#include "Sound.h"
#include <cstdio>
using namespace std;

namespace
{
	LuaLevel* luaLevel = NULL;
	Settings settings;
	int32 width = 640;
	int32 height = 480;
	float settingsHz = 60.0; // target fps?
	int32 framePeriod = (int)(1000/settingsHz);
	int32 mainWindow;
	bool rMouseDown;
	b2Vec2 lastp;
}

static void Resize(int32 w, int32 h)
{
	width = w;
	height = h;
	glViewport(0,0,width,height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float32 ratio = float32(height) / float32(width);

	float32 size = settings.getViewSize();
	b2Vec2 extents;

	if (ratio<1)
		if (settings.isWidthConstant())
			extents.Set(size, size * ratio);
		else
			extents.Set(size / ratio, size);
	else
		extents.Set(size, size * ratio);


	b2Vec2 viewportPosition = settings.getViewPosition();
	extents += viewportPosition;

	gluOrtho2D(viewportPosition.x, extents.x, viewportPosition.y, extents.y);

}

void Settings::setViewPosition(b2Vec2 set)
{
	viewPosition = set;
	Resize(width,height);
}

/*
void Settings::setIsWidthConstant(bool set)
{
	widthIsConstant = set;
	Resize(width,height);
}
*/

void Settings::setViewSize(float32 set)
{
	viewSize = set;
	Resize(width,height);
}

static b2Vec2 ConvertScreenToWorld(int32 x, int32 y)
{
	//~~~~~~~~~~~~~~~~~~ HACK FOR WINDOWS 8???? if the window is not maximized the mouse is 8 pixels more up
	//if (glutGet(GLUT_WINDOW_Y)>19)
	//	y+=8;
	float32 u = x / float32(width);
	float32 v = (height - y) / float32(height);

	float32 ratio = float32(height) / float32(width);

	float32 size = settings.getViewSize();
	b2Vec2 extents;

	if (ratio<1)
		if (settings.isWidthConstant())
			extents.Set(size, size * ratio);
		else
			extents.Set(size / ratio, size);
	else
		extents.Set(size, size*ratio);


	b2Vec2 viewportPosition = settings.getViewPosition();

	b2Vec2 p;
	p.x = viewportPosition.x + u * extents.x;
	p.y = viewportPosition.y + v * extents.y;
	return p;
}	


// This is used to control the frame rate (60Hz).
static void Timer(int)
{
	//glutSetWindow(mainWindow);
	glutPostRedisplay();
	glutTimerFunc(framePeriod, Timer, 0);
}

static void SimulationLoop()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	settings.setHz(settingsHz);
	if (luaLevel!=NULL)
		luaLevel->Step(&settings);

	glutSwapBuffers();
}

static void Keyboard(unsigned char key, int x, int y)
{
	B2_NOT_USED(x);
	B2_NOT_USED(y);

	switch (key)
	{
		// Press 'z' to zoom out.
	case 'z':
		settings.setViewSize(b2Min(1.1f * settings.getViewSize(), 20.0f));
		Resize(width, height);
		break;

		// Press 'x' to zoom in.
	case 'x':
		settings.setViewSize(b2Max(0.9f * settings.getViewSize(), 0.02f));
		Resize(width, height);
		break;

	case 'p':
		settings.setPause(!settings.getPause());
		break;
	case 'l':
		settings.setSingleStep(true);
		break;

	default:
		if (luaLevel)
		{
			luaLevel->Keyboard(key, &settings);
		}
		break;
	}
}

static void KeyboardSpecial(int key, int x, int y)
{
	B2_NOT_USED(x);
	B2_NOT_USED(y);

	b2Vec2 newViewCenter = settings.getViewPosition();
	switch (key)
	{
	case GLUT_ACTIVE_SHIFT:

		// Press left to pan left.
	case GLUT_KEY_LEFT:
		newViewCenter.x -= .5f;
		settings.setViewPosition(newViewCenter);
		break;

		// Press right to pan right.
	case GLUT_KEY_RIGHT:
		newViewCenter.x += .5f;
		settings.setViewPosition(newViewCenter);
		break;

		// Press down to pan down.
	case GLUT_KEY_DOWN:
		newViewCenter.y -= .5f;
		settings.setViewPosition(newViewCenter);
		break;

		// Press up to pan up.
	case GLUT_KEY_UP:
		newViewCenter.y += .5f;
		settings.setViewPosition(newViewCenter);
		break;

		// Press home to reset the view.
	case GLUT_KEY_HOME:
		settings.setViewSize(20.0f);
		settings.setViewPosition(b2Vec2(0.0f, 0.0f));
		break;

	}
}

static void KeyboardUp(unsigned char key, int x, int y)
{
	B2_NOT_USED(x);
	B2_NOT_USED(y);
	luaLevel->KeyboardUp(key);
}

static void Mouse(int32 button, int32 state, int32 x, int32 y)
{
	// Use the mouse to move things around.

	if (button == GLUT_LEFT_BUTTON)
	{
		int mod = glutGetModifiers();
		b2Vec2 p = ConvertScreenToWorld(x, y);
		if (state == GLUT_DOWN)
		{
			b2Vec2 p = ConvertScreenToWorld(x, y);
			if (mod == GLUT_ACTIVE_SHIFT)
			{
				//test->ShiftMouseDown(p);
			}
			else
			{
				luaLevel->MouseDown(p,&settings);
			}
		}

		if (state == GLUT_UP)
		{
			luaLevel->MouseUp(p);
		}
	}
	else if (button == GLUT_RIGHT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{	
			lastp = ConvertScreenToWorld(x, y);
			rMouseDown = true;
		}

		if (state == GLUT_UP)
		{
			rMouseDown = false;
		}
	}

}

static void MouseMotion(int32 x, int32 y)
{
	b2Vec2 p = ConvertScreenToWorld(x, y);
	luaLevel->MouseMove(p);
	/*
	if (rMouseDown)
	{
	b2Vec2 diff = p - lastp;
	b2Vec2 viewCenter = settings.getViewCenter();
	viewCenter.x -= diff.x;
	viewCenter.y -= diff.y;
	settings.setViewCenter(viewCenter);
	lastp = ConvertScreenToWorld(x, y);
	}
	*/
}

static void MouseWheel(int wheel, int direction, int x, int y)
{
	B2_NOT_USED(wheel);
	B2_NOT_USED(x);
	B2_NOT_USED(y);
	if (direction > 0)
	{
		settings.setViewSize(settings.getViewSize()/ 1.1f);
	}
	else
	{
		settings.setViewSize(settings.getViewSize()* 1.1f);
	}
}

#ifdef NDEBUG
//Uncomment the next line if you do not want to see the console.
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif //NDEBUG
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);// | GLUT_MULTISAMPLE);
	glutInitWindowSize(width, height);
	mainWindow = glutCreateWindow("StepStone");

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutDisplayFunc(SimulationLoop);
	glutReshapeFunc(Resize);
	glutKeyboardFunc(Keyboard);
	glutIgnoreKeyRepeat(true);
	glutSpecialFunc(KeyboardSpecial);
	glutMouseFunc(Mouse);
	glutMouseWheelFunc(MouseWheel);
	glutPassiveMotionFunc(MouseMotion);
	glutKeyboardUpFunc(KeyboardUp);


	// Use a timer to control the frame rate.
	glutTimerFunc(framePeriod, Timer, 0);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_ALPHA_TEST);


	luaLevel = new LuaLevel(&settings);

	glutMainLoop();

	delete luaLevel;
	return EXIT_SUCCESS;
}
