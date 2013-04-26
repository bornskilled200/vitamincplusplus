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
#include "glui/glui.h"
#include "mpg.h"
#include <cstdio>
using namespace std;

namespace
{
	LuaLevel* luaLevel;
	Settings settings;
	int32 width = 640;
	int32 height = 480;
	int32 framePeriod = 16;
	int32 mainWindow;
	float settingsHz = 60.0; // target fps?
	int tx, ty, tw, th; // 
	bool rMouseDown;
	b2Vec2 lastp;
}

static void Resize(int32 w, int32 h)
{
	width = w;
	height = h;

	GLUI_Master.get_viewport_area(&tx, &ty, &tw, &th);
	glViewport(tx, ty, tw, th);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float32 ratio = float32(tw) / float32(th);

	b2Vec2 extents(ratio * 25.0f, 25.0f);
	extents *= settings.getViewZoom();

	b2Vec2 lower = settings.getViewCenter() - extents;
	b2Vec2 upper = settings.getViewCenter() + extents;

	// L/R/B/T
	gluOrtho2D(lower.x, upper.x, lower.y, upper.y);
}

void Settings::setViewCenter(b2Vec2 set)
{
	viewCenter = set;
	Resize(width,height);
}

void Settings::setViewZoom(float32 set)
{
	viewZoom = set;
	Resize(width,height);
}

static b2Vec2 ConvertScreenToWorld(int32 x, int32 y)
{
	float32 u = x / float32(tw);
	float32 v = (th - y) / float32(th);

	float32 ratio = float32(tw) / float32(th);
	b2Vec2 extents(ratio * 25.0f, 25.0f);
	extents *= settings.getViewZoom();

	b2Vec2 lower = settings.getViewCenter() - extents;
	b2Vec2 upper = settings.getViewCenter() + extents;

	b2Vec2 p;
	p.x = (1.0f - u) * lower.x + u * upper.x;
	p.y = (1.0f - v) * lower.y + v * upper.y;
	return p;
}

// This is used to control the frame rate (60Hz).
static void Timer(int)
{
	glutSetWindow(mainWindow);
	glutPostRedisplay();
	glutTimerFunc(framePeriod, Timer, 0);
}

static void SimulationLoop()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	settings.setHz(settingsHz);
	luaLevel->Step(&settings);

	//test->DrawTitle(5, 15, entry->name);

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
		settings.setViewZoom(b2Min(1.1f * settings.getViewZoom(), 20.0f));
		Resize(width, height);
		break;

		// Press 'x' to zoom in.
	case 'x':
		settings.setViewZoom(b2Max(0.9f * settings.getViewZoom(), 0.02f));
		Resize(width, height);
		break;

		// Press 'r' to reset.
	case 'r':
		//delete test;
		//test = entry->createFcn();
		break;

	case 'p':
		settings.setPause(!settings.getPause());
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

	b2Vec2 newViewCenter(settings.getViewCenter());
	switch (key)
	{
	case GLUT_ACTIVE_SHIFT:

		// Press left to pan left.
	case GLUT_KEY_LEFT:
		newViewCenter.x -= .5f;
		settings.setViewCenter(newViewCenter);
		break;

		// Press right to pan right.
	case GLUT_KEY_RIGHT:
		newViewCenter.x += .5f;
		settings.setViewCenter(newViewCenter);
		break;

		// Press down to pan down.
	case GLUT_KEY_DOWN:
		newViewCenter.y -= .5f;
		settings.setViewCenter(newViewCenter);
		break;

		// Press up to pan up.
	case GLUT_KEY_UP:
		newViewCenter.y += .5f;
		settings.setViewCenter(newViewCenter);
		break;

		// Press home to reset the view.
	case GLUT_KEY_HOME:
		settings.setViewZoom(1.0f);
		settings.setViewCenter(b2Vec2(0.0f, 20.0f));
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
				//test->MouseDown(p);
			}
		}

		if (state == GLUT_UP)
		{
			//test->MouseUp(p);
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
	//test->MouseMove(p);

	if (rMouseDown)
	{
		b2Vec2 diff = p - lastp;
		b2Vec2 viewCenter = settings.getViewCenter();
		viewCenter.x -= diff.x;
		viewCenter.y -= diff.y;
		settings.setViewCenter(viewCenter);
		lastp = ConvertScreenToWorld(x, y);
	}
}

static void MouseWheel(int wheel, int direction, int x, int y)
{
	B2_NOT_USED(wheel);
	B2_NOT_USED(x);
	B2_NOT_USED(y);
	if (direction > 0)
	{
		settings.setViewZoom(settings.getViewZoom()/ 1.1f);
	}
	else
	{
		settings.setViewZoom(settings.getViewZoom()* 1.1f);
	}
}

static void Restart(int)
{
	//delete test;
	//entry = g_testEntries + testIndex;
	//test = entry->createFcn();
	Resize(width, height);
}

static void Pause(int)
{
	settings.setPause(!settings.getPause());
}

static void Exit(int code)
{
	glutLeaveMainLoop();
}

static void SingleStep(int)
{
	settings.setPause(1);
	settings.setSingleStep(1);
}


int audioCallback(const void *input, void *output, 
				  unsigned long frameCount,
				  const PaStreamCallbackTimeInfo* timeInfo,
				  PaStreamCallbackFlags statusFlags,
				  void* userData)
{
	
	mpg123Struct* mpgStruct = static_cast<mpg123Struct*>(userData);
	memset(output, 0, frameCount * 2 * sizeof(short));
	// I HAVE NO IDEA WHY THIS WORKKKKKSSSSSSSSS
	if (mpg123_read(mpgStruct->mh, (unsigned char*)output, frameCount*4, &mpgStruct->done) == MPG123_OK) {
		//memcpy((unsigned char*)output, mpgStruct->buffer, mpgStruct->done/4);
		//portaudio->rms = rms(outputBuffer, size);
		//portaudio->position = mpg123_tellframe(portaudio->mpg123);
		/*
		short *out = (short*)output;
		int i,j,playbackIndex = 0;

		for( i=0; i<frameCount; i++ )
			{
				for( j = 0; j < 2; ++j ){
					*out++ = ((short*)mpgStruct->buffer)[ playbackIndex++ ];

				}
			}
			*/
		//totalBtyes += mpgStruct->done;
		//std::cout<<"huh"<<buffer<<" "<<done<<", "<<done/4<<std::endl;
	}
	// Play it safe when debugging and coding, protect your ears by clearing
	// the output buffer.

	// Decode the number of samples that PortAudio said it needs to send to the 
	// soundcard. This is where we're grabbing audio from demo.mp3!

	else{
		/* clean up */
	
		free(mpgStruct->buffer);
		mpg123_close(mpgStruct->mh);
		mpg123_delete(mpgStruct->mh);
		mpg123_exit();
		return paComplete;
	}
	return paContinue;
	 /* Cast data passed through stream to our structure. */
    //paTestData *data = (paTestData*)userData; 
}
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glutInitWindowSize(width, height);
	mainWindow = glutCreateWindow("StepStone");

	glutDisplayFunc(SimulationLoop);
	GLUI_Master.set_glutReshapeFunc(Resize);  
	GLUI_Master.set_glutKeyboardFunc(Keyboard);
	GLUI_Master.set_glutSpecialFunc(KeyboardSpecial);
	GLUI_Master.set_glutMouseFunc(Mouse);
	glutMouseWheelFunc(MouseWheel);
	glutMotionFunc(MouseMotion);
	glutKeyboardUpFunc(KeyboardUp);

	// Use a timer to control the frame rate.
	glutTimerFunc(framePeriod, Timer, 0);
	LuaLevel aLuaLevel(&settings); // can we safely initlize the world in the stack rather in the heap?
	luaLevel = &aLuaLevel;
	loadMp3File("title\\music.mp3", &audioCallback);
	glutMainLoop();

	return 0;
}
