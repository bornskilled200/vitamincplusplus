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
#include <string>

using namespace std;

void LuaLevelDestructionListener::SayGoodbye(b2Joint* joint)
{
}
const static uint16 debrisBits = 1<<1;
const static uint16 boundaryBits =1<<2;
const static uint16 playerFeetBits =1<<3;
const static uint16 playerBodyBits =1<<4;
const static uint16 playerShieldBits =1<<5;
const static uint16 PLAYER_FEET_TOUCHING_BOUNDARY=playerFeetBits|boundaryBits;
const static uint16 PLAYER_FEET_TOUCHING_DEBRIS=playerFeetBits|debrisBits;
const static uint16 PLAYER_BODY_TOUCHING_DEBRIS=playerBodyBits|debrisBits;
const static uint16 PLAYER_SHIELD_TOUCHING_DEBRIS=playerShieldBits|debrisBits;

static char controlKeyLeft = 'a';
static char controlKeyRight = 'd';
static char controlKeyJump = ' ';

LuaLevel::LuaLevel(Settings* settings):m_world(NULL)
{
	// Init Lua
	luaPState = LuaState::Create(true);
	//init();
	vector<unsigned char> image;
	image.reserve(1024*1024*4);

	vector<Graphics::Texture> textures(4);
	vector<int> framesPerImage(4);
	loadATexture("Alex\\idle\\1.png", &textures[0], image);  framesPerImage[0]=8;  uniqueTextures.push_back(textures[0].id);
	loadATexture("Alex\\idle\\2.png", &textures[1], image);  framesPerImage[1]=30; uniqueTextures.push_back(textures[1].id);
	textures[2]=textures[0];						         framesPerImage[2]=8;
	loadATexture("Alex\\idle\\3.png", &textures[3], image);  framesPerImage[3]=30; uniqueTextures.push_back(textures[3].id);
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
	loadATexture("CGs\\openingcgalex.png", &introImage, image);					uniqueTextures.push_back(introImage.id);
	loadATexture("CGs\\final.png", &winImage, image);					uniqueTextures.push_back(winImage.id);
	loadATexture("backdrops\\cloudyskies.png", &backdropImage, image);	uniqueTextures.push_back(backdropImage.id);
	currentAnimatedTexture = animatedIdle;

	//Menu Music
	loadMp3File("title\\music.mp3", &menuMusic);
	menuMusic.loop=true;
	currentMusic = &menuMusic;
	playMp3File(&menuMusic);


	luaPState->GetGlobals().SetNumber("GAME",GAME);
	luaPState->GetGlobals().RegisterDirect("createButton", *this, &LuaLevel::createButton);
	//menu lua
	if (luaPState->DoFile("Menu.lua"))
		std::cout << "An error occured: " << luaPState->StackTop().GetString() << std::endl;

	setGameState(MENU, settings);

}
int LuaLevel::createButton(float x, float y, const char* file1, const char* file2, int state)
{
	Graphics::Texture hovering,standard;
	Graphics::loadATexture(file1, &standard);
	Graphics::loadATexture(file2, &hovering);
	uniqueTextures.push_back(hovering.id);
	uniqueTextures.push_back(standard.id);
	buttons.push_back(Button(x,y,hovering,standard, state));
	return 0;
}

void LuaLevel::init()
{
	// Init Box2D World
	b2Vec2 gravity;
	gravity.Set(0.0f, -30.0f);
	if (m_world)
		delete m_world;
	m_world = new b2World(gravity);

	//~~LEVEL LOADING~~~~~~~~~~~~~~~~~~~~~~~~
	b2BodyDef bodyDef;
	bodyDef.type = b2_staticBody;
	m_groundBody = m_world->CreateBody(&bodyDef);

	loadLevelGlobals(luaPState);

	// Open the Lua Script File
	if (luaPState->DoFile("TrainingLevel.lua"))
		std::cout << "An error occured: " << luaPState->StackTop().GetString() << std::endl;


	if (luaPState->DoFile("Settings.lua"))
		std::cout << "An error occured: " << luaPState->StackTop().GetString() << std::endl;

	unloadLevelGlobals(luaPState);

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
	fixtureDef.filter.categoryBits=playerBodyBits;
	fixtureDef.density=1;
	playerBox = playerBody->CreateFixture(&fixtureDef);

	b2Vec2 center(0,-1.28f);
	polygonShape.SetAsBox(.76f,.2f, center, 0);
	fixtureDef.filter.categoryBits=playerFeetBits;
	fixtureDef.density=0;
	playerFeet = playerBody->CreateFixture(&fixtureDef);

	fixtureDef.isSensor = true;
	circleShape.m_radius = .76f;
	circleShape.m_p.Set(0,1.28f);
	fixtureDef.shape = &circleShape;
	fixtureDef.filter.categoryBits = playerShieldBits;
	playerShield = playerBody->CreateFixture(&fixtureDef);

	fixtureDef.isSensor = false;
	fixtureDef.shape = &polygonShape;

	// Winning trigger
	b2Vec2 d;
	d.Set(15, 10);
	b2Vec2 p(15,90);
	aabb.lowerBound = p - d;
	aabb.upperBound = p + d;
	//~~~~~~~~~~~~~~~~~User Interface
	controlJump=false;
	controlLeft= false;
	controlRight=false;

	uint32 flags = 0;
	flags += b2Draw::e_shapeBit;
	flags += b2Draw::e_jointBit;
	m_debugDraw.SetFlags(flags);

	m_destructionListener.luaLevel = this;
	m_world->SetDestructionListener(&m_destructionListener);
	m_world->SetDebugDraw(&m_debugDraw);
}

GLuint LuaLevel::bindTexture(string file)
{
	Graphics::Texture texture;
	loadATexture(file,&texture);
	return texture.id;
}
static const b2FixtureDef defaultFixtureDef;
int LuaLevel::createAnEdge( float32 x1, float32 y1, float32 x2, float32 y2 )
{
	fixtureDef = defaultFixtureDef;
	fixtureDef.filter.categoryBits=boundaryBits;
	edgeShape.Set(b2Vec2(x1,y1),b2Vec2(x2,y2));
	fixtureDef.shape = &edgeShape;
	m_groundBody->CreateFixture(&fixtureDef);
	return 0;
}
int LuaLevel::createBox( float32 x, float32 y, float32 w, float32 h)
{
	fixtureDef = defaultFixtureDef;
	fixtureDef.filter.categoryBits=boundaryBits;
	polygonShape.SetAsBox(w/2,h/2,b2Vec2(x+w/2,y+h/2),0);
	fixtureDef.shape = &polygonShape;
	m_groundBody->CreateFixture(&fixtureDef);
	return 0;
}

int LuaLevel::createDebris( float32 x, float32 y)
{
	float32 r = ((float) rand() / (RAND_MAX));
	bodyDef.angle = r * 360 * 3.14f / 180;
	bodyDef.position.x = x;
	bodyDef.position.y= playerBody->GetPosition().y+30;
	bodyDef.type = b2_dynamicBody;
	bodyDef.fixedRotation = false;
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
	metaTableObj.RegisterObjectDirect("createBox", (LuaLevel *)nullptr, &LuaLevel::createBox);
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
	globals.SetString("controlKeyJump",&controlKeyJump, 1);
	globals.SetString("controlKeyRight",&controlKeyRight,1);
	globals.SetString("controlKeyLeft",&controlKeyLeft,1);


	//Level specific stuff
	globals.SetNil("music");
}

inline void checkAndSetChar(char &aChar, LuaObject luaObject)
{
	if (luaObject.IsString())
		aChar = luaObject.GetString()[0];
}

inline void checkAndSetFloat(float &aFloat, LuaObject luaObject)
{
	if (luaObject.IsNumber())
		aFloat = (float)luaObject.GetNumber();
}

void LuaLevel::unloadLevelGlobals(LuaState *pstate)
{
	//controls
	checkAndSetChar(controlKeyLeft, pstate->GetGlobal("controlKeyLeft"));
	checkAndSetChar(controlKeyRight, pstate->GetGlobal("controlKeyRight"));
	checkAndSetChar(controlKeyJump, pstate->GetGlobal("controlKeyJump"));

	//level specific stuffs
	//music
	if (pstate->GetGlobal("music").IsString())
	{
		loadMp3File(pstate->GetGlobal("music").GetString(),&gameMusic);
	}
	if (pstate->GetGlobal("tile1ImageFile").IsString())
	{
		Graphics::loadATexture(pstate->GetGlobal("tile1ImageFile").GetString(),&tile1Image);
		if (pstate->GetGlobal("tile1ImageWidth").IsNumber())
			tile1Image.imageWidth = (unsigned int)pstate->GetGlobal("tile1ImageWidth").GetNumber();
		if (pstate->GetGlobal("tile1ImageHeight").IsNumber())
			tile1Image.imageHeight = (unsigned int)pstate->GetGlobal("tile1ImageHeight").GetNumber();

		if (pstate->GetGlobal("tile1ImageDrawList").IsTable())
		{
			tile1ImageDrawList = pstate->GetGlobal("tile1ImageDrawList");
			if (tile1ImageDrawList.GetN()%4)
				cout<<"Drawing list is not a mutliple of 4"<<endl;
			//cout<<tile1ImageDrawList.GetByIndex(1).TypeName()<<tile1ImageDrawList.GetByIndex(1).IsInteger()<<" "<<tile1ImageDrawList.GetByIndex(1).IsNumber()<<endl;
		}
	}
	if (pstate->GetGlobal("tile2ImageFile").IsString())
	{
		Graphics::loadATexture(pstate->GetGlobal("tile2ImageFile").GetString(),&tile2Image);
		if (pstate->GetGlobal("tile2ImageWidth").IsNumber())
			tile2Image.imageWidth = (unsigned int)pstate->GetGlobal("tile2ImageWidth").GetNumber();
		if (pstate->GetGlobal("tile2ImageHeight").IsNumber())
			tile2Image.imageHeight = (unsigned int)pstate->GetGlobal("tile2ImageHeight").GetNumber();

		if (pstate->GetGlobal("tile2ImageDrawList").IsTable())
		{
			tile2ImageDrawList = pstate->GetGlobal("tile2ImageDrawList");
			if (tile2ImageDrawList.GetN()%4)
				cout<<"Drawing list is not a mutliple of 4"<<endl;
		}
	}
	if (pstate->GetGlobal("backgroundImageFile").IsString())
	{
		Graphics::loadATexture(pstate->GetGlobal("backgroundImageFile").GetString(),&backgroundImage);
		if (pstate->GetGlobal("backgroundImageWidth").IsNumber())
			backgroundImage.imageWidth = (unsigned int)pstate->GetGlobal("backgroundImageWidth").GetNumber();
		if (pstate->GetGlobal("backgroundImageHeight").IsNumber())
			backgroundImage.imageHeight = (unsigned int)pstate->GetGlobal("backgroundImageHeight").GetNumber();
	}

	//functions
	if (pstate->GetGlobal("step").IsFunction())
	{
		luaStepFunction = pstate->GetGlobal("step");
	}

}

LuaLevel::~LuaLevel()
{
	// Deleting all of our textures in 1 fell swoop
	glDeleteTextures(uniqueTextures.size(), &uniqueTextures[0]);
	delete animatedIdle;
	delete animatedRun;
	delete animatedJump;

	//need to reset luastepfunction to destroy luastate
	luaStepFunction.Reset();
	// Deleting our lua state/context,
	LuaState::Destroy(luaPState);
	delete m_world;

	if (currentMusic)
		Pa_AbortStream(currentMusic->pStream);
	terminateSound();
}

void LuaLevel::drawGame(Settings* settings, float32 timeStep)
{
	if (settings->getPause())
		if (settings->getSingleStep())
			settings->setSingleStep(0);
		else
			timeStep = 0.0f;

	if (luaStepFunction.IsFunction())
	{
		LuaFunction<void> stepFunction = luaStepFunction;
		stepFunction(timeStep);
		//tile1ImageDrawList = luaPState->GetGlobal("tile1ImageDrawList");
	}

	m_world->Step(timeStep, 8, 3);

	m_world->DrawDebugData();
}

void LuaLevel::processCollisionsForGame(Settings* settings)
{
	//winning
	if (playerBody->GetPosition().y>60)
	{
		setGameState(GAME_WIN,settings);
		//init();
		return;
	}

	//Shield
	/*
	if (playerBody->GetLinearVelocity().y<10)
	{
	playerShield->SetSensor(true);
	playerShield->SetDensity(0);
	}
	else
	{
	playerShield->SetSensor(false);
	playerShield->SetDensity(1);
	}
	playerBody->ResetMassData();
	*/
	isFeetTouchingBoundary = canJump = canKickOff = false;
	slowDown = false;
	b2WorldManifold worldManifold;
	for (b2ContactEdge *ce = playerBody->GetContactList() ; ce ; ce = ce->next)
	{
		b2Contact *c = ce->contact;
		if (!c->IsTouching())
			continue;

		short collision = c->GetFixtureA()->GetFilterData().categoryBits | c->GetFixtureB()->GetFilterData().categoryBits;

		// ~~~~~~~~~~~~~~ YOU LOOSE EFFECT && slowdown effect
		if (collision == PLAYER_BODY_TOUCHING_DEBRIS || collision == PLAYER_SHIELD_TOUCHING_DEBRIS)
		{
			b2Body* debrisBody;
			if (c->GetFixtureA()->GetFilterData().categoryBits==debrisBits)
			{
				debrisBody = c->GetFixtureA()->GetBody();
			}
			else
			{
				debrisBody = c->GetFixtureB()->GetBody();
			}
			b2Vec2 debrisSpeed = debrisBody->GetLinearVelocity();

			// loose
			if (collision == PLAYER_BODY_TOUCHING_DEBRIS)
			{
				if (playerBody->GetLinearVelocity().y<15 && debrisSpeed.Length()>15 && debrisBody->GetPosition().y>playerBody->GetPosition().y+1.28f)
				{
					settings->setPause(true);
					//setGameState(MENU_HELP,settings);
					//init();
					return;
				}
			}
			else if (collision == PLAYER_SHIELD_TOUCHING_DEBRIS)//slowdown
			{
				if (debrisBody->GetLinearVelocity().Length()>13)
				{
					slowDown = true;
				}
			}
		}

		if (PLAYER_FEET_TOUCHING_BOUNDARY == collision || PLAYER_FEET_TOUCHING_DEBRIS == collision) 
		{
			isFeetTouchingBoundary = true;
			c->GetWorldManifold(&worldManifold);
			b2Vec2 pos = playerBody->GetPosition();
			bool below = true, side = true;
			for(int j = 0; j < c->GetManifold()->pointCount; j++) {
				below &= (worldManifold.points[j].y < pos.y - 1.28f);
				side &= (worldManifold.points[j].x < pos.x - .76f) | (worldManifold.points[j].x > pos.x + .76f);
			}

			if (below)
			{
				canJump = true;
				if (currentAnimatedTexture==animatedJump && justJumped==false)
					currentAnimatedTexture=animatedIdle;
			}
			else if (side) {
				canKickOff = true;
				justJumped = false;
			}
		}
	}

	if (isFeetTouchingBoundary == false) {
		justKickedOff = false;
		justJumped = false;
		if (currentAnimatedTexture!=animatedJump)
			currentAnimatedTexture=animatedJump;
	}
}

void LuaLevel::processInputForGame(Settings *settings, float32 timeStep)
{
	b2Vec2 worldCenter = playerBody->GetWorldCenter();
	b2Vec2 linearVelocity = playerBody->GetLinearVelocity();

	// JUMPING
	if (playerCanMoveUpwards>=0) playerCanMoveUpwards -= timeStep;
	if (controlJump) {
		if (canJump && justJumped==false)
		{
			playerBody->ApplyLinearImpulse(b2Vec2(0,15), worldCenter);
			playerCanMoveUpwards = .3f;
			justJumped = true;
			if (currentAnimatedTexture!=animatedJump)
				currentAnimatedTexture=animatedJump;
		}
		else if (canKickOff && justKickedOff==false)
		{
			playerBody->ApplyLinearImpulse(b2Vec2(0,20), worldCenter);
			justKickedOff = true;
		}
		else if (playerCanMoveUpwards > 0)
		{
			playerBody->ApplyLinearImpulse(b2Vec2(0,2.2f), worldCenter);
		}
	}
	// HORIZONTAL MOVEMENT
	float32 vx = 0;
	if (controlLeft)
		vx += -200;
	if (controlRight)
		vx += 200;

	if (vx == 0) {
		playerFeet->SetFriction(5);
		if (currentAnimatedTexture!=animatedIdle && isFeetTouchingBoundary == true && currentAnimatedTexture!=animatedJump && justJumped==false)
			currentAnimatedTexture=animatedIdle;
		if (wasMoving) {
			for (b2ContactEdge *c = playerBody->GetContactList() ; c ; c = c->next)
			{
				c->contact->ResetFriction();
			}
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
		if (currentAnimatedTexture!=animatedRun && isFeetTouchingBoundary == true && currentAnimatedTexture!=animatedJump && justJumped==false)
			currentAnimatedTexture=animatedRun;
		if (!wasMoving) {
			for (b2ContactEdge *c = playerBody->GetContactList() ; c ; c = c->next)
			{
				c->contact->ResetFriction();
			}
			wasMoving = true;
		}
	}
	b2Vec2 viewportPosition = settings->getViewPosition();
	viewportPosition.y = max(viewportPosition.y-(viewportPosition.y-(worldCenter.y-10)) * .2f,0.0f);
	settings->setViewPosition(viewportPosition);
}


b2Vec2 mouse;
void LuaLevel::Step(Settings* settings)
{
	switch (gameState)
	{
	case MENU:
		glColor4ub(255, 255, 255, 255);
		drawImage(&menuImage);
		for (unsigned int i = 0; i<buttons.size();i++)
		{
			if (mouse.x>buttons[i].x && mouse.x<buttons[i].x+buttons[i].standard.imageWidth &&
				mouse.y>buttons[i].y && mouse.y<buttons[i].y+buttons[i].standard.imageHeight)
				Graphics::drawImage(&buttons[i].hovering,(unsigned int)buttons[i].x,(unsigned int)buttons[i].y,buttons[i].hovering.imageWidth,buttons[i].hovering.imageHeight);
			else	
				Graphics::drawImage(&buttons[i].standard,(unsigned int)buttons[i].x,(unsigned int)buttons[i].y,buttons[i].hovering.imageWidth,buttons[i].hovering.imageHeight);
		}
		break;
	case MENU_ABOUT:
		glColor4ub(255, 255, 255, 255);
		drawImage(&aboutImage);
		break;
	case MENU_HELP:
		glColor4ub(255, 255, 255, 255);
		drawImage(&helpImage);
		break;
	case GAME_WIN:
		glColor4ub(255, 255, 255, 255);
		drawImage(&winImage);
		break;
	case GAME_INTRO:
		glColor4ub(255, 255, 255, 255);
		drawImage(&introImage);
		break;
	case GAME:		
		float32 timeStep = settings->getHz() > 0.0f ? 1.0f / settings->getHz() : float32(0.0f);

		processCollisionsForGame(settings);
		if (slowDown)
			timeStep/=20;
		processInputForGame(settings, timeStep);

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glColor4f(1,1,1,1);
		/*
		glPushMatrix();
		const float backdropSize = 30.f/1;
		const float backdropScale = backdropSize/backdropImage.imageWidth;
		glScalef(backdropScale,backdropScale,1);
		for (int i = 0; i<1; i++)
		{

		glTranslatef((float)backdropImage.imageWidth,0,0);
		}
		glPopMatrix();
		*/
		//Graphics::drawImage(&backdropImage,10,10);
		if (Graphics::isValidTexture(backgroundImage))
			Graphics::drawImage(&backgroundImage);
		if (Graphics::isValidTexture(tile1Image))
		{
			if (tile1ImageDrawList.IsTable())
			{
				int drawListLength = tile1ImageDrawList.GetN();
				for (int i = 1; i <= drawListLength-3; i+=4)
				{
					Graphics::drawImage(&tile1Image, 
						(unsigned int)tile1ImageDrawList.GetByIndex(i).GetInteger(),(unsigned int)tile1ImageDrawList.GetByIndex(i+1).GetInteger(),
						(unsigned int)tile1ImageDrawList.GetByIndex(i+2).GetInteger(),(unsigned int)tile1ImageDrawList.GetByIndex(i+3).GetInteger());
				}
			}
		}
		b2Vec2 worldCenter = playerBody->GetWorldCenter();
		glPushMatrix();
		if (playerBody->GetLinearVelocity().y>15)
			glColor4f(1,1,1,.5f);
		glTranslatef(worldCenter.x,worldCenter.y-1.28f-.2f*2,0);
		const float32 scale = .025f;
		glScalef(scale*(isFacingRight?1:-1),scale,scale);
		Graphics::Texture currentTexture = currentAnimatedTexture->updateAndGetTexture();
		glTranslatef(currentTexture.imageWidth/(-2.0f),0,0);
		drawImage(&currentTexture);
		glPopMatrix();
		//glDisable(GL_TEXTURE_2D);
		drawGame(settings, timeStep);
		break;
	}
	m_debugDraw.DrawPoint(mouse, 2, b2Color(1,1,1));
	//m_debugDraw.DrawPoint(mouse, 2, b2Color(1,1,1));
	if (settings->getPause())
	{

		m_debugDraw.DrawString(200,200,"%d",glutGet(GLUT_WINDOW_HEIGHT));
		for (b2Body *body = m_world->GetBodyList() ; body ; body = body->GetNext())
		{
			b2Vec2 pos = body->GetPosition();
			b2Vec2 vel = body->GetLinearVelocity();
			m_debugDraw.DrawString(int(pos.x*(640/30)), 480-int(pos.y*(480/25)), "%.2f, %.2f", vel.x, vel.y);

		}


		m_debugDraw.DrawString(0, 15, "key 1: Game");
		m_debugDraw.DrawString(0, 30, "key 2: Menu");
		m_debugDraw.DrawString(0, 45, "key 3: About");
		m_debugDraw.DrawString(0, 60, "key 4: Help");
		m_debugDraw.DrawString(0, 75, "isplayerfeettouchingground %s", isFeetTouchingBoundary?"true":"false");
		m_debugDraw.DrawString(0, 90, "canJump %s", canJump?"true":"false");
		m_debugDraw.DrawString(0, 105, "justJumped %s", justJumped?"true":"false");
		m_debugDraw.DrawString(0, 130, "playerCanMoveUpwards %f",playerCanMoveUpwards);
	}
}

void LuaLevel::setGameState(GameState state, Settings* settings)
{
	gameState = state;
	if (gameState==GAME)
	{
		init();
		settings->setViewSize(30);
		settings->widthIsConstant = true;
		settings->setViewPosition(b2Vec2(0,0));
		glClearColor(0,0,0,1);
		if (currentMusic != &gameMusic)
		{
			if (currentMusic)
				Pa_AbortStream(currentMusic->pStream);
			if (gameMusic.loaded.size()!=0)
			{
				gameMusic.pos = 0;
				currentMusic = &gameMusic;
				playMp3File(&gameMusic);
			}
		}
	}
	else
	{
		glClearColor(97/255.f,117/255.f,113/255.f,1);
		glEnable(GL_TEXTURE_2D);
		settings->setViewPosition(b2Vec2(0,0));
		settings->widthIsConstant = false;
		settings->setViewSize(1024);

		if (state!=GAME_WIN || state!= GAME_INTRO)
		{
			if (currentMusic != &menuMusic)
			{
				if (currentMusic)
					Pa_AbortStream(currentMusic->pStream);

				menuMusic.pos = 0;
				currentMusic = &menuMusic;
				playMp3File(&menuMusic);
			}
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~	INPUT HANDLING	~```~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void LuaLevel::Keyboard(unsigned char key, Settings* settings)
{
	if (gameState==GAME_INTRO)
	{
		setGameState(GAME, settings);
	}
	else if (gameState==GAME_WIN && key==27)
		setGameState(MENU, settings);
	else
		switch (key)
	{
		case '1':
			setGameState(GAME_INTRO, settings);
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
		case 'r':
			setGameState(GAME,settings);
			settings->setPause(0);
			break;
		case '5':
			if (luaPState->DoFile("testscript.lua"))
				std::cout << "An error occured: " << luaPState->StackTop().GetString() << std::endl;
			break;
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

void LuaLevel::MouseDown(const b2Vec2& p, Settings *settings)
{
	mouse = p;

	for (unsigned int i = 0; i<buttons.size();i++)
	{
		if (mouse.x>buttons[i].x && mouse.x<buttons[i].x+buttons[i].standard.imageWidth &&
			mouse.y>buttons[i].y && mouse.y<buttons[i].y+buttons[i].standard.imageHeight)
			setGameState((GameState)buttons[i].state, settings);
		else	
			Graphics::drawImage(&buttons[i].standard,(unsigned int)buttons[i].x,(unsigned int)buttons[i].y,buttons[i].hovering.imageWidth,buttons[i].hovering.imageHeight);
	}
}

void LuaLevel::ShiftMouseDown(const b2Vec2& p)
{
	mouse = p;
}

void LuaLevel::MouseUp(const b2Vec2& p)
{
	mouse = p;
}

void LuaLevel::MouseMove(const b2Vec2& p)
{
	mouse = p;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~ LUA STUFF

int LuaLevel::BodyDefIndex(LuaState* pState){
	// Table, Key
	int top = pState->GetTop();
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