#ifndef MPG_H
#define MPG_H

#include "mpg123\mpg123.h"
#include "portaudio\portaudio.h"
#include <iostream>

#define INBUFF  16384
#define OUTBUFF 32768

struct Sound
{
	Sound():mh(NULL),pStream(NULL){}
	mpg123_handle *mh;
	PaStream* pStream;
	short* loaded;
	int pos;
};

Sound* loadMp3File(const char* filename, Sound* sound);
Sound* playMp3File(const char* filename, Sound* sound);
int audioCallback(const void *input, void *output, 
				  unsigned long frameCount,
				  const PaStreamCallbackTimeInfo* timeInfo,
				  PaStreamCallbackFlags statusFlags,
				  void* userData);
void finishedCallback( void *userData );
#endif