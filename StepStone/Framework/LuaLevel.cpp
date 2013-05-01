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
#include <cmath>
//#include <sstream> 
#include <string>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
using namespace std;

void LuaLevelDestructionListener::SayGoodbye(b2Joint* joint)
{
}
const static uint16 debrisBits = 0x0002 ;
const static uint16 boundaryBits =0x0004;
const static uint16 playerFeetBits =0x0008;
const static uint16 PLAYER_FEET_TOUCHING_BOUNDARY=playerFeetBits|boundaryBits;
const static uint16 PLAYER_FEET_TOUCHING_DEBRIS=playerFeetBits|debrisBits;

static char controlKeyLeft = 'a';
static char controlKeyRight = 'd';
static char controlKeyJump = 'w';

LuaLevel::LuaLevel(Settings* settings)
{
	framecount=0;
	// Init Lua
	luaPState = LuaState::Create(true);
	//LuaState::Destroy(luaPState);

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
		std::cout << "An error occured: " << luaPState->StackTop().GetString() << std::endl;

	/*
	int luaError = luaPState->DoFile("Settings.lua");
	if (luaError)
		std::cout << "An error occured: " << luaPState->StackTop().GetString() << std::endl;
	*/
	//unloadLevelGlobals(luaPState);

	//~~~~~~PLAYER STUFF
	//~~~~~~~~~~~~~~~~~Box2D Stuff
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set(2,2);
	bodyDef.fixedRotation=true;
	playerBody = m_world->CreateBody(&bodyDef);

	//b2PolygonShape polygonShape;
	fixtureDef.shape = &polygonShape;
	polygonShape.SetAsBox(.76f,1.28f);
	fixtureDef.friction=0;
	fixtureDef.density=1;
	playerBody->CreateFixture(&fixtureDef);

	b2Vec2 center(0,-1.28f);
	polygonShape.SetAsBox(.76f,.2f, center, 0);
	fixtureDef.filter.categoryBits=playerFeetBits;
	fixtureDef.density=0;
	playerFeet = playerBody->CreateFixture(&fixtureDef);
	//~~~~~~~~~~~~~~~~~User Interface
	controlJump = false;
	controlLeft= false;
	controlRight=false;

	uint32 flags = 0;
	flags += b2Draw::e_shapeBit;
	flags += b2Draw::e_jointBit;
	m_debugDraw.SetFlags(flags);

	m_destructionListener.luaLevel = this;
	m_world->SetDestructionListener(&m_destructionListener);
	m_world->SetDebugDraw(&m_debugDraw);

	vector<unsigned char> image;
	image.reserve(1024*1024*4);
	unsigned char* buffer=NULL;

	vector<Graphics::Texture> textures(4);
	vector<int> framesPerImage(4);
	loadATexture("Alex\\idle\\1.png", &textures[0], image);  framesPerImage[0]=8; uniqueTextures.push_back(textures[0].id);
	loadATexture("Alex\\idle\\2.png", &textures[1], image);  framesPerImage[1]=30; uniqueTextures.push_back(textures[1].id);
	textures[2]=textures[0];																	     framesPerImage[2]=8;
	loadATexture("Alex\\idle\\3.png", &textures[3], image, buffer, LodePNGColorType::LCT_RGBA, 8U);  framesPerImage[3]=30; uniqueTextures.push_back(textures[3].id);
	animatedIdle = new Graphics::AnimatedTexture(textures,4,framesPerImage);

	textures.resize(8);
	framesPerImage.resize(8);
	loadATexture("Alex\\run\\1.png", &textures[0], image); framesPerImage[0]=5; uniqueTextures.push_back(textures[0].id);
	loadATexture("Alex\\run\\2.png", &textures[1], image); framesPerImage[1]=5; uniqueTextures.push_back(textures[1].id);
	loadATexture("Alex\\run\\3.png", &textures[2], image); framesPerImage[2]=5; uniqueTextures.push_back(textures[2].id);
	loadATexture("Alex\\run\\4.png", &textures[3], image); framesPerImage[3]=7; uniqueTextures.push_back(textures[3].id);
	loadATexture("Alex\\run\\5.png", &textures[4], image); framesPerImage[4]=5; uniqueTextures.push_back(textures[4].id);
	loadATexture("Alex\\run\\6.png", &textures[5], image); framesPerImage[5]=5; uniqueTextures.push_back(textures[5].id);
	loadATexture("Alex\\run\\7.png", &textures[6], image); framesPerImage[6]=5; uniqueTextures.push_back(textures[6].id);
	loadATexture("Alex\\run\\8.png", &textures[7], image); framesPerImage[7]=7; uniqueTextures.push_back(textures[7].id);
	animatedRun = new Graphics::AnimatedTexture(textures,8,framesPerImage);

	textures.resize(5);
	framesPerImage.resize(5);
	loadATexture("Alex\\jump\\1.png", &textures[0], image); framesPerImage[0]=10; uniqueTextures.push_back(textures[0].id);
	loadATexture("Alex\\jump\\2.png", &textures[1], image); framesPerImage[1]=10; uniqueTextures.push_back(textures[1].id);
	loadATexture("Alex\\jump\\3.png", &textures[2], image); framesPerImage[2]=10; uniqueTextures.push_back(textures[2].id);
	loadATexture("Alex\\jump\\4.png", &textures[3], image); framesPerImage[3]=10; uniqueTextures.push_back(textures[3].id);
	loadATexture("Alex\\jump\\5.png", &textures[4], image); framesPerImage[4]=10; uniqueTextures.push_back(textures[4].id);
	animatedJump = new Graphics::AnimatedTexture(textures,5,framesPerImage);

	loadATexture("helpscreen.png", &helpImage, image);					uniqueTextures.push_back(helpImage.id);
	loadATexture("aboutscreen.png", &aboutImage, image);					uniqueTextures.push_back(aboutImage.id);
	loadATexture("titlescreen.png", &menuImage, image);					uniqueTextures.push_back(menuImage.id);
	loadATexture("backdrops\\cloudyskies.png", &backdropImage, image);	uniqueTextures.push_back(backdropImage.id);
	currentAnimatedTexture = animatedIdle;
	setGameState(MENU, settings);
}

GLuint LuaLevel::bindTexture(string file)
{
	Graphics::Texture texture;
	loadATexture(file,&texture);
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

int LuaLevel::createDebris( float32 x, float32 y)
{
	float32 r = ((float) rand() / (RAND_MAX));
	bodyDef.angle = r * 360 * 3.14f / 180;
	bodyDef.position.x = x;
	bodyDef.position.y= playerBody->GetPosition().y+40;
	bodyDef.type = b2_dynamicBody;
	bodyDef.fixedRotation = false;
	//bodyDef.gravityScale = .2f;
	b2Body* body = m_world->CreateBody(&bodyDef);

	fixtureDef.filter.categoryBits = debrisBits;
	fixtureDef.density = 1;
	fixtureDef.friction = .4f;
	fixtureDef.shape = &polygonShape;
	r = ((float) rand() / (RAND_MAX));
	float32 r2 = ((float) rand() / (RAND_MAX));
	polygonShape.SetAsBox(.2f + r * .8f, .2f + r2 * .8f);
	body->CreateFixture(&fixtureDef);
	return 0;
}

void LuaLevel::loadLevelGlobals(LuaState *pstate)
{
	//Box2DFactory/Level Loading~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	LuaObject globals = pstate->GetGlobals();
	
	LuaObject metaTableObj = globals.CreateTable("box2DFactoryMetaTable");
	metaTableObj.SetObject("__index", metaTableObj);
	metaTableObj.RegisterObjectDirect("createEdge", (LuaLevel *)nullptr, &LuaLevel::createAnEdge);
	metaTableObj.RegisterObjectDirect("createDebris", (LuaLevel *)nullptr, &LuaLevel::createDebris);

	LuaObject box2DFactoryObject = pstate->BoxPointer(this);
	box2DFactoryObject.SetMetaTable(metaTableObj);
	pstate->GetGlobals().SetObject("box2DFactory", box2DFactoryObject);
	
	/*
	//BodyDef
	LuaObject bodyDefMetaTableObj = globals.CreateTable("bodyDefMetaTable");
    bodyDefMetaTableObj.RegisterObjectFunctor("__index", &LuaLevel::BodyDefIndex);
    bodyDefMetaTableObj.RegisterObjectFunctor("__newindex", &LuaLevel::BodyDefNewIndex);

	bodyDefObj = pstate->BoxPointer(this);
	bodyDefObj.SetMetaTable(bodyDefMetaTableObj);
	pstate->GetGlobals().SetObject("bodyDef", bodyDefObj);

	//FixtureDef
	LuaObject fixtureDefMetaTableObj = globals.CreateTable("fixtureDefMetaTable");
    fixtureDefMetaTableObj.RegisterObjectFunctor("__index", &LuaLevel::FixtureDefIndex);
    fixtureDefMetaTableObj.RegisterObjectFunctor("__newindex", &LuaLevel::FixtureDefNewIndex);

	fixtureDefObj = pstate->BoxPointer(this);
	fixtureDefObj.SetMetaTable(fixtureDefMetaTableObj);
	pstate->GetGlobals().SetObject("fixtureDef", fixtureDefObj);
	*/
	// Controls ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//globals.SetString("controlKeyJump",&controlKeyJump, 1);
	//globals.SetString("controlKeyRight",&controlKeyRight,1);
	//globals.SetString("controlKeyLeft",&controlKeyLeft,1);
}


int LuaLevel::BodyDefIndex(LuaState* pState){
	// Table, Key
    int top = pState->GetTop();
	//cout<<pState->Stack(1).IsLightUserData();
	//pState->BoxPointer();
    if( top == 2 && pState->Stack(1).IsUserData() && pState->Stack(2).IsString() )
    {
        // t[key]
        std::string key( pState->Stack(2).GetString() );
 
        if( key == "active" )
            pState->PushBoolean(bodyDef.active);
        else if( key == "allowSleep" )
			pState->PushBoolean(bodyDef.allowSleep);
        else if( key == "angle" )
			pState->PushNumber(bodyDef.angle);
        else if( key == "angularDamping" )
			pState->PushNumber(bodyDef.angularDamping);
        else if( key == "angularVelocity" )
			pState->PushNumber(bodyDef.angularVelocity);
        else if( key == "awake" )
			pState->PushBoolean(bodyDef.awake);
        else if( key == "fixedRotation" )
			pState->PushBoolean(bodyDef.fixedRotation);
        else if( key == "gravityScale" )
			pState->PushNumber(bodyDef.gravityScale);
        else if( key == "linearDamping" )
			pState->PushNumber(bodyDef.linearDamping);
        else if( key == "linearVelocity" )
		{
			pState->PushNumber(bodyDef.linearVelocity.x);
			pState->PushNumber(bodyDef.linearVelocity.x); //todo
		}
		else if( key == "position" )
		{
			pState->PushNumber(bodyDef.position.x);
			pState->PushNumber(bodyDef.position.x); //todo
		}
        else if( key == "type" )
			pState->PushNumber(bodyDef.type);
    }
    // return the count of our pushed values
    return pState->GetTop() - top;
}

int LuaLevel::BodyDefNewIndex(LuaState* pState){
    // Table, Key, Value
    if( pState->GetTop() == 3 && pState->Stack(1).IsUserData() && pState->Stack(2).IsString())
    {
        // t[key]
        std::string key( pState->Stack(2).GetString() );
 
		if (pState->Stack(3).IsBoolean())
		{
			if( key == "active" )
				bodyDef.active = pState->Stack(3).GetBoolean();
			else if( key == "allowSleep" )
				bodyDef.active = pState->Stack(3).GetBoolean();
		}
    }
    // We don't return any values here
    return 0;
}


int LuaLevel::FixtureDefIndex(LuaState* pState)
{
    int top = pState->GetTop();
    if( top == 2 && pState->Stack(1).IsUserData() && pState->Stack(2).IsString() )
    {
        // t[key]
        std::string key( pState->Stack(2).GetString() );
 
        if( key == "denisty" )
            pState->PushNumber(fixtureDef.density);
        else if( key == "friction" )
			pState->PushNumber(fixtureDef.friction);
        else if( key == "isSensor" )
			pState->PushBoolean(fixtureDef.isSensor);
        else if( key == "restitution" )
			pState->PushNumber(fixtureDef.restitution);
        else if( key == "shape" )
		{
			if (fixtureDef.shape == &circleShape)
				circleShapeObj.Push();
			else if (fixtureDef.shape==&edgeShape)
				edgeShapeObj.Push();
			else if (fixtureDef.shape==NULL)
				pState->PushString("nothing");
		}

    }
    // return the count of our pushed values
    return pState->GetTop() - top;
}
int LuaLevel::FixtureDefNewIndex(LuaState* pState)
{
	return 0;
}

int PolygonShapeIndex(LuaState* pState)
{
	return 0;
}
int PolygonShapeNewIndex(LuaState* pState)
{
	return 0;
}
int edgeShapeIndex(LuaState* pState)
{
	return 0;
}
int edgeShapeNewIndex(LuaState* pState)
{
	return 0;
}
int circleShapeIndex(LuaState* pState)
{
	return 0;
}
int circleShapeNewIndex(LuaState* pState)
{
	return 0;
}

inline void checkAndSetChar(char &aChar, LuaObject luaObject)
{
	if (luaObject.IsString())
		aChar = luaObject.GetString()[0];
}

void LuaLevel::unloadLevelGlobals(LuaState *pstate)
{
	//controls
	//checkAndSetChar(controlKeyLeft, pstate->GetGlobal("controlKeyLeft"));
	//checkAndSetChar(controlKeyRight, pstate->GetGlobal("controlKeyRight"));
	//checkAndSetChar(controlKeyJump, pstate->GetGlobal("controlKeyJump"));
	//functions
	if (pstate->GetGlobal("step").IsFunction())
	{
		luaStepFunction = pstate->GetGlobal("step");
	}
	/*
	pstate->GetGlobals().SetNil("box2DFactory");
	pstate->GetGlobals().SetNil("controlKeyLeft");
	pstate->GetGlobals().SetNil("controlKeyRight");
	pstate->GetGlobals().SetNil("controlKeyJump");
	pstate->GetGlobals().SetNil("bodyDef");
	pstate->GetGlobals().SetNil("fixtureDef");
	*/
}

LuaLevel::~LuaLevel()
{
	// Deleting all of our textures in 1 fell swoop
	glDeleteTextures(uniqueTextures.size(), &uniqueTextures[0]);
	delete animatedIdle;
	delete animatedRun;
	delete animatedJump;

	// Deleting our lua state/context
	LuaState::Destroy(luaPState);
	delete m_world;
}

void LuaLevel::DrawTitle(int x, int y, const char *string)
{
	m_debugDraw.DrawString(x, y, string);
}

void LuaLevel::drawGame(Settings* settings, float32 timeStep)
{
	if (settings->getPause())
		if (settings->getSingleStep())
			settings->setSingleStep(0);
		else
			timeStep = 0.0f;

	m_world->Step(timeStep, 8, 3);

	m_world->DrawDebugData();
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

		if (PLAYER_FEET_TOUCHING_BOUNDARY == collision || PLAYER_FEET_TOUCHING_DEBRIS == collision) 
		{
			isFeetTouchingBoundary = true;
			c->GetWorldManifold(&worldManifold);
			float normalImpulse = c->GetManifold()->points->normalImpulse;
			if (PLAYER_FEET_TOUCHING_DEBRIS == collision && (normalImpulse>5 || normalImpulse<-5))
			{
				cout<<c->GetManifold()->points->normalImpulse<<"\t"<<worldManifold.normal.y<<" from "<<
									((c->GetFixtureA()->GetBody()==playerBody)?"player":"something")<<" to "<<((c->GetFixtureB()->GetBody()==playerBody)?"player":"something")<<endl;
			}
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

void LuaLevel::processInputForGame(Settings *settings, float32 timeStep)
{
	b2Vec2 worldCenter = playerBody->GetWorldCenter();
	b2Vec2 linearVelocity = playerBody->GetLinearVelocity();

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
			if (currentAnimatedTexture!=animatedJump)
				currentAnimatedTexture=animatedJump;
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
		if (currentAnimatedTexture!=animatedIdle && isFeetTouchingBoundary == true)
			currentAnimatedTexture=animatedIdle;
		if (wasMoving) {
			for (b2Contact *c = m_world->GetContactList() ; c ; c = c->GetNext())
				c->ResetFriction();
			wasMoving = false;
		}
	} else {
		b2Vec2 force(vx, 0);
		if (vx > 0 && linearVelocity.x < 8) {
			playerBody->ApplyForce(force, worldCenter);
			isFacingRight = true;
			//moving right
		} else if (vx < 0 && linearVelocity.x > -8) {
			playerBody->ApplyForce(force, worldCenter);
			//moving left
			isFacingRight = false;
		}
		playerFeet->SetFriction(0);
		if (currentAnimatedTexture!=animatedRun && isFeetTouchingBoundary == true)
			currentAnimatedTexture=animatedRun;
		if (!wasMoving) {
			for (b2Contact *c = m_world->GetContactList() ; c ; c = c->GetNext())
				c->ResetFriction();
			wasMoving = true;
		}
	}
}


void LuaLevel::Step(Settings* settings)
{
	switch (gameState)
	{
	case MENU:
		glColor4ub(255, 255, 255, 255);
		drawImage(&menuImage);
		break;
	case MENU_ABOUT:
		glColor4ub(255, 255, 255, 255);
		drawImage(&aboutImage);
		break;
	case MENU_HELP:
		glColor4ub(255, 255, 255, 255);
		drawImage(&helpImage);
		break;
	case GAME:		
		float32 timeStep = settings->getHz() > 0.0f ? 1.0f / settings->getHz() : float32(0.0f);
		processCollisionsForGame();
		processInputForGame(settings, timeStep);
		
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glColor4f(1,1,1,1);
		glPushMatrix();
		const float backdropSize = 30.f/1;
		const float backdropScale = backdropSize/backdropImage.imageWidth;
		glScalef(backdropScale,backdropScale,1);
		for (int i = 0; i<1; i++)
		{
			drawImage(&backdropImage);
			glTranslatef((float)backdropImage.imageWidth,0,0);
		}
		glPopMatrix();

		b2Vec2 worldCenter = playerBody->GetWorldCenter();
		glPushMatrix();
		glTranslatef(worldCenter.x,worldCenter.y-1.28f-.2f*2,0);
		const float32 scale = .025f;
		glScalef(scale*(isFacingRight?1:-1),scale,scale);
		Graphics::Texture currentTexture = currentAnimatedTexture->updateAndGetTexture();
		glTranslatef(currentTexture.imageWidth/(-2.0f),0,0);
		drawImage(&currentTexture);
		glPopMatrix();

		//static LuaFunction<void> stepFunction = luaStepFunction;
		//stepFunction(timeStep);
		glDisable(GL_TEXTURE_2D);
		drawGame(settings, timeStep);
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

void LuaLevel::setGameState(GameState state, Settings* settings)
{
	gameState = state;
	if (gameState==GAME)
	{
		settings->setViewZoom(1);
		settings->setViewCenter(b2Vec2(0,20));
		glClearColor(0,0,0,1);

	}
	else
	{
		glClearColor(97/255.f,117/255.f,113/255.f,1);
		glEnable(GL_TEXTURE_2D);
		settings->setViewCenter(b2Vec2(0,20));
		//settings->setViewCenter(b2Vec2((float32)aboutImage.imageWidth/2,(float32)helpImage.imageHeight/2));
		settings->setViewZoom(41);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~	INPUT HANDLING	~```~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void LuaLevel::Keyboard(unsigned char key, Settings* settings)
{
	switch (key)
	{
	case '1':
		setGameState(GAME, settings);
		break;
	case 27: //ESCAPE KEY
		if (gameState==MENU)
			glutLeaveMainLoop();
	case '2':
		setGameState(MENU, settings);
		break;
	case '3':
		setGameState(MENU_ABOUT, settings);
		break;
	case '4':
		setGameState(MENU_HELP, settings);
		break;
	case '5':
		{
			int luaError = luaPState->DoFile("testscript.lua");
			if (luaError)
				std::cout << "An error occured: " << luaPState->StackTop().GetString() << std::endl;

			break;
		}
	default:
		if (key==controlKeyLeft)
			controlLeft = true;
		else if (key==controlKeyRight)
			controlRight = true;
		else if (key==controlKeyJump)
			controlJump= true;
		break;
	}
}

void LuaLevel::KeyboardUp(unsigned char key)
{
	if (key==controlKeyLeft)
		controlLeft = false;
	else if (key==controlKeyRight)
		controlRight = false;
	else if (key==controlKeyJump)
		controlJump= false;
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