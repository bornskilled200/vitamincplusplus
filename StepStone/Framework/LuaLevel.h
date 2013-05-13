
#ifndef LUALEVEL_H
#define LUALEVEL_H

#include <Box2D/Box2D.h>
#include "Render.h"
#include "LuaPlusFramework\LuaPlus.h"
#include <vector>
#include "Graphics.h"
#include "Main.h"
#include "Sound.h"
using namespace LuaPlus;

#include <cstdlib>
using namespace std;

class LuaLevel;

// This is called when a joint in the world is implicitly destroyed
// because an attached body is destroyed. This gives us a chance to
// nullify the mouse joint.
class LuaLevelDestructionListener : public b2DestructionListener
{
public:
	void SayGoodbye(b2Fixture* fixture) { B2_NOT_USED(fixture); }
	void SayGoodbye(b2Joint* joint);

	LuaLevel* luaLevel;
};

enum GameState
{
	SPLASH,
	MENU,
	MENU_ABOUT,
	MENU_HELP,
	GAME,
	GAME_WIN,
	GAME_INTRO,
	EXIT // not really a state
};

struct Button
{
	Button(float tx, float ty, Graphics::Texture aHovering, Graphics::Texture aStandard, int aState, vector<GameState> asts):
		x(tx),y(ty),
		hovering(aHovering),
		standard(aStandard),
	state(aState), statesToShow(asts){}
	int state; // should really be GameState
	Graphics::Texture hovering, standard;
	float x,y;
	vector<GameState> statesToShow;
};

class LuaLevel : b2ContactListener
{
public:

	LuaLevel(Settings* settings);
	~LuaLevel();

	void drawGame(Settings* settings, float32 timeStep);
	void Step(Settings* settings);
	void Keyboard(unsigned char key, Settings* settings);
	void KeyboardUp(unsigned char key);
	void ShiftMouseDown(const b2Vec2& p);
	void MouseDown(const b2Vec2& p,  Settings *settings);
	void MouseUp(const b2Vec2& p);
	void MouseMove(const b2Vec2& p);
	// Let derived tests know that a joint was destroyed.
	void JointDestroyed(b2Joint* joint) { B2_NOT_USED(joint); }

	void loadLevelGlobals(LuaState *pstate);
	void unloadLevelGlobals(LuaState *pstate);

	void setGameState(GameState state, Settings* settings);
	
	// Lua hooked methods
	int createAnEdge(float32 x1, float32 y1, float32 x2, float32 y2);
	int createFrictionlessEdge(float32 x1, float32 y1, float32 x2, float32 y2);
	int createBox( float32 x, float32 y, float32 hw, float32 hh);
	int createDebris( float32 x, float32 y,  float32 w, float32 h);
	void init();
	int createButton(float x, float y, const char* file1,const char* file2, int state, LuaStackObject statesToShow);

	virtual void BeginContact(b2Contact* contact) { B2_NOT_USED(contact); }
	virtual void EndContact(b2Contact* contact) { B2_NOT_USED(contact); }
	virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) 
	{
		B2_NOT_USED(contact); 
		B2_NOT_USED(oldManifold);
	}
	virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse);
protected:
	vector<Button> buttons;

	//helper methods to break up code
	inline void processCollisionsForGame(Settings* settings);
	inline void processInputForGame(Settings *settings, float32 timeStep);
	
	LuaState* luaPState;
	LuaObject luaStepFunction;
	string currentLevelLuaFile;

	b2Body* m_groundBody;

	b2Body* playerBody;
	b2Fixture* playerFeet;
	b2Fixture* playerBox;

	b2World* m_world;
	LuaLevelDestructionListener m_destructionListener;
	DebugDraw m_debugDraw;
	GameState gameState;

	//UI
	bool controlLeft,
		controlRight,
		controlJump;
	//cheats
	bool slowDown;
	float invincibility;
	float invincibilityEffectTimer;
	float invincibilityEffectShow;
	bool uncollidable;
	
	char controlKeyLeft, controlKeyRight, controlKeyJump, controlKeySlowDown, controlKeyUncollidable, controlKeyInvincibility,controlKeyThrusters;
    float slowdownBy;

	//Game Variables
	bool isFeetTouchingBoundary, canJump, justJumped, justKickedOff, wasMoving, canKickOff;
	float32 playerCanMoveUpwards;

	//Texutres
	Graphics::Texture aboutImage;
	Graphics::Texture menuImage;
	Graphics::Texture helpImage;
	Graphics::Texture backdropImage;
	Graphics::Texture winImage;
	Graphics::Texture introImage;
	
	Graphics::Texture tile1Image;
	LuaObject tile1ImageDrawList;
	Graphics::Texture tile2Image;
	LuaObject tile2ImageDrawList;
	Graphics::Texture backgroundImage;
	
	vector<GLuint> uniqueTextures;
	vector<GLuint> levelTextures;

	Graphics::AnimatedTexture* animatedIdle;
	Graphics::AnimatedTexture* animatedRun;
	Graphics::AnimatedTexture* animatedJump;

	Graphics::AnimatedTexture* currentAnimatedTexture;
	bool isFacingRight;
	float viewportMaximumX;
	
	Sound menuMusic;

	Sound gameMusic;
	Sound deathSound;
	Sound* currentVoice;
	int currentDialog;
	
	Sound* currentMusic;
};

#endif
