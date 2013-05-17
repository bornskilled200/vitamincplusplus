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
#include <limits>       // std::numeric_limits

using namespace std;

void LuaLevelDestructionListener::SayGoodbye(b2Joint* joint)
{
}
const static uint16 debrisBits = 1<<1;
const static uint16 boundaryBits =1<<2;
const static uint16 playerFeetBits =1<<3;
const static uint16 playerBodyBits =1<<4;
const static uint16 PLAYER_FEET_TOUCHING_BOUNDARY=playerFeetBits|boundaryBits;
const static uint16 PLAYER_FEET_TOUCHING_DEBRIS=playerFeetBits|debrisBits;
const static uint16 PLAYER_BODY_TOUCHING_DEBRIS=playerBodyBits|debrisBits;

LuaLevel::LuaLevel(Settings* settings):m_world(NULL),currentLevelLuaFile("TrainingLevel.lua"),slowdownBy(50),currentVoice(NULL),secret(false)
{
	// Init Lua
	luaPState = LuaState::Create(true);

	vector<unsigned char> image;
	image.reserve(1024*1024*4);

	loadATexture("helpscreen.png", &helpImage, image);					uniqueTextures.push_back(helpImage.id);
	loadATexture("aboutscreen.png", &aboutImage, image);				uniqueTextures.push_back(aboutImage.id);
	loadATexture("titlescreen.png", &menuImage, image);					uniqueTextures.push_back(menuImage.id);
	loadATexture("wizard\\idle.png", &wizardImage, image);				uniqueTextures.push_back(wizardImage.id);

	loadATexture("health\\0.png", &healthBar[0], image); uniqueTextures.push_back(healthBar[0].id);
	loadATexture("health\\1.png", &healthBar[1], image); uniqueTextures.push_back(healthBar[1].id);
	loadATexture("health\\2.png", &healthBar[2], image); uniqueTextures.push_back(healthBar[2].id);
	loadATexture("health\\3.png", &healthBar[3], image); uniqueTextures.push_back(healthBar[3].id);
	loadATexture("health\\4.png", &healthBar[4], image); uniqueTextures.push_back(healthBar[4].id);
	loadATexture("health\\5.png", &healthBar[5], image); uniqueTextures.push_back(healthBar[5].id);
	loadATexture("health\\6.png", &healthBar[6], image); uniqueTextures.push_back(healthBar[6].id);
	loadATexture("health\\7.png", &healthBar[7], image); uniqueTextures.push_back(healthBar[7].id);
	loadATexture("health\\8.png", &healthBar[8], image); uniqueTextures.push_back(healthBar[8].id);
	loadATexture("health\\hp.png", &healthBarIndicator, image); uniqueTextures.push_back(healthBarIndicator.id);
	//Menu Music
	loadMp3File("title\\music.mp3", &menuMusic);
	loadMp3File("level3\\ending.mp3", &endMusic);
	endMusic.loop = true;
	loadMp3File("common\\debrisHit.mp3", &debrisHitSound);
	loadMp3File("common\\weirdMagic.mp3", &strangeSound);
	strangeSound.loop = false;
	debrisHitSound.loop = false;
	menuMusic.loop=true;
	currentMusic = &menuMusic;
	playMp3File(&menuMusic);

	luaPState->GetGlobals().SetNumber("GAME",GAME);
	luaPState->GetGlobals().SetNumber("GAME_WIN",GAME_WIN) ;
	luaPState->GetGlobals().SetNumber("GAME_INTRO",GAME_INTRO);
	luaPState->GetGlobals().SetNumber("MENU",MENU);
	luaPState->GetGlobals().SetNumber("MENU_HELP",MENU_HELP);
	luaPState->GetGlobals().SetNumber("MENU_ABOUT",MENU_ABOUT);
	luaPState->GetGlobals().SetNumber("EXIT",EXIT);
	luaPState->GetGlobals().RegisterDirect("createButton", *this, &LuaLevel::createButton);
	//menu lua
	if (luaPState->DoFile("Menu.lua"))
		std::cout << "An error occured: " << luaPState->StackTop().GetString() << std::endl;

	//cheats
	slowDown = false;
	invincibility = 0;
	uncollidable = false;

	setGameState(MENU, settings);
}

LuaLevel::~LuaLevel()
{
	// Deleting all of our textures in 1 fell swoop
	glDeleteTextures(uniqueTextures.size(), &uniqueTextures[0]);

	//need to reset luastepfunction to destroy luastate
	luaStepFunction.Reset();
	tile1ImageDrawList.Reset();
	tile2ImageDrawList.Reset();
	// Deleting our lua state/context,
	LuaState::Destroy(luaPState);
	if (m_world)
		delete m_world;

	if (currentMusic)
		Pa_CloseStream(currentMusic->pStream);
	terminateSound();
}

void LuaLevel::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
{
	if (invincibility)
		return;

	short collision = contact->GetFixtureA()->GetFilterData().categoryBits | contact->GetFixtureB()->GetFilterData().categoryBits;

	if (collision!=PLAYER_BODY_TOUCHING_DEBRIS && collision!=PLAYER_FEET_TOUCHING_DEBRIS)
		return;
	b2WorldManifold worldManifold;
	contact->GetWorldManifold(&worldManifold);
	b2Vec2 pos = playerBody->GetPosition();
	bool aboveCenterOfMass = true;
	for(int j = 0; j < contact->GetManifold()->pointCount; j++) {
		aboveCenterOfMass &= (worldManifold.points[j].y > pos.y);
	}
	if (!aboveCenterOfMass)
		return;

	// Should the player be killed?
	int32 count = contact->GetManifold()->pointCount;

	float32 maxImpulse = 0.0f;
	for (int32 i = 0; i < count; ++i)
	{
		maxImpulse = b2Max(maxImpulse, impulse->normalImpulses[i]);
	}

	if (maxImpulse > 22.0f)
	{
		playerBody->SetUserData((void*)true);
	}
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
	if (luaPState->DoFile(currentLevelLuaFile.c_str()))
		std::cout << "An error occured: " << luaPState->StackTop().GetString() << std::endl;


	if (luaPState->DoFile("Settings.lua"))
		std::cout << "An error occured: " << luaPState->StackTop().GetString() << std::endl;

	unloadLevelGlobals(luaPState);

	//~~~~~~PLAYER STUFF
	//~~~~~~~~~~~~~~~~~~~~Sprites
	vector<unsigned char> image;
	image.reserve(128*128*4);
	string character(luaPState->GetGlobal("character").GetString());

	vector<Graphics::Texture> textures(4);
	vector<int> framesPerImage(4);
	loadATexture(character + "\\idle\\1.png", &textures[0], image);  framesPerImage[0]=8;  levelTextures.push_back(textures[0].id);
	loadATexture(character + "\\idle\\2.png", &textures[1], image);  framesPerImage[1]=30; levelTextures.push_back(textures[1].id);
	textures[2]=textures[0];						         framesPerImage[2]=8;
	loadATexture(character + "\\idle\\3.png", &textures[3], image);  framesPerImage[3]=30; levelTextures.push_back(textures[3].id);
	animatedIdle = new Graphics::AnimatedTexture(textures,4,framesPerImage);

	textures.resize(8);
	framesPerImage.resize(8);
	loadATexture(character + "\\run\\1.png", &textures[0], image); framesPerImage[0]=5; levelTextures.push_back(textures[0].id);
	loadATexture(character + "\\run\\2.png", &textures[1], image); framesPerImage[1]=5; levelTextures.push_back(textures[1].id);
	loadATexture(character + "\\run\\3.png", &textures[2], image); framesPerImage[2]=5; levelTextures.push_back(textures[2].id);
	loadATexture(character + "\\run\\4.png", &textures[3], image); framesPerImage[3]=7; levelTextures.push_back(textures[3].id);
	loadATexture(character + "\\run\\5.png", &textures[4], image); framesPerImage[4]=5; levelTextures.push_back(textures[4].id);
	loadATexture(character + "\\run\\6.png", &textures[5], image); framesPerImage[5]=5; levelTextures.push_back(textures[5].id);
	loadATexture(character + "\\run\\7.png", &textures[6], image); framesPerImage[6]=5; levelTextures.push_back(textures[6].id);
	loadATexture(character + "\\run\\8.png", &textures[7], image); framesPerImage[7]=7; levelTextures.push_back(textures[7].id);
	animatedRun = new Graphics::AnimatedTexture(textures,8,framesPerImage);

	textures.resize(5);
	framesPerImage.resize(5);
	loadATexture(character + "\\jump\\1.png", &textures[0], image); framesPerImage[0]=10; levelTextures.push_back(textures[0].id);
	loadATexture(character + "\\jump\\2.png", &textures[1], image); framesPerImage[1]=10; levelTextures.push_back(textures[1].id);
	loadATexture(character + "\\jump\\3.png", &textures[2], image); framesPerImage[2]=10; levelTextures.push_back(textures[2].id);
	loadATexture(character + "\\jump\\4.png", &textures[3], image); framesPerImage[3]=10; levelTextures.push_back(textures[3].id);
	loadATexture(character + "\\jump\\5.png", &textures[4], image); framesPerImage[4]=10; levelTextures.push_back(textures[4].id);
	animatedJump = new Graphics::AnimatedTexture(textures,5,framesPerImage);

	
	textures.resize(2);
	framesPerImage.resize(2);
	loadATexture(character + "\\hurt\\1.png", &textures[0], image); framesPerImage[0]=10; levelTextures.push_back(textures[0].id);
	loadATexture(character + "\\hurt\\2.png", &textures[1], image); framesPerImage[1]=10; levelTextures.push_back(textures[1].id);
	animatedHurt = new Graphics::AnimatedTexture(textures,2,framesPerImage);

	
	textures.resize(2);
	framesPerImage.resize(2);
	loadATexture(character + "\\dead\\1.png", &textures[0], image); framesPerImage[0]=10; levelTextures.push_back(textures[0].id);
	textures[1]=textures[0];
	textures[1].id=0;
	animatedDead = new Graphics::AnimatedTexture(textures,2,framesPerImage);

	loadATexture("CGs\\final"+character+".png", &winImage, image);					
	//loadATexture("CGs\\openingcg"+character+".png", &introImage, image);					uniqueTextures.push_back(introImage.id);
	if (luaPState->GetGlobal("introImageFile").IsString())
	{
		loadATexture(luaPState->GetGlobal("introImageFile").GetString(), &introImage, image);
		levelTextures.push_back(introImage.id);
	}
	else introImage.id=0;
	loadMp3File(("common\\" + character + "Death.mp3").c_str(),&deathSound);
	deathSound.loop = false;
	currentAnimatedTexture = animatedIdle;

	//~~~~~~~~~~~~~~~~~Box2D Stuff
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set((float)luaPState->GetGlobal("playerPositionX").GetNumber(),2);
	bodyDef.fixedRotation=true;
	playerBody = m_world->CreateBody(&bodyDef);

	b2FixtureDef fixtureDef;
	b2PolygonShape polygonShape;
	fixtureDef.shape = &polygonShape;
	polygonShape.SetAsBox(.6f,1.28f);
	fixtureDef.friction=0;
	fixtureDef.filter.categoryBits=playerBodyBits;
	fixtureDef.density=1;
	playerBox = playerBody->CreateFixture(&fixtureDef);

	b2Vec2 center(0,-1.28f);
	polygonShape.SetAsBox(.6f,.2f, center, 0);
	fixtureDef.filter.categoryBits=playerFeetBits;
	fixtureDef.density=0;
	playerFeet = playerBody->CreateFixture(&fixtureDef);

	//~~~~~~~~~~~~~~~~~User Interface
	controlJump=false;
	controlLeft= false;
	controlRight=false;
	uncollidable = false;
	invincibility = false;

	died = 0;
	citamatic = 100;

	currentDialog = 1;
	health = secret?2:8;

	uint32 flags = 0;
	flags += b2Draw::e_shapeBit;
	flags += b2Draw::e_jointBit;
	m_debugDraw.SetFlags(flags);

	m_destructionListener.luaLevel = this;
	m_world->SetDestructionListener(&m_destructionListener);
	m_world->SetDebugDraw(&m_debugDraw);
	m_world->SetContactListener(this);
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
	}

	m_world->Step(timeStep, 8, 3);
}

void LuaLevel::processCollisionsForGame(Settings* settings)
{
	//Check for winnning
	if ((b2Vec2(wizardPositionX+2.f,wizardPositionY+1.5f)-playerBody->GetPosition()).Length()<2)
	{
		if (luaPState->GetGlobal("afterWin").IsInteger())
		{
			setGameState((GameState)luaPState->GetGlobal("afterWin").GetInteger(), settings);
		}
		else if (luaPState->GetGlobal("afterWin").IsString())
		{
			currentLevelLuaFile = luaPState->GetGlobal("afterWin").GetString();
			setGameState(GAME_INTRO, settings);
		}
		return;
	}
	// check for outofbounds
	b2Vec2 pos = playerBody->GetPosition();
	if (pos.x<0 || pos.y<0 || pos.x>viewportMaximumX)
	{
		playMp3File(&deathSound);
		died = 30;
		//setGameState(GAME_INTRO,settings);
		return;
	}

	isFeetTouchingBoundary = canJump = canKickOff = false;
	b2WorldManifold worldManifold;
	for (b2ContactEdge *ce = playerBody->GetContactList() ; ce ; ce = ce->next)
	{
		b2Contact *c = ce->contact;
		if (!c->IsTouching())
			continue;

		short collision = c->GetFixtureA()->GetFilterData().categoryBits | c->GetFixtureB()->GetFilterData().categoryBits;

		/*
		// ~~~~~~~~~~~~~~ YOU LOOSE EFFECT && slowdown effect
		if (invincibility<=0)
		{
		if (collision == PLAYER_BODY_TOUCHING_DEBRIS)
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
		if (debrisSpeed.Length()>15 && debrisBody->GetPosition().y>playerBody->GetPosition().y+1.f)
		{
		setGameState(GAME_INTRO,settings);
		playMp3File(&deathSound);
		return;
		}
		}
		}
		}else invincibility-=.2f;
		if (invincibility<0)
		invincibility = 0;
		*/
		if (PLAYER_FEET_TOUCHING_BOUNDARY == collision || PLAYER_FEET_TOUCHING_DEBRIS == collision) 
		{
			isFeetTouchingBoundary = true;
			c->GetWorldManifold(&worldManifold);
			b2Vec2 pos = playerBody->GetPosition();
			bool below = true, side = true;
			for(int j = 0; j < c->GetManifold()->pointCount; j++) {
				below &= (worldManifold.points[j].y < pos.y - 1.28f);
				side &= (worldManifold.points[j].x < pos.x - .6f) | (worldManifold.points[j].x > pos.x + .6f);
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
		if (canJump && justJumped==false && linearVelocity.y<10)
		{
			playerBody->ApplyLinearImpulse(b2Vec2(0,25), worldCenter);
			playerCanMoveUpwards = .6f;
			justJumped = true;
			if (currentAnimatedTexture!=animatedJump)
				currentAnimatedTexture=animatedJump;
		}
		else if (canKickOff && justKickedOff==false)
		{
			playerBody->ApplyLinearImpulse(b2Vec2(0,12), worldCenter);
			justKickedOff = true;
		}/*
		else if (playerCanMoveUpwards > 0)
		{
			m_debugDraw.DrawString(0, 45, "floating");
			playerBody->ApplyLinearImpulse(b2Vec2(0,2.f), worldCenter);
		}*/
	}

	// HORIZONTAL MOVEMENT/RUNNING
	float32 vx = 0;
	if (controlLeft)
		vx += -400;
	if (controlRight)
		vx += 400;

	if (vx == 0) { // if not pressing left and right
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
		if (vx > 0 && linearVelocity.x < 10) {
			playerBody->ApplyForce(force, worldCenter);
			isFacingRight = true;
			//moving right
		} else if (vx < 0 && linearVelocity.x > -10) {
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

	if (citamatic==0)
	{
		b2Vec2 viewportPosition = settings->getViewPosition();
		viewportPosition.y = max(viewportPosition.y-(viewportPosition.y-(worldCenter.y-10)) * .2f,0.0f);
		viewportPosition.x = max(0.0f,viewportPosition.x-(viewportPosition.x-(worldCenter.x-10)) * .2f);
		viewportPosition.x = min(viewportMaximumX-30.f,viewportPosition.x);//30.f is viewport width
		settings->setViewPosition(viewportPosition);
	}
}


b2Vec2 mouse;
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
	case GAME_WIN:
		glColor4ub(255, 255, 255, 255);
		drawImage(&winImage);
		m_debugDraw.DrawString(0,1000,"Congratulations! Press the Space Bar in the main menu for a suprise!");


		if (!currentVoice)
		{
			if (luaPState->GetGlobal("endDialogFile").IsTable())
			{
				LuaObject table = luaPState->GetGlobal("endDialogFile");
				int size = table.GetCount();
				if (currentDialog<=size)
				{
					currentVoice = new Sound;
					loadMp3File(table.GetByIndex(currentDialog).GetString(),currentVoice);
					currentVoice->loop=false;
					playMp3File(currentVoice);
				}
			}
		}
		else if (currentVoice->pStream==0)
		{
			LuaObject table = luaPState->GetGlobal("endDialogFile");
			int size = table.GetCount();
			delete currentVoice;
			currentVoice = NULL;
			currentDialog++;
		}
		break;
	case GAME_INTRO:
		if (isValidTexture(introImage))
		{
			glColor4ub(255, 255, 255, 255);
			unsigned int vpW = settings->getVPW();
			drawIntroImage(&introImage, vpW);
		}

		if (!currentVoice)
		{
			if (luaPState->GetGlobal("dialogFile").IsTable())
			{
				LuaObject table = luaPState->GetGlobal("dialogFile");
				int size = table.GetCount();
				if (currentDialog<=size)
				{
					currentVoice = new Sound;
					loadMp3File(table.GetByIndex(currentDialog).GetString(),currentVoice);
					currentVoice->loop=false;
					playMp3File(currentVoice);
				}
			}
		}
		else if (currentVoice->pStream==0)
		{
			LuaObject table = luaPState->GetGlobal("dialogFile");
			int size = table.GetCount();
			delete currentVoice;
			currentVoice = NULL;
			currentDialog++;
		}
		break;
	case GAME:		
		if (died==1)
		{
			setGameState(GAME_INTRO, settings);
		}
		else
		{
			float32 timeStep = settings->getHz() > 0.0f ? 1.0f / settings->getHz() : float32(0.0f);

			processCollisionsForGame(settings);
			if (died==0 && citamatic==0) // check to see if this is still the game state or citamitc
				processInputForGame(settings, timeStep);
			if (slowDown)
				timeStep/=slowdownBy;

			if (citamatic<10 && citamatic!=0)
			{
				b2Vec2 playerPosition = playerBody->GetPosition();
				b2Vec2 originalViewport = settings->getViewPosition();
				b2Vec2 viewportPosition = settings->getViewPosition();
				viewportPosition.y = max(viewportPosition.y-(viewportPosition.y-(playerPosition.y-10)) * .02f,0.0f);
				viewportPosition.x = max(0.0f,viewportPosition.x-(viewportPosition.x-(playerPosition.x-15)) * .02f);
				viewportPosition.x = min(viewportMaximumX-30.f,viewportPosition.x);//30.f is viewport width
				settings->setViewPosition(viewportPosition);
				if ((viewportPosition-originalViewport).Length()<.01f)
					citamatic--;
			} else if (citamatic!=0) citamatic--;

			if (invincibility)
			{
				invincibility-=.2f;
				if (invincibility<0)
					invincibility = 0;
			}


			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glColor4f(1,1,1,1);
			// ~~~~~~~~~~~~~ background drawing
			if (Graphics::isValidTexture(backgroundImage))
				Graphics::drawImage(&backgroundImage);

			if (citamatic==0)
				// ~~~~~~~~~~~~~ box2d drawing
					drawGame(settings, timeStep);

			if (playerBody->GetUserData())
			{
				playMp3File(&debrisHitSound);
				playMp3File(&deathSound);
				//died = true;
				if (--health==0)
					died=200;
				else
					invincibility=20;
				playerBody->SetUserData(NULL);
			}


			// ~~~~~~~~~~~~~ tile drawing
			if (Graphics::isValidTexture(tile1Image))
			{
				if (tile1ImageDrawList.IsTable())
				{
					int drawListLength = tile1ImageDrawList.GetN();
					for (int i = 1; i <= drawListLength-3; i+=4)
					{
						Graphics::drawImage(&tile1Image, 
							(int)tile1ImageDrawList.GetByIndex(i).GetInteger(),(int)tile1ImageDrawList.GetByIndex(i+1).GetInteger(),
							(int)tile1ImageDrawList.GetByIndex(i+2).GetInteger(),(int)tile1ImageDrawList.GetByIndex(i+3).GetInteger());
					}
				}
			}
			if (Graphics::isValidTexture(tile2Image))
			{
				if (tile2ImageDrawList.IsTable())
				{
					int drawListLength = tile2ImageDrawList.GetN();
					for (int i = 1; i <= drawListLength-3; i+=4)
					{
						Graphics::drawImage(&tile2Image, 
							(int)tile2ImageDrawList.GetByIndex(i).GetInteger(),(int)tile2ImageDrawList.GetByIndex(i+1).GetInteger(),
							(int)tile2ImageDrawList.GetByIndex(i+2).GetInteger(),(int)tile2ImageDrawList.GetByIndex(i+3).GetInteger());
					}
				}
			}
			
			if (invincibility)
				currentAnimatedTexture = animatedHurt;
			if (died>0)
				currentAnimatedTexture = animatedDead;
			// ~~~~~~~~~~~~~ player drawing
			if (!invincibility || (invincibility && invincibilityEffectShow))
			{
				b2Vec2 worldCenter = playerBody->GetWorldCenter();
				glPushMatrix();
				if (uncollidable)
					glColor4f(1,1,1,.5f);
				glTranslatef(worldCenter.x,worldCenter.y-1.28f-.2f*2,0);
				const float32 scale = .028f;
				glScalef(scale*(isFacingRight?1:-1),scale,scale);
				Graphics::Texture currentTexture = currentAnimatedTexture->updateAndGetTexture();
				glTranslatef(currentTexture.imageWidth/(-2.0f),0,0);
				if (currentTexture.id!=0)
					drawImage(&currentTexture);
				glPopMatrix();
			}


			if (invincibility)
			{
				invincibilityEffectTimer-=.2f;
				if (invincibilityEffectTimer<=0)
				{
					invincibilityEffectShow = !invincibilityEffectShow;
					invincibilityEffectTimer = 2;
				}
			}

			//wizard drawing
			glPushMatrix();
			if (uncollidable)
				glColor4f(1,1,1,.5f);
			glTranslatef((float)wizardPositionX,(float)wizardPositionY,0);
			const float32 scale = .025f;
			glScalef(scale*(wizardIsFacingRight?1:-1),scale,scale);
			Graphics::drawImage(&wizardImage);
			glPopMatrix();

			// ~~~~~~~~~~~~~ debris drawing
			for (vector<int>::size_type i = 0; i < debris.size(); i++)
			{
				Graphics::Texture* texture = (Graphics::Texture*)debris[i]->GetUserData();
				//b2Vec2 halfSize(texture->imageWidth/2.f,texture->imageHeight/2.f);

				b2Vec2 pos;
				pos = debris[i]->GetWorldCenter();
				float size = 1/128.f * (int)(debris[i]->GetFixtureList()->GetUserData())/100.f;
				b2Vec2 bottomLeftCorner = pos;//-halfSize;

				glPushMatrix();
				glTranslatef(pos.x,pos.y,0);
				glRotatef(debris[i]->GetAngle()*180/3.14f,0,0,1);
				glTranslatef(texture->imageWidth*-size,texture->imageHeight*-size,0);
				//glTranslatef(pos.x,pos.y,0);
				//glTranslatef(pos.x-texture->imageWidth,pos.y-texture->imageHeight,0);
				glScalef(size*2,size*2,size*2);
				Graphics::drawImage(texture,0,0,texture->imageWidth,texture->imageHeight);
				glPopMatrix();
			}

			// ~~~~~~~~~~~~ health bar drawing
			if (citamatic==0)
			{
				b2Vec2 viewportPosition = settings->getViewPosition();
				glPushMatrix();
				glTranslatef(viewportPosition.x,settings->getTop(),0);
				Graphics::drawImage(&healthBarIndicator,1,-3,2,2);
				Graphics::drawImage(&healthBar[health],3,-3,16,2);
				glPopMatrix();
			}

#ifdef _DEBUG
			//glColor4f(1,1,1,1);
			glDisable(GL_TEXTURE_2D);
			m_world->DrawDebugData();
#endif
			if (died>0)
				died--;
		}
		break;
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for (unsigned int i = 0; i<buttons.size();i++)
	{
		if (find(buttons[i].statesToShow.begin(),buttons[i].statesToShow.end(),gameState)!=buttons[i].statesToShow.end())
			if (mouse.x>buttons[i].x && mouse.x<buttons[i].x+buttons[i].standard.imageWidth &&
				mouse.y>buttons[i].y && mouse.y<buttons[i].y+buttons[i].standard.imageHeight)
				Graphics::drawImage(&buttons[i].hovering,buttons[i].x,buttons[i].y,buttons[i].hovering.imageWidth,buttons[i].hovering.imageHeight);
			else	
				Graphics::drawImage(&buttons[i].standard,buttons[i].x,buttons[i].y,buttons[i].hovering.imageWidth,buttons[i].hovering.imageHeight);
	}

#ifdef _DEBUG 
	if (settings->getPause())
	{

		m_debugDraw.DrawString(200,200,"%d",glutGet(GLUT_WINDOW_HEIGHT));
		for (b2Body *body = m_world->GetBodyList() ; body ; body = body->GetNext())
		{
			b2Vec2 pos = body->GetPosition();
			b2Vec2 vel = body->GetLinearVelocity();
			m_debugDraw.DrawString(int(pos.x*(640/30)), 480-int(pos.y*(480/25)), "%.2f, %.2f", vel.x, vel.y);

		}
	}
#endif // _DEBUG 
}
void LuaLevel::setGameState(GameState state, Settings* settings)
{
	if (gameState==GAME && state!=GAME)
	{
		delete m_world;
		glDeleteTextures(levelTextures.size(),&levelTextures[0]);
		vector<GLuint>().swap(levelTextures);
		vector<Graphics::Texture>().swap(debrisList);
		vector<b2Body*>().swap(debris);
		introImage.id = 0;
		backgroundImage.id  = 0;
		tile1Image.id = 0;
		tile2Image.id = 0;
		if (currentMusic==&gameMusic)
		{
			Pa_CloseStream(currentMusic->pStream);
			currentMusic = NULL;
		}
		delete animatedJump;
		delete animatedIdle;
		delete animatedRun;
		delete animatedHurt;
		delete animatedDead;
		animatedJump=animatedIdle=animatedRun=NULL;
		luaStepFunction.Reset();
		vector<unsigned char>().swap(gameMusic.loaded);
		vector<unsigned char>().swap(introMusic.loaded);

		m_world = NULL;
		if (state!=GAME_WIN)
		{
			if (winImage.id)
			{
				glDeleteTextures(1,&winImage.id);
				winImage.id=0;
			}
		}
	}
	if (gameState==GAME_INTRO && state!=GAME_INTRO)
	{
		if (currentVoice && currentVoice->pStream)
		{
			Pa_CloseStream(currentVoice->pStream);
			delete currentVoice;
			currentVoice = NULL;
		}
	}

	if (gameState==GAME_WIN && state!=GAME_WIN)
	{
		if (currentVoice && currentVoice->pStream)
		{
			Pa_CloseStream(currentVoice->pStream);
			delete currentVoice;
			currentVoice = NULL;
		}
		if (winImage.id)
		{
			glDeleteTextures(1,&winImage.id);
			winImage.id=0;
		}
	}
	gameState = state;
	if (gameState==GAME)
	{
		settings->setViewSize(30);
		settings->widthIsConstant = true;
		b2Vec2 pos(min(max(0.f,float(wizardPositionX)-15),viewportMaximumX-30),max(0.f,float(wizardPositionY)-8));
		settings->setViewPosition(pos);
		glClearColor(201/255.f,229/255.f,245/255.f,1);
		if (currentMusic != &gameMusic)
		{
			if (currentMusic)
				Pa_CloseStream(currentMusic->pStream);
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
		if (gameState==GAME_INTRO)
		{
			init();
		}

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
					Pa_CloseStream(currentMusic->pStream);

				menuMusic.pos = 0;
				currentMusic = &menuMusic;
				playMp3File(&menuMusic);
			}
		}
		if (state==GAME_WIN)
		{
			if (currentMusic)
				Pa_CloseStream(currentMusic->pStream);

			endMusic.pos = 0;
			currentMusic = &endMusic;
			playMp3File(&endMusic);
			currentDialog=1;
		}else if (state==GAME_INTRO)
		{
			if (currentMusic)
				Pa_CloseStream(currentMusic->pStream);

			if (introMusic.loaded.size()!=0)
			{
				introMusic.pos = 0;
				currentMusic = &introMusic;
				playMp3File(&introMusic);
			}
		}

		if (state==MENU)
		{
			currentLevelLuaFile = "TrainingLevel.lua";
			vector<unsigned char>().swap(deathSound.loaded);
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~	INPUT HANDLING	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void LuaLevel::Keyboard(unsigned char key, Settings* settings)
{
	if (gameState==GAME_INTRO)
	{
		if (key=='\r' || key=='\n')
			setGameState(GAME, settings);
	}
	else if (gameState==GAME_WIN && (key==27 || key=='\r' || key=='\n'))
		setGameState(MENU, settings);
	else if (key==' ' && gameState==MENU)
	{
		if (secret=!secret)
		{
			playMp3File(&strangeSound);
		}
	}
	else
		switch (key)
	{
		case 27: //ESCAPE KEY
			if (gameState==MENU)
				glutLeaveMainLoop();
			setGameState(MENU, settings);
			break;
		case 'r':
			playMp3File(&deathSound);
			setGameState(GAME_INTRO,settings);
			break;
		default:
			if (key==controlKeyLeft)
				controlLeft = true;
			else if (key==controlKeyRight)
				controlRight = true;
			else if (key==controlKeyJump)
				controlJump= true;
			else if (key==controlKeySlowDown)
				slowDown = !slowDown;
			else if (key==controlKeyInvincibility)
				if (invincibility>0) 
					invincibility = 0;
				else
					invincibility = numeric_limits<float>::infinity();
			else if (key==controlKeyUncollidable)
			{
				b2Filter feetFilter(playerFeet->GetFilterData()), bodyFilter(playerBox->GetFilterData());
				if (uncollidable = !uncollidable)
				{
					//uncollidable
					feetFilter.maskBits = 0;
					bodyFilter.maskBits = 0;
				}
				else
				{
					//collidable
					feetFilter.maskBits = -1;
					bodyFilter.maskBits = -1;
				}
				playerFeet->SetFilterData(feetFilter);
				playerBox->SetFilterData(bodyFilter);
			}
			else if (key==controlKeyThrusters)
			{
				b2Vec2 worldCenter = playerBody->GetWorldCenter();
				playerBody->ApplyLinearImpulse(b2Vec2(0,50), worldCenter);
			}
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
		if (find(buttons[i].statesToShow.begin(),buttons[i].statesToShow.end(),gameState)!=buttons[i].statesToShow.end() &&
			mouse.x>buttons[i].x && mouse.x<buttons[i].x+buttons[i].standard.imageWidth &&
			mouse.y>buttons[i].y && mouse.y<buttons[i].y+buttons[i].standard.imageHeight)
		{
			if (buttons[i].state!=EXIT)
				setGameState((GameState)buttons[i].state, settings);
			else glutLeaveMainLoop();
		}
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


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Lua Specific methods    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


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

void LuaLevel::loadLevelGlobals(LuaState *pstate)
{
	//Box2DFactory/Level Loading~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	LuaObject globals = pstate->GetGlobals();

	LuaObject metaTableObj = globals.CreateTable("box2DFactoryMetaTable");
	metaTableObj.SetObject("__index", metaTableObj);
	metaTableObj.RegisterObjectDirect("createEdge", (LuaLevel *)nullptr, &LuaLevel::createAnEdge);
	metaTableObj.RegisterObjectDirect("createFrictionlessEdge", (LuaLevel *)nullptr, &LuaLevel::createFrictionlessEdge);
	metaTableObj.RegisterObjectDirect("createBox", (LuaLevel *)nullptr, &LuaLevel::createBox);
	metaTableObj.RegisterObjectDirect("createDebris", (LuaLevel *)nullptr, &LuaLevel::createDebris);

	LuaObject box2DFactoryObject = pstate->BoxPointer(this);
	box2DFactoryObject.SetMetaTable(metaTableObj);
	pstate->GetGlobals().SetObject("box2DFactory", box2DFactoryObject);

	// Controls ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	globals.SetString("controlKeyJump"," ", 1);
	globals.SetString("controlKeyRight","d",1);
	globals.SetString("controlKeyLeft","a",1);
	globals.SetString("controlSlowDown","\0",1);
	globals.SetString("controlKeyInvincibility","\0",1);
	globals.SetString("controlKeyUncollidable","\0",1);
	globals.SetString("controlKeyThrusters","\0",1);


	globals.SetString("character",secret?"Angela":"Alex");
	globals.SetNumber("viewportMaximumX",30);
	globals.SetNumber("playerPositionX",2);
	globals.SetNumber("wizardPositionX",-50);
	globals.SetNumber("wizardPositionY",-50);
	globals.SetNumber("winHeight",60);



	//Level specific stuff
	globals.SetNil("music");
	globals.SetNil("musicLoop");

	globals.SetNil("tile1ImageFile");
	globals.SetNil("tile1ImageWidth");
	globals.SetNil("tile1ImageHeight");
	globals.SetNil("tile1ImageDrawList");

	globals.SetNil("tile2ImageFile");
	globals.SetNil("tile2ImageWidth");
	globals.SetNil("tile2ImageHeight");
	globals.SetNil("tile2ImageDrawList");

	globals.SetNil("backgroundImageFile");
	globals.SetNil("step");

	globals.SetNil("afterWin");
	globals.SetNil("debrisList");
}

void LuaLevel::unloadLevelGlobals(LuaState *pstate)
{
	//controls
	checkAndSetChar(controlKeyLeft, pstate->GetGlobal("controlKeyLeft"));
	checkAndSetChar(controlKeyRight, pstate->GetGlobal("controlKeyRight"));
	checkAndSetChar(controlKeyJump, pstate->GetGlobal("controlKeyJump"));
	checkAndSetChar(controlKeySlowDown, pstate->GetGlobal("controlKeySlowDown"));
	checkAndSetChar(controlKeyInvincibility, pstate->GetGlobal("controlKeyInvincibility"));
	checkAndSetChar(controlKeyUncollidable, pstate->GetGlobal("controlKeyUncollidable"));
	checkAndSetChar(controlKeyThrusters, pstate->GetGlobal("controlKeyThrusters"));

	//level specific stuffs
	//music
	if (pstate->GetGlobal("music").IsString())
	{
		loadMp3File(pstate->GetGlobal("music").GetString(),&gameMusic);
	}
	if (pstate->GetGlobal("introMusic").IsString())
	{
		loadMp3File(pstate->GetGlobal("introMusic").GetString(),&introMusic);
	}
	if (pstate->GetGlobal("musicLoop").IsBoolean())
	{
		gameMusic.loop = pstate->GetGlobal("musicLoop").GetBoolean();
	}
	if (pstate->GetGlobal("viewportMaximumX").IsNumber())
	{
		viewportMaximumX = (float)pstate->GetGlobal("viewportMaximumX").GetNumber();
	}
	if (pstate->GetGlobal("winHeight").IsNumber())
	{
		winHeight = (float)pstate->GetGlobal("winHeight").GetNumber();
	}

	if (pstate->GetGlobal("wizardPositionX").IsNumber())
	{
		wizardPositionX = (float)pstate->GetGlobal("wizardPositionX").GetNumber();
	}
	if (pstate->GetGlobal("wizardPositionY").IsNumber())
	{
		wizardPositionY = (float)pstate->GetGlobal("wizardPositionY").GetNumber();
	}
	//Tiles
	if (pstate->GetGlobal("tile1ImageFile").IsString())
	{
		Graphics::loadATexture(pstate->GetGlobal("tile1ImageFile").GetString(),&tile1Image);
		levelTextures.push_back(tile1Image.id);
		if (pstate->GetGlobal("tile1ImageWidth").IsNumber())
			tile1Image.imageWidth = (unsigned int)pstate->GetGlobal("tile1ImageWidth").GetNumber();
		if (pstate->GetGlobal("tile1ImageHeight").IsNumber())
			tile1Image.imageHeight = (unsigned int)pstate->GetGlobal("tile1ImageHeight").GetNumber();

		if (pstate  ->GetGlobal("tile1ImageDrawList").IsTable())
		{
			tile1ImageDrawList = pstate->GetGlobal("tile1ImageDrawList");
			if (tile1ImageDrawList.GetN()%4)
				cout<<"Drawing list is not a mutliple of 4"<<endl;
		}
	}
	if (pstate->GetGlobal("tile2ImageFile").IsString())
	{
		Graphics::loadATexture(pstate->GetGlobal("tile2ImageFile").GetString(),&tile2Image);
		levelTextures.push_back(tile1Image.id);
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
		levelTextures.push_back(backgroundImage.id);
		if (pstate->GetGlobal("backgroundImageWidth").IsNumber())
			backgroundImage.imageWidth = (unsigned int)pstate->GetGlobal("backgroundImageWidth").GetNumber();
		if (pstate->GetGlobal("backgroundImageHeight").IsNumber())
			backgroundImage.imageHeight = (unsigned int)pstate->GetGlobal("backgroundImageHeight").GetNumber();
	}

	if (pstate->GetGlobal("debrisList").IsTable())
	{
		LuaObject obj = pstate->GetGlobal("debrisList");
		int size = obj.GetN();
		for (int i = 1; i<=size; i++)
		{
			Graphics::Texture debris;
			Graphics::loadATexture(obj.GetByIndex(i).GetString(), &debris);
			levelTextures.push_back(debris.id);
			debrisList.push_back(debris);
		}
	}


	//functions
	if (pstate->GetGlobal("step").IsFunction())
	{
		luaStepFunction = pstate->GetGlobal("step");
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Lua hooked methods    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int LuaLevel::createAnEdge( float32 x1, float32 y1, float32 x2, float32 y2 )
{
	b2FixtureDef fixtureDef;
	b2EdgeShape edgeShape;


	fixtureDef.filter.categoryBits=boundaryBits;
	fixtureDef.shape = &edgeShape;

	edgeShape.Set(b2Vec2(x1,y1),b2Vec2(x2,y2));

	m_groundBody->CreateFixture(&fixtureDef);
	return 0;
}

int LuaLevel::createFrictionlessEdge(float32 x1, float32 y1, float32 x2, float32 y2)
{
	b2FixtureDef fixtureDef;
	b2EdgeShape edgeShape;

	fixtureDef.friction = 0;
	fixtureDef.filter.categoryBits=boundaryBits;
	fixtureDef.shape = &edgeShape;

	edgeShape.Set(b2Vec2(x1,y1),b2Vec2(x2,y2));

	m_groundBody->CreateFixture(&fixtureDef);
	return 0;
}

int LuaLevel::createBox( float32 x, float32 y, float32 w, float32 h)
{
	b2FixtureDef fixtureDef;
	b2PolygonShape polygonShape;


	fixtureDef.filter.categoryBits=boundaryBits;
	fixtureDef.shape = &polygonShape;

	polygonShape.SetAsBox(w/2,h/2,b2Vec2(x+w/2,y+h/2),0);

	m_groundBody->CreateFixture(&fixtureDef);
	return 0;
}

int LuaLevel::createDebris( float32 x, float32 y)
{
	b2BodyDef bodyDef;
	b2FixtureDef fixtureDef;
	b2PolygonShape polygonShape;


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

	int index = rand()%debrisList.size();
	Graphics::Texture debrisTexture = debrisList[index];
	r = ((float) rand() / (RAND_MAX))*.5f+.8f;
	polygonShape.SetAsBox(debrisTexture.imageWidth/128.f*r,debrisTexture.imageHeight/128.f*r);
	body->SetUserData(&debrisList[index]);
	debris.push_back(body);
	fixtureDef.userData = (void*)((int)(r*100));
	body->CreateFixture(&fixtureDef);


	return 0;
}

int LuaLevel::createButton(float x, float y, const char* file1, const char* file2, int state, LuaStackObject statesToShow)
{
	Graphics::Texture hovering,standard;
	Graphics::loadATexture(file1, &standard);
	Graphics::loadATexture(file2, &hovering);
	uniqueTextures.push_back(hovering.id);
	uniqueTextures.push_back(standard.id);
	vector<GameState> statesToShowVector;
	if (statesToShow.IsTable())
	{
		int size = statesToShow.GetCount();
		for (int i = 1; i <= size; i++)
			statesToShowVector.push_back((GameState)statesToShow.GetByIndex(i).GetInteger());
	}
	buttons.push_back(Button(x,y,hovering,standard, state, statesToShowVector));
	return 0;
}