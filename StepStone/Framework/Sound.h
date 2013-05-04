#ifndef MPG_H
#define MPG_H

#include "mpg123\mpg123.h"
#include "portaudio\portaudio.h"
#include <iostream>
#include <vector>

#define INBUFF  16384
#define OUTBUFF 32768

struct Sound
{
	Sound():mh(NULL),pStream(NULL),pos(0){}
	mpg123_handle *mh;
	PaStream* pStream;
	std::vector<unsigned char> loaded;
	int pos;
	bool loop;
	int channels;
	int rate;
	int bits;
};

Sound* loadMp3File(const char* filename, Sound* sound);
Sound* playMp3File(const char* filename);
Sound* playMp3File(Sound* sound);

void terminateSound();
#endif