#ifndef MPG_H
#define MPG_H

#include "mpg123.h"
#include "portaudio\portaudio.h"
#include <iostream>

#define INBUFF  16384
#define OUTBUFF 32768

struct mpg123Struct
{
	mpg123Struct():done(0){}
	mpg123_handle *mh;
	unsigned char *buffer;
    size_t buffer_size;
    size_t done;
	int totalBtyes;
};

inline void loadMp3File(const char* filename, PaStreamCallback* audioCallBack)
{
	if (Pa_Initialize() != paNoError) {
        std::cout << "Failed to initialize PortAudio." << std::endl;
        return;
    };

	Pa_GetDefaultOutputDevice();

	PaStream* pStream = NULL;
    
	mpg123Struct* mpgStruct = new mpg123Struct;
    int err;

    int channels, encoding;
    long rate;

    if (mpg123_init()!=MPG123_OK) std::cout<<"error1"<<std::endl;
    mpgStruct->mh = mpg123_new(NULL, &err);
    mpgStruct->buffer_size = mpg123_outblock(mpgStruct->mh);
    mpgStruct->buffer = (unsigned char*) malloc(mpgStruct->buffer_size * sizeof(unsigned char));

    /* open the file and get the decoding format */
    if (mpg123_open(mpgStruct->mh, filename)!=MPG123_OK) std::cout<<"error1"<<std::endl;
	if (mpg123_getformat(mpgStruct->mh, &rate, &channels, &encoding)!=MPG123_OK) std::cout<<"error1"<<std::endl;

    /* set the output format and open the output device */
    int m_bits = mpg123_encsize(encoding);
	std::cout<<m_bits<<" bits check2"<<std::endl;
    int m_rate = rate;
    int m_channels = channels;
	
	// Open the PortAudio stream (opens the soundcard device).
	std::cout<<"hi"<<std::endl;
    if (Pa_OpenDefaultStream(&pStream, 
                  0, // No input channels
                  m_channels, // 2 output channel
				  paInt16, // Sample format (see PaSampleFormat)           
                  m_rate, // Sample Rate
                  paFramesPerBufferUnspecified,  // Frames per buffer 
                  audioCallBack,//&audioCallback,
                  static_cast<void*>(mpgStruct))!=paNoError)//static_cast<void*>(pAudioDecoder)) != paNoError)
    {
        std::cout << "Failed to open the default PortAudio stream." << std::endl;
        return;
    }
    /* decode and play */
    /*
	for (int totalBtyes = 0 ; mpg123_read(mpgStruct->mh, mpgStruct->buffer, mpgStruct->buffer_size, &mpgStruct->done) == MPG123_OK ; ) {
		//Pa_WriteStream(pStream, mpgStruct->buffer,mpgStruct->done/4);
        totalBtyes += mpgStruct->done;
		//std::cout<<"huh"<<buffer<<" "<<done<<", "<<done/4<<std::endl;
    }
	*/
	if (Pa_StartStream(pStream) != paNoError)
    {
        std::cout << "Failed to start the PortAudio stream." << std::endl;
        return;
    }  
	
	// Shutdown:
    // First, stop the PortAudio stream (closes the soundcard device).
	/*
    if (Pa_StopStream(pStream) != paNoError)
    {
        std::cout << "Failed to stop the PortAudio stream." << std::endl;
        return;
    }  

	if (Pa_CloseStream(pStream) != paNoError)
    {
        std::cout << "Failed to stop the PortAudio stream." << std::endl;
        return;
    }     

    // Tell the PortAudio library that we're all done with it.
    if (Pa_Terminate() != paNoError)
    {
        std::cout << "Failed to terminate PortAudio." << std::endl;
        return;
    }

    /* clean up */
	/*
    free(mpgStruct->buffer);
    mpg123_close(mpgStruct->mh);
    mpg123_delete(mpgStruct->mh);
    mpg123_exit();
	*/
}

#endif