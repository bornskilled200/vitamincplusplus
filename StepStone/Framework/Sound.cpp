#include "Sound.h"

Sound* loadMp3File(const char* filename)
{
	return loadMp3File(filename, new Sound());
}

Sound* loadMp3File(const char* filename, Sound* sound)
{
	if (Pa_Initialize() != paNoError) {
        std::cout << "Failed to initialize PortAudio." << std::endl;
        return NULL;
    };

    int err;
    int channels, encoding;
    long rate;

    if ((err = mpg123_init())!=MPG123_OK){
		std::cout<<"An error occurred: "<<mpg123_plain_strerror(err)<<std::endl;
		return NULL;
	}
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

int audioCallback(const void *input, void *output, 
				  unsigned long frameCount,
				  const PaStreamCallbackTimeInfo* timeInfo,
				  PaStreamCallbackFlags statusFlags,
				  void* userData)
{
	Sound* sound = static_cast<Sound*>(userData);
	// I HAVE NO IDEA WHY THIS WORKKKKKSSSSSSSSS
	size_t done = 0;
	if (mpg123_read(sound->mh, (unsigned char*)output, frameCount*4, &done) == MPG123_OK) {	
		return paContinue;
	}
	else
	{
		/* clean up */
		memset((unsigned char*)output, 0, frameCount * 4);
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