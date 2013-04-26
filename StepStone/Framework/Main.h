#ifndef MAIN_H
#define MAIN_H

#include <Box2D\Box2D.h>

class Settings
{
public:
	Settings() :
		viewCenter(0.0f, 20.0f),
		viewZoom(1.0f),
		hz(60.0f),
		pause(0),
		singleStep(0)
	{}
	~Settings() {}
	
	inline b2Vec2 getViewCenter() { return viewCenter; }
	inline void setViewCenter(b2Vec2 set);
	inline float32 getViewZoom() { return viewZoom; }
	inline void setViewZoom(float32 set);
	inline float32 getHz() { return hz; }
	inline void setHz(float32 set) { hz = set; }
	inline int32 getPause() { return pause; }
	inline void setPause(int32 set) { pause = set; }
	inline int32 getSingleStep() { return singleStep; }
	inline void setSingleStep(int32 set) { singleStep = set; }

private:
	b2Vec2 viewCenter;
	float32 viewZoom;
	float32 hz;
	int32 pause;
	int32 singleStep;
};
#endif