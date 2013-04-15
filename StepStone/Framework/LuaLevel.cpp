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

int Print( LuaPlus::LuaState* pState )
{
 // Get the argument count
 int top = pState->GetTop();
 
 for( int i = 1; i <= top; ++i )
 {
 // Retrieve all arguments, if possible they will be converted to strings
 cout << pState->CheckString(i) << std::endl;
 }
 
 // We don't return any values to the script
 return 0;
}
static uint16 debrisBits = 0x0002 ;
static uint16 boundaryBits =0x0004;
static uint16 playerFeetBits =0x0008;
static uint16 PLAYER_FEET_TOUCHING_BOUNDARY=playerFeetBits|boundaryBits;
static uint16 PLAYER_FEET_TOUCHING_DEBRIS=playerFeetBits|debrisBits;

GLuint loadTexture(string fileName, unsigned int &imageWidth, unsigned int &imageHeight, double &scaledImageWidth, double &scaledImageHeight)
{
	vector<unsigned char> image;
	  unsigned error = lodepng::decode(image, imageWidth, imageHeight, fileName);
	  //if there's an error, display it
	  if(error) {std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;return 0;}
	  if (false)
	  for (int i = 0; i <imageHeight; i++)
	  {
		for (int j = 0; j <imageWidth; j++)
			cout<<image[imageHeight*i+imageWidth]<<" ";
		cout<<endl;
	  }

  // Texture size must be power of two for the primitive OpenGL version this is written for. Find next power of two.
  size_t u2 = 1; while(u2 < imageWidth) u2 *= 2;
  size_t v2 = 1; while(v2 < imageHeight) v2 *= 2;
  // Ratio for power of two version compared to actual version, to render the non power of two image with proper size.
  scaledImageWidth = (double)imageWidth / u2;
  scaledImageHeight= (double)imageHeight / v2;

  // Make power of two version of the image.
  std::vector<unsigned char> image2(u2 * v2 * 4);
  for(size_t y = 0; y < imageHeight; y++)
  for(size_t x = 0; x < imageWidth; x++)
  for(size_t c = 0; c < 4; c++)
  {
    image2[4 * u2 * y + 4 * x + c] = image[4 * imageWidth * y + 4 * x + c];
  }

  //now fix the orientation of the image
  unsigned char *imagePtr = &image2[0];
	int halfTheHeightInPixels = imageHeight / 2;
 
	// Assuming RGBA for 4 components per pixel.
	int numColorComponents = 4;
 
	// Assuming each color component is an unsigned char.
	int widthInChars = imageWidth * numColorComponents;
 
	unsigned char *top = NULL;
	unsigned char *bottom = NULL;
	unsigned char temp = 0;
 
	for( int h = 0; h < halfTheHeightInPixels; ++h )
	{
		top = imagePtr + h * widthInChars;
		bottom = imagePtr + (imageHeight - h - 1) * widthInChars;
 
		for( int w = 0; w < widthInChars; ++w )
		{
			// Swap the chars around.
			temp = *top;
			*top = *bottom;
			*bottom = temp;
 
			++top;
			++bottom;
		}
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

LuaLevel::LuaLevel()
{
	helpImageID = loadTexture("helpscreen.png",helpImageWidth,helpImageHeight,helpImageWidthScaled,helpImageHeightScaled);
	aboutImageID = loadTexture("aboutscreen.png",aboutImageWidth,aboutImageHeight,aboutImageWidthScaled,aboutImageHeightScaled);
	menuImageID = loadTexture("titlescreen.png",menuImageWidth,menuImageHeight,menuImageWidthScaled,menuImageHeightScaled);
  
	cout<<"debris bit "<< debrisBits<<endl;
	cout<<"boundary bit "<< boundaryBits<<endl;
	cout<<"playerFeet bit "<< playerFeetBits<<endl;
	cout<<"PLAYER_FEET_TOUCHING_BOUNDARY bits"<< PLAYER_FEET_TOUCHING_BOUNDARY<<endl;
	cout<<"PLAYER_FEET_TOUCHING_Debris bits"<< PLAYER_FEET_TOUCHING_DEBRIS<<endl;
	// Init Lua
	luaPState = LuaState::Create();
	gameState = MENU;

	b2Vec2 gravity;
	gravity.Set(0.0f, -30.0f);
	m_world = new b2World(gravity);

	//~~LEVEL LOADING~~~~~~~~~~~~~~~~~~~~~~~~
	b2BodyDef bodyDef;
	bodyDef.type = b2_staticBody;
	m_groundBody = m_world->CreateBody(&bodyDef);
	//m_world->IsLocked();

	loadLevelGlobals(luaPState);

	// Open the Lua Script File
	if (luaPState->DoFile("TrainingLevel.lua"))
		if( luaPState->GetTop() == 1 )
			std::cout << "An error occured: " << luaPState->CheckString(1) << std::endl;
	LuaObject testObject = luaPState->GetGlobal("test");
	cout<<((!testObject.IsNil())?testObject.GetString():"nil") << endl;

	unloadLevelGlobals(luaPState);

	//~~~~~~PLAYER STUFF
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

	m_textLine = 30;
	m_pointCount = 0;

	m_destructionListener.luaLevel = this;
	m_world->SetDestructionListener(&m_destructionListener);
	m_world->SetContactListener(this);
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
	pstate->GetGlobals().RegisterDirect( "createEdge",*this, &LuaLevel::createAnEdge);
	pstate->GetGlobals().Register("Print",Print);
}

void LuaLevel::unloadLevelGlobals(LuaState *pstate)
{
}

LuaLevel::~LuaLevel()
{
	// By deleting the world, we delete the bomb, mouse joint, etc.
	LuaState::Destroy(luaPState);
	delete m_world;
	m_world = NULL;
}

void LuaLevel::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
{
	const b2Manifold* manifold = contact->GetManifold();

	if (manifold->pointCount == 0)
	{
		return;
	}

	b2Fixture* fixtureA = contact->GetFixtureA();
	b2Fixture* fixtureB = contact->GetFixtureB();

	b2PointState state1[b2_maxManifoldPoints], state2[b2_maxManifoldPoints];
	b2GetPointStates(state1, state2, oldManifold, manifold);

	b2WorldManifold worldManifold;
	contact->GetWorldManifold(&worldManifold);

	for (int32 i = 0; i < manifold->pointCount && m_pointCount < k_maxContactPoints; ++i)
	{
		ContactPoint* cp = m_points + m_pointCount;
		cp->fixtureA = fixtureA;
		cp->fixtureB = fixtureB;
		cp->position = worldManifold.points[i];
		cp->normal = worldManifold.normal;
		cp->state = state2[i];
		++m_pointCount;
	}
}

void LuaLevel::DrawTitle(int x, int y, const char *string)
{
    m_debugDraw.DrawString(x, y, string);
}

class QueryCallback : public b2QueryCallback
{
public:
	QueryCallback(const b2Vec2& point)
	{
		m_point = point;
		m_fixture = NULL;
	}

	bool ReportFixture(b2Fixture* fixture)
	{
		b2Body* body = fixture->GetBody();
		if (body->GetType() == b2_dynamicBody)
		{
			bool inside = fixture->TestPoint(m_point);
			if (inside)
			{
				m_fixture = fixture;

				// We are done, terminate the query.
				return false;
			}
		}

		// Continue the query.
		return true;
	}

	b2Vec2 m_point;
	b2Fixture* m_fixture;
};

void LuaLevel::MouseDown(const b2Vec2& p)
{
	m_mouseWorld = p;
}

void LuaLevel::ShiftMouseDown(const b2Vec2& p)
{
	m_mouseWorld = p;
}

void LuaLevel::MouseUp(const b2Vec2& p)
{
	m_mouseWorld = p;
}

void LuaLevel::MouseMove(const b2Vec2& p)
{
	m_mouseWorld = p;
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

		m_debugDraw.DrawString(5, m_textLine, "****PAUSED****");
		m_textLine += 15;
	}

	uint32 flags = 0;
	flags += settings->drawShapes			* b2Draw::e_shapeBit;
	flags += settings->drawJoints			* b2Draw::e_jointBit;
	flags += settings->drawAABBs			* b2Draw::e_aabbBit;
	flags += settings->drawPairs			* b2Draw::e_pairBit;
	flags += settings->drawCOMs				* b2Draw::e_centerOfMassBit;
	m_debugDraw.SetFlags(flags);

	m_world->SetWarmStarting(settings->enableWarmStarting > 0);
	m_world->SetContinuousPhysics(settings->enableContinuous > 0);
	m_world->SetSubStepping(settings->enableSubStepping > 0);

	m_pointCount = 0;

	m_world->Step(timeStep, settings->velocityIterations, settings->positionIterations);

	m_world->DrawDebugData();

	if (timeStep > 0.0f)
	{
		++m_stepCount;
	}

	if (settings->drawContactPoints)
	{
		//const float32 k_impulseScale = 0.1f;
		const float32 k_axisScale = 0.3f;

		for (int32 i = 0; i < m_pointCount; ++i)
		{
			ContactPoint* point = m_points + i;

			if (point->state == b2_addState)
			{
				// Add
				m_debugDraw.DrawPoint(point->position, 10.0f, b2Color(0.3f, 0.95f, 0.3f));
			}
			else if (point->state == b2_persistState)
			{
				// Persist
				m_debugDraw.DrawPoint(point->position, 5.0f, b2Color(0.3f, 0.3f, 0.95f));
			}

			if (settings->drawContactNormals == 1)
			{
				b2Vec2 p1 = point->position;
				b2Vec2 p2 = p1 + k_axisScale * point->normal;
				m_debugDraw.DrawSegment(p1, p2, b2Color(0.9f, 0.9f, 0.9f));
			}
			else if (settings->drawContactForces == 1)
			{
				//b2Vec2 p1 = point->position;
				//b2Vec2 p2 = p1 + k_forceScale * point->normalForce * point->normal;
				//DrawSegment(p1, p2, b2Color(0.9f, 0.9f, 0.3f));
			}

			if (settings->drawFrictionForces == 1)
			{
				//b2Vec2 tangent = b2Cross(point->normal, 1.0f);
				//b2Vec2 p1 = point->position;
				//b2Vec2 p2 = p1 + k_forceScale * point->tangentForce * tangent;
				//DrawSegment(p1, p2, b2Color(0.9f, 0.9f, 0.3f));
			}
		}
	}
}
void drawImage(GLubyte id, unsigned int width, unsigned int height, double scaledWidth, double scaledHeight)
{
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, id);
  glColor4ub(255, 255, 255, 255);
    glBegin(GL_QUADS);
      glTexCoord2d(	   		 0,			   0);	glVertex2f(	   0,	   0);
      glTexCoord2d(scaledWidth,			   0);	glVertex2f(width, 	   0);
      glTexCoord2d(scaledWidth, scaledHeight);	glVertex2f(width, height);
      glTexCoord2d(			 0, scaledHeight);	glVertex2f(    0, height);
    glEnd();
  glDisable(GL_TEXTURE_2D);
}
void LuaLevel::Step(Settings* settings, float32 &viewZoom)
{
	switch (gameState)
	{
	case MENU:
		settings->viewCenter.Set(menuImageWidth/2,menuImageHeight/2);
		viewZoom=15;
		drawImage(menuImageID, menuImageWidth, menuImageHeight, menuImageWidthScaled, menuImageHeightScaled);
		break;
	case MENU_ABOUT:
		settings->viewCenter.Set(aboutImageWidth/2,aboutImageHeight/2);
		viewZoom=15;
		drawImage(aboutImageID, aboutImageWidth, aboutImageHeight, aboutImageWidthScaled, aboutImageHeightScaled);
		break;
	case MENU_HELP:
		settings->viewCenter.Set(aboutImageWidth/2,helpImageHeight/2);
		viewZoom=15;
		drawImage(helpImageID, helpImageWidth, helpImageHeight, helpImageWidthScaled, helpImageHeightScaled);
		break;
	case GAME:
		viewZoom=1;
		settings->viewCenter.Set(0,20);
		
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
		
		b2Vec2 worldCenter = playerBody->GetWorldCenter();
        b2Vec2 linearVelocity = playerBody->GetLinearVelocity();
		
		float32 timeStep = settings->hz > 0.0f ? 1.0f / settings->hz : float32(0.0f);
        // JUMPING
        playerCanMoveUpwards -= timeStep;
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
					jumpImpulse.Set(0,2.2);
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
