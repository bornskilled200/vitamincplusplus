#ifndef MAIN_H
#define MAIN_H

#include <Box2D\Box2D.h>

class Settings
{
public:
	Settings() :
		viewPosition(0.0f, 0.0f),
		viewSize(1.0f),
		hz(60.0f),
		pause(0),
		singleStep(0),
		widthIsMinimum(true)
	{}
	~Settings() {}
	
	inline b2Vec2 getViewPosition() { return viewPosition; }
	inline void setViewPosition(b2Vec2 set);
	inline float32 getViewSize() { return viewSize; }
	inline void setViewSize(float32 set);
	inline bool isWidthIsMinimum() { return widthIsMinimum; }
	inline void setWidthIsMinimum(bool set);
	inline float32 getHz() { return hz; }
	inline void setHz(float32 set) { hz = set; }
	inline int32 getPause() { return pause; }
	inline void setPause(int32 set) { pause = set; }
	inline int32 getSingleStep() { return singleStep; }
	inline void setSingleStep(int32 set) { singleStep = set; }
	
	bool widthIsMinimum;
private:
	b2Vec2 viewPosition;
	float32 viewSize;
	float32 hz;
	int32 pause;
	int32 singleStep;
};
#endif