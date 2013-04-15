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
#include <string>
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

LuaLevel::LuaLevel()
{
	
	// Init Lua
	luaPState = LuaState::Create();

	b2Vec2 gravity;
	gravity.Set(0.0f, -10.0f);
	m_world = new b2World(gravity);
	b2BodyDef bodyDef;
	bodyDef.type = b2_staticBody;
	m_groundBody = m_world->CreateBody(&bodyDef);
	//m_world->IsLocked();

	loadLevelGlobals(luaPState);
	luaPState->GetGlobals().Register("Print",Print);

	// Open the Lua Script File
	if (luaPState->DoFile("TrainingLevel.lua"))
		if( luaPState->GetTop() == 1 )
			std::cout << "An error occured: " << luaPState->CheckString(1) << std::endl;
	LuaObject testObject = luaPState->GetGlobal("test");
	cout<<((!testObject.IsNil())?testObject.GetString():"nil") << endl;

	unloadLevelGlobals(luaPState);

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
	cout<<x1<<", "<<y1<<", "<<x2<<", "<<y2<<endl;
	b2FixtureDef fixtureDef;
	b2EdgeShape edgeShape;
	b2Vec2 from(x1,y1),to(x2,y2);
	edgeShape.Set(from,to);
	fixtureDef.shape = &edgeShape;
	m_groundBody->CreateFixture(&fixtureDef);
	return 0;
}
int createEdge( float32 x1, float32 y1, float32 x2, float32 y2 )
{
	cout<<x1<<", "<<y1<<", "<<x2<<", "<<y2<<endl;
	return 0;
}
void LuaLevel::loadLevelGlobals(LuaState *pstate)
{
	//FixtureDef~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	b2FixtureDef *fixtureDef = new b2FixtureDef;
	LuaPlus::LuaObject fixtureDefObject = pstate->BoxPointer(fixtureDef);
	pstate->GetGlobals().SetObject("fixtureDef", fixtureDefObject);

	//Box2DFactory~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//pstate->GetGlobals().RegisterObjectDirect( "createEdge",createEdge);
	pstate->GetGlobals().RegisterDirect( "createEdge",*this, &LuaLevel::createAnEdge);
}

int LuaLevel::Index(LuaState *pState)
{
    // Table, Key
    int top = pState->GetTop();
	//pState-
    if( top == 2 && /*pState->Check(1) && */pState->CheckString(2) )
    {
        // t[key]
		std::string key( pState->CheckString(2) );
 
		if(key == "health")
            pState->PushInteger(1);
        else if( key == "maxHealth" )
            pState->PushInteger(2);
        else if( key== "dead" )
            pState->PushBoolean(3 <= 0);
        else if( key== "name" )
            pState->PushString("312");
    }
    // return the count of our pushed values
    return pState->GetTop() - top;
}


void LuaLevel::unloadLevelGlobals(LuaState *pstate)
{
	LuaObject testObject = pstate->GetGlobals().GetByName("fixtureDef");
	//cout<<((!testObject.IsNil())?testObject.GetString():"nil") << endl;
	//cout<<((!testObject.IsNil())?testObject.GetByObject(:"nil") << endl;
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

void LuaLevel::Step(Settings* settings)
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
