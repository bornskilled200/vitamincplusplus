#include "LuaLevel.h"
/*
* Copyright (c) 2006-2009 Erin Catto http://www.box2d.org
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
#include <cstdio>
#include <iostream>
#include <sstream> 
#include <string>
#include "lodepng\lodepng.h"
#include "glui/glui.h"
using namespace std;

void LuaLevelDestructionListener::SayGoodbye(b2Joint* joint)
{
}
const static uint16 debrisBits = 0x0002 ;
const static uint16 boundaryBits =0x0004;
const static uint16 playerFeetBits =0x0008;
const static uint16 PLAYER_FEET_TOUCHING_BOUNDARY=playerFeetBits|boundaryBits;
const static uint16 PLAYER_FEET_TOUCHING_DEBRIS=playerFeetBits|debrisBits;

static  char controlKeyLeft = 'a';
static char controlKeyRight = 'd';
static char controlKeyJump = 'w';

static vector<unsigned char> image;
GLuint loadTexture(string fileName, unsigned int &imageWidth, unsigned int &imageHeight, float32 &scaledImageWidth, float32 &scaledImageHeight)
{
	image.clear();
	unsigned error = lodepng::decode(image, imageWidth, imageHeight, fileName);
	//if there's an error, display it
	if(error) 
	{
		std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
		return 0;
	}

	// Texture size must be power of two for the primitive OpenGL version this is written for. Find next power of two.
	size_t u2 = 1; while(u2 < imageWidth) u2 *= 2;
	size_t v2 = 1; while(v2 < imageHeight) v2 *= 2;
	// Ratio for power of two version compared to actual version, to render the non power of two image with proper size.
	scaledImageWidth = (float32)imageWidth / u2;
	scaledImageHeight= (float32)imageHeight / v2;

	// Make power of two version of the image.
	std::vector<unsigned char> image2(u2 * v2 * 4);
	for(size_t y = 0; y < imageHeight; y++)
		for(size_t x = 0; x < imageWidth; x++)
			for(size_t c = 0; c < 4; c++)
			{
				image2[4 * u2 * y + 4 * x + c] = image[4 * imageWidth * y + 4 * x + c];
			}

			// Enable the texture for OpenGL.
			GLuint id;
			glEnable(GL_TEXTURE_2D);
			glGenTextures(1, &id);
			//printf("\ntexture = %u", id);
			glBindTexture(GL_TEXTURE_2D, id);//evrything we're about to do is about this texture
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //GL_NEAREST = no smoothing
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, 4, u2, v2, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image2[0]);
			return id;
}

inline void loadATexture(string fileName, Texture *texture)
{
	texture->id = loadTexture(fileName,texture->imageWidth,texture->imageHeight,texture->scaledImageWidth,texture->scaledImageHeight);
}

LuaLevel::LuaLevel()
{
	image.reserve(1024*768*4);
	loadATexture("Angela\\test\\1.png", &idleImages[0]);
	loadATexture("Angela\\test\\2.png", &(idleImages[1]));
	loadATexture("Angela\\test\\3.png", &(idleImages[2]));
	loadATexture("helpscreen.png", &helpImage);
	loadATexture("aboutscreen.png", &aboutImage);
	loadATexture("titlescreen.png", &menuImage);
	framecount=0;
	// Init Lua
	luaPState = LuaState::Create();
	luaPState->OpenLibs();

	// Init Box2D World
	b2Vec2 gravity;
	gravity.Set(0.0f, -30.0f);
	m_world = new b2World(gravity);

	//~~LEVEL LOADING~~~~~~~~~~~~~~~~~~~~~~~~
	b2BodyDef bodyDef;
	bodyDef.type = b2_staticBody;
	m_groundBody = m_world->CreateBody(&bodyDef);

	loadLevelGlobals(luaPState);

	// Open the Lua Script File
	if (luaPState->DoFile("TrainingLevel.lua"))
		if( luaPState->GetTop() == 1 )
			std::cout << "An error occured: " << luaPState->CheckString(1) << std::endl;

	unloadLevelGlobals(luaPState);

	//~~~~~~PLAYER STUFF
	//~~~~~~~~~~~~~~~~~Box2D Stuff
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set(2,2);
	bodyDef.fixedRotation=true;
	playerBody = m_world->CreateBody(&bodyDef);

	b2PolygonShape polygonShape;
	polygonShape.SetAsBox(.76f,1.28f);
	playerBody->CreateFixture(&polygonShape,1.0);

	b2Vec2 center(0,-1.28f);
	polygonShape.SetAsBox(.76f,.2f, center, 0);
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &polygonShape;
	fixtureDef.filter.categoryBits=playerFeetBits;
	playerFeet = playerBody->CreateFixture(&fixtureDef);
	//~~~~~~~~~~~~~~~~~User Interface
	controlJump = false;
	controlLeft= false;
	controlRight=false;
	gameState = MENU;


	uint32 flags = 0;
	flags += b2Draw::e_shapeBit;
	flags += b2Draw::e_jointBit;
	m_debugDraw.SetFlags(flags);

	m_destructionListener.luaLevel = this;
	m_world->SetDestructionListener(&m_destructionListener);
	m_world->SetDebugDraw(&m_debugDraw);

	m_stepCount = 0;
}

int LuaLevel::createAnEdge( float32 x1, float32 y1, float32 x2, float32 y2 )
{
	//cout<<x1<<", "<<y1<<", "<<x2<<", "<<y2<<endl;
	b2FixtureDef fixtureDef;
	fixtureDef.filter.categoryBits=boundaryBits;
	b2EdgeShape edgeShape;
	b2Vec2 from(x1,y1),to(x2,y2);
	edgeShape.Set(from,to);
	fixtureDef.shape = &edgeShape;
	m_groundBody->CreateFixture(&fixtureDef);
	return 0;
}

void LuaLevel::loadLevelGlobals(LuaState *pstate)
{
	//Box2DFactory~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//pstate->GetGlobals().RegisterObjectDirect( "createEdge",createEdge);
	LuaObject globals = pstate->GetGlobals();
	globals.RegisterDirect( "createEdge",*this, &LuaLevel::createAnEdge);
	pstate->GetGlobal("controlKeyLeft").AssignString(pstate,&controlKeyLeft,1);
	pstate->GetGlobal("controlKeyRight").AssignString(pstate,&controlKeyRight,1);
	pstate->GetGlobal("controlKeyJump").AssignString(pstate,&controlKeyJump,1);
}

inline void checkAndSetChar(char &aChar, LuaObject luaObject)
{
	if (luaObject.IsString())
		aChar = luaObject.GetString()[0];
}

void LuaLevel::unloadLevelGlobals(LuaState *pstate)
{
	pstate->GetGlobals().Unregister("createEdge");
	checkAndSetChar(controlKeyLeft, pstate->GetGlobal("controlKeyLeft"));
	checkAndSetChar(controlKeyRight, pstate->GetGlobal("controlKeyRight"));
	checkAndSetChar(controlKeyJump, pstate->GetGlobal("controlKeyJump"));
}

LuaLevel::~LuaLevel()
{
	// By deleting the world, we delete the bomb, mouse joint, etc.
	LuaState::Destroy(luaPState);
	delete m_world;
	m_world = NULL;
}

void LuaLevel::DrawTitle(int x, int y, const char *string)
{
	m_debugDraw.DrawString(x, y, string);
}

void LuaLevel::drawGame(Settings* settings)
{
	float32 timeStep = settings->hz > 0.0f ? 1.0f / settings->hz : float32(0.0f);

	if (settings->pause)
	{
		if (settings->singleStep)
		{
			settings->singleStep = 0;
		}
		else
		{
			timeStep = 0.0f;
		}

		//m_debugDraw.DrawString(5, 512, "****PAUSED****");
	}

	m_world->SetWarmStarting(settings->enableWarmStarting > 0);
	m_world->SetContinuousPhysics(settings->enableContinuous > 0);
	m_world->SetSubStepping(settings->enableSubStepping > 0);

	m_world->Step(timeStep, settings->velocityIterations, settings->positionIterations);

	m_world->DrawDebugData();

	if (timeStep > 0.0f)
	{
		++m_stepCount;
	}
}

//This draws the texture flipped, so perfect for a directly loaded png!
void drawImage(GLubyte id, unsigned int width, unsigned int height, GLfloat scaledWidth, GLfloat scaledHeight)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, id);
	glBegin(GL_QUADS);
	glTexCoord2f(		   0, scaledHeight); glVertex2i(	   0,	   0);
	glTexCoord2f(scaledWidth, scaledHeight); glVertex2i(width,	   0);
	glTexCoord2f(scaledWidth,			 0); glVertex2i(width, height);
	glTexCoord2f(		   0,			 0); glVertex2i(	   0, height);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

inline void drawImage(Texture *texture)
{
	drawImage(texture->id,texture->imageWidth,texture->imageHeight,texture->scaledImageWidth,texture->scaledImageHeight);
}

void LuaLevel::processCollisionsForGame()
{
	isFeetTouchingBoundary = false;
	b2WorldManifold worldManifold;
	for (b2Contact *c = m_world->GetContactList() ; c ; c = c->GetNext())
	{
		if (!c->IsTouching())
			continue;


		int collision = c->GetFixtureA()->GetFilterData().categoryBits | c->GetFixtureB()->GetFilterData().categoryBits;

		if (PLAYER_FEET_TOUCHING_BOUNDARY == collision || PLAYER_FEET_TOUCHING_DEBRIS == collision) {
			isFeetTouchingBoundary = true;
			c->GetWorldManifold(&worldManifold);
			if (worldManifold.normal.y != 0) {
				canJump = true;
				justKickedOff = false;
			}
		}
	}
	if (isFeetTouchingBoundary == false) {
		justKickedOff = false;
	}
}

void LuaLevel::processInputForGame(Settings *settings)
{
	b2Vec2 worldCenter = playerBody->GetWorldCenter();
	b2Vec2 linearVelocity = playerBody->GetLinearVelocity();

	float32 timeStep = settings->hz > 0.0f ? 1.0f / settings->hz : float32(0.0f);
	// JUMPING
	if (playerCanMoveUpwards>0) playerCanMoveUpwards -= timeStep;
	b2Vec2 jumpImpulse;
	if (controlJump) {
		if (isFeetTouchingBoundary) {
			if (canJump) {
				jumpImpulse.Set(0,12);
				playerBody->ApplyLinearImpulse(jumpImpulse, worldCenter);
				playerCanMoveUpwards = .15f;
				canJump = false;
			} else {
				if (!justKickedOff && linearVelocity.y > 0) {
					jumpImpulse.Set(0,40);
					playerBody->ApplyLinearImpulse(jumpImpulse, worldCenter);
					justKickedOff = true;
				}
			}
		} else {
			if (playerCanMoveUpwards > 0)
			{
				jumpImpulse.Set(0,2.2f);
				playerBody->ApplyLinearImpulse(jumpImpulse, worldCenter);
			}
		}
	} else playerCanMoveUpwards = 0;

	// HORIZONTAL MOVEMENT
	float32 vx = 0;
	if (controlLeft)
		vx += -200;
	if (controlRight)
		vx += 200;

	if (vx == 0) {
		playerFeet->SetFriction(5);
		if (wasMoving) {
			for (b2Contact *c = m_world->GetContactList() ; c ; c = c->GetNext())
				c->ResetFriction();
			wasMoving = false;
		}
	} else {
		b2Vec2 force(vx, 0);
		if (vx > 0 && linearVelocity.x < 8) {
			playerBody->ApplyForce(force, worldCenter);
		} else if (vx < 0 && linearVelocity.x > -8) {
			playerBody->ApplyForce(force, worldCenter);
		}
		playerFeet->SetFriction(0);
		if (!wasMoving) {
			for (b2Contact *c = m_world->GetContactList() ; c ; c = c->GetNext())
				c->ResetFriction();
			wasMoving = true;
		}
	}
}
void LuaLevel::Step(Settings* settings, float32 &viewZoom)
{
	switch (gameState)
	{
	case MENU:
		settings->viewCenter.Set((float32)menuImage.imageWidth/2,(float32)menuImage.imageHeight/2);
		viewZoom=15;
		glColor4ub(255, 255, 255, 255);
		drawImage(&menuImage);
		break;
	case MENU_ABOUT:
		settings->viewCenter.Set((float32)aboutImage.imageWidth/2,(float32)aboutImage.imageHeight/2);
		viewZoom=15;
		glColor4ub(255, 255, 255, 255);
		drawImage(&aboutImage);
		break;
	case MENU_HELP:
		settings->viewCenter.Set((float32)aboutImage.imageWidth/2,(float32)helpImage.imageHeight/2);
		viewZoom=15;
		glColor4ub(255, 255, 255, 255);
		drawImage(&helpImage);
		break;
	case GAME:
		viewZoom=1;
		settings->viewCenter.Set(0,20);

		processCollisionsForGame();
		processInputForGame(settings);

		b2Vec2 worldCenter = playerBody->GetWorldCenter();
		glTranslatef(worldCenter.x-.76f,worldCenter.y-1.28f,0);
		float32 scale = .015f;
		glScalef(scale,scale,scale);
		glColor4f(1,1,1,1);
		drawImage(&idleImages[framecount/26]);
		framecount=(framecount+1)%(26*3);
		glScalef(1/scale,1/scale,1/scale);
		glTranslatef(-(worldCenter.x-.76f),-(worldCenter.y-1.28f),0);
		drawGame(settings);
		break;
	}

	m_debugDraw.DrawString(0, 15, "key 1: Game");
	m_debugDraw.DrawString(0, 30, "key 2: Menu");
	m_debugDraw.DrawString(0, 45, "key 3: About");
	m_debugDraw.DrawString(0, 60, "key 4: Help");
	m_debugDraw.DrawString(0, 75, "isplayerfeettouchingground %s", isFeetTouchingBoundary?"true":"false");
	m_debugDraw.DrawString(0, 90, "canJump %s", canJump?"true":"false");
	m_debugDraw.DrawString(0, 105, "playerCanMoveUpwards %f",playerCanMoveUpwards);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~	INPUT HANDLING	~```~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void LuaLevel::Keyboard(unsigned char key)
{
	switch (key)
	{
	case 'a':
		controlLeft = true;
		break;
	case 'd':
		controlRight = true;
		break;
	case 'w':
		controlJump= true;
		break;
	case '1':
		gameState=GAME;
		break;
	case 27: //ESCAPE KEY
		if (gameState==MENU)
			exit(0);
	case '2':
		gameState=MENU;
		break;
	case '3':
		gameState=MENU_ABOUT;
		break;
	case '4':
		gameState=MENU_HELP;
		break;
	}
	//cout<<gameState<<endl;
}

void LuaLevel::KeyboardUp(unsigned char key)
{
	switch (key)
	{
	case 'a':
		controlLeft = false;
		break;
	case 'd':
		controlRight = false;
		break;
	case 'w':
		controlJump= false;
		break;
	}
}


void LuaLevel::MouseDown(const b2Vec2& p)
{
	//m_mouseWorld = p;
}

void LuaLevel::ShiftMouseDown(const b2Vec2& p)
{
	//m_mouseWorld = p;
}

void LuaLevel::MouseUp(const b2Vec2& p)
{
	//m_mouseWorld = p;
}

void LuaLevel::MouseMove(const b2Vec2& p)
{
	//m_mouseWorld = p;
}