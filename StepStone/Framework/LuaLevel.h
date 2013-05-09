
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
	GAME_INTRO
};

struct Button
{
	Button(float tx, float ty, Graphics::Texture aHovering, Graphics::Texture aStandard, int aState):
		x(tx),y(ty),
		hovering(aHovering),
		standard(aStandard),
	state(aState){}
	int state; // should really be GameState
	Graphics::Texture hovering, standard;
	float x,y;
};

class LuaLevel
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
	int createBox( float32 x, float32 y, float32 hw, float32 hh);
	int createDebris(float32 x, float32 y);
	void init();
	int createButton(float x, float y, const char* file1,const char* file2, int state);
protected:
	vector<Button> buttons;
	inline void processCollisionsForGame(Settings* settings);
	inline void processInputForGame(Settings *settings, float32 timeStep);

	GLuint bindTexture(string file);

	friend class LuaLevelDestructionListener;
	//friend class BoundaryListener;
	//friend class ContactListener;
	
	b2AABB aabb;
	LuaState* luaPState;
	LuaObject luaStepFunction;
	b2BodyDef bodyDef;
	LuaObject bodyDefObj;
	int BodyDefIndex(LuaState* pState);
	int BodyDefNewIndex(LuaState* pState);
	b2FixtureDef fixtureDef;
	LuaObject fixtureDefObj;
	int FixtureDefIndex(LuaState* pState);
	int FixtureDefNewIndex(LuaState* pState);
	b2PolygonShape polygonShape;
	LuaObject polygonShapeObj;
	int PolygonShapeIndex(LuaState* pState);
	int PolygonShapeNewIndex(LuaState* pState);
	b2EdgeShape edgeShape;
	LuaObject edgeShapeObj;
	int edgeShapeIndex(LuaState* pState);
	int edgeShapeNewIndex(LuaState* pState);
	b2CircleShape circleShape;
	LuaObject circleShapeObj;
	int circleShapeIndex(LuaState* pState);
	int circleShapeNewIndex(LuaState* pState);

	b2Body* m_groundBody;
	b2Body* playerBody;
	b2Fixture* playerFeet;
	b2Fixture* playerBox;
	b2Fixture* playerShield;
	bool slowDown;

	b2World* m_world;
	LuaLevelDestructionListener m_destructionListener;
	DebugDraw m_debugDraw;
	GameState gameState;

	//UI
	bool controlLeft,
		controlRight,
		controlJump;

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

	Graphics::AnimatedTexture* animatedIdle;
	Graphics::AnimatedTexture* animatedRun;
	Graphics::AnimatedTexture* animatedJump;

	Graphics::AnimatedTexture* currentAnimatedTexture;
	bool isFacingRight;
	
	Sound menuMusic;
	Sound gameMusic;
	Sound* currentMusic;
};

#endif
