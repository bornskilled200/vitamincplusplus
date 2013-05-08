#include "Sound.h"

bool soundInitialized = false;

int audioCallback(const void *input, void *output, 
				  unsigned long frameCount,
				  const PaStreamCallbackTimeInfo* timeInfo,
				  PaStreamCallbackFlags statusFlags,
				  void* userData);
int preloadedAudioCallback(const void *input, void *output, 
				  unsigned long frameCount,
				  const PaStreamCallbackTimeInfo* timeInfo,
				  PaStreamCallbackFlags statusFlags,
				  void* userData);
void finishedCallback( void *userData );

Sound* loadMp3File(const char* filename, Sound* sound)
{
	int err;
	if (soundInitialized==false)
	{
		if (Pa_Initialize() != paNoError) {
			std::cout << "Failed to initialize PortAudio." << std::endl;
			return NULL;
		}

		if ((err = mpg123_init())!=MPG123_OK){
			std::cout<<"An error occurred: "<<mpg123_plain_strerror(err)<<std::endl;
			return NULL;
		}
		soundInitialized = true;
	}

	int channels, encoding;
	long rate;
	mpg123_handle* mh = mpg123_new(NULL, &err);
	if (err!=MPG123_OK) 
	{
		std::cout<<"An error occurred: "<<mpg123_plain_strerror(err)<<std::endl;
		return NULL;
	}

	/* open the file and get the decoding format */
	if ((err = mpg123_open(mh, filename))!=MPG123_OK)
	{
		std::cout<<"An error occurred: "<<mpg123_plain_strerror(err)<<std::endl;
		mpg123_close(mh);
		mpg123_delete(mh);
		return NULL;
	}
	if ((err = mpg123_getformat(mh, &rate, &channels, &encoding))!=MPG123_OK)
	{
		std::cout<<"An error occurred: "<<mpg123_plain_strerror(err)<<std::endl;
		mpg123_close(mh);
		mpg123_delete(mh);
		return NULL;
	}

	/* set the output format and open the output device */
	sound->bits = mpg123_encsize(encoding);
	sound->rate = rate;
	sound->channels = channels;

	std::cout<<"going to scan"<<std::endl;
	if (mpg123_scan(mh)==MPG123_OK)
	{
		std::cout<<"scanned"<<std::endl;
		sound->loaded.resize(mpg123_length(mh) * sound->bits * sound->channels);
		std::cout<<"resized to\t"<<mpg123_length(mh) * sound->bits * sound->channels<<std::endl<<"sized to\t"<<sound->loaded.size()<<std::endl;
		size_t done;
		mpg123_read(mh, &sound->loaded[0], sound->loaded.size(), &done);
	}
	else
	{
		const int bufferSize = 2*2*10000; //big ass buffer
		unsigned char buffer[bufferSize];
		size_t done;

		for (int i = 0; mpg123_read(mh, buffer, bufferSize, &done) == MPG123_OK;)
		{
			int loaded = sound->loaded.size();
			sound->loaded.resize(loaded+done);
			memcpy(&sound->loaded[loaded],buffer,done);
		} 
	}

	return sound;
}

Sound* playMp3File(Sound *sound)
{
	int err;
	if (soundInitialized==false)
	{
		if (Pa_Initialize() != paNoError) {
			std::cout << "Failed to initialize PortAudio." << std::endl;
			return NULL;
		}

		if ((err = mpg123_init())!=MPG123_OK){
			std::cout<<"An error occurred: "<<mpg123_plain_strerror(err)<<std::endl;
			return NULL;
		}
		soundInitialized = true;
	}

	if (Pa_OpenDefaultStream(&sound->pStream, 
		0, // No input channels
		sound->channels, // 2 output channel
		paInt16, // Sample format (see PaSampleFormat)           
		sound->rate, // Sample Rate
		paFramesPerBufferUnspecified,  // Frames per buffer 
		&preloadedAudioCallback,//&audioCallback,
		static_cast<void*>(sound))!=paNoError)//static_cast<void*>(pAudioDecoder)) != paNoError)
	{
		std::cout << "Failed to open the default PortAudio stream." << std::endl;
		if (Pa_Terminate() != paNoError)
		{
			std::cout << "Failed to terminate PortAudio." << std::endl;
			return NULL;
		}
		return NULL;
	}

	if (Pa_StartStream(sound->pStream) != paNoError)
	{
		std::cout << "Failed to start the PortAudio stream." << std::endl;
		if (Pa_Terminate() != paNoError)
		{
			std::cout << "Failed to terminate PortAudio." << std::endl;
			return NULL;
		}
		return NULL;
	} 

	return sound;
}

//Returns a "new" Sound.
Sound* playMp3File(const char* filename)
{
	int err;
	if (soundInitialized==false)
	{
		if (Pa_Initialize() != paNoError) {
			std::cout << "Failed to initialize PortAudio." << std::endl;
			return NULL;
		}

		if ((err = mpg123_init())!=MPG123_OK){
			std::cout<<"An error occurred: "<<mpg123_plain_strerror(err)<<std::endl;
			return NULL;
		}
		soundInitialized = true;
	}

	int channels, encoding;
	long rate;
	Sound *sound = new Sound;
	sound->mh = mpg123_new(NULL, &err);
	if (err!=MPG123_OK) 
	{
		std::cout<<"An error occurred: "<<mpg123_plain_strerror(err)<<std::endl;
		return NULL;
	}

	/* open the file and get the decoding format */
	if ((err = mpg123_open(sound->mh, filename))!=MPG123_OK)
	{
		std::cout<<"An error occurred: "<<mpg123_plain_strerror(err)<<std::endl;
		mpg123_close(sound->mh);
		mpg123_delete(sound->mh);
		return NULL;
	}
	if ((err = mpg123_getformat(sound->mh, &rate, &channels, &encoding))!=MPG123_OK)
	{
		std::cout<<"An error occurred: "<<mpg123_plain_strerror(err)<<std::endl;
		mpg123_close(sound->mh);
		mpg123_delete(sound->mh);
		return NULL;
	}

	/* set the output format and open the output device */
	int m_bits = mpg123_encsize(encoding);
	int m_rate = rate;
	int m_channels = channels;

	// Open the PortAudio stream (opens the soundcard device).
	if (Pa_OpenDefaultStream(&sound->pStream, 
		0, // No input channels
		m_channels, // 2 output channel
		paInt16, // Sample format (see PaSampleFormat)           
		m_rate, // Sample Rate
		paFramesPerBufferUnspecified,  // Frames per buffer 
		&audioCallback,//&audioCallback,
		static_cast<void*>(sound))!=paNoError)//static_cast<void*>(pAudioDecoder)) != paNoError)
	{
		std::cout << "Failed to open the default PortAudio stream." << std::endl;
		mpg123_close(sound->mh);
		mpg123_delete(sound->mh);
		if (Pa_Terminate() != paNoError)
		{
			std::cout << "Failed to terminate PortAudio." << std::endl;
			return NULL;
		}
		return NULL;
	}

	if (Pa_SetStreamFinishedCallback( sound->pStream, &finishedCallback) != paNoError)
	{
		std::cout << "Failed to register finished callback." << std::endl;
		mpg123_close(sound->mh);
		mpg123_delete(sound->mh);
		if (Pa_Terminate() != paNoError)
		{
			std::cout << "Failed to terminate PortAudio." << std::endl;
			return NULL;
		}
		return NULL;
	} 


	if (Pa_StartStream(sound->pStream) != paNoError)
	{
		std::cout << "Failed to start the PortAudio stream." << std::endl;
		mpg123_close(sound->mh);
		mpg123_delete(sound->mh);
		if (Pa_Terminate() != paNoError)
		{
			std::cout << "Failed to terminate PortAudio." << std::endl;
			return NULL;
		}
		return NULL;
	}  

	return sound;
}
#include <algorithm>
int preloadedAudioCallback(const void *input, void *output, 
						   unsigned long frameCount,
						   const PaStreamCallbackTimeInfo* timeInfo,
						   PaStreamCallbackFlags statusFlags,
						   void* userData)
{	
	Sound* sound = static_cast<Sound*>(userData);

	unsigned long bufferSize = frameCount*sound->channels*sound->bits;
	size_t next = std::min(bufferSize,(unsigned long)sound->loaded.size()-sound->pos);
	memcpy(output,&sound->loaded[sound->pos],next);
	if (bufferSize-next)
	{
		if (sound->loop)
		{
			sound->pos = bufferSize-next;
			memcpy(output,&sound->loaded[0],bufferSize-next);
		}
		else
		{
			memset((unsigned char*)output+next,0,bufferSize-next);
			return paComplete;
		}
	}
	sound->pos+=next;
	return paContinue;
}

int audioCallback(const void *input, void *output, 
				  unsigned long frameCount,
				  const PaStreamCallbackTimeInfo* timeInfo,
				  PaStreamCallbackFlags statusFlags,
				  void* userData)
{
	Sound* sound = static_cast<Sound*>(userData);

	size_t done = 0;
	if (mpg123_read(sound->mh, (unsigned char*)output, frameCount*2*sizeof(short), &done) == MPG123_OK) {	
		return paContinue;
	}
	else
	{
		memset((unsigned char*)output, 0, frameCount * 2*sizeof(short));
		return paComplete;
	}
}

void finishedCallback( void *userData )
{
	Sound* sound = static_cast<Sound*>(userData);

	// Tell the PortAudio library that we're all done with it.
	if (Pa_Terminate() != paNoError)
	{
		std::cout << "Failed to terminate PortAudio." << std::endl;
		return;
	}
	mpg123_close(sound->mh);
	mpg123_delete(sound->mh);
	mpg123_exit();
}

void terminateSound()
{
	// Tell the PortAudio library that we're all done with it.
	if (Pa_Terminate() != paNoError)
	{
		std::cout << "Failed to terminate PortAudio." << std::endl;
		return;
	}

	mpg123_exit();
}