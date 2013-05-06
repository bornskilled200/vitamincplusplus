
#ifndef SIMPLEWRAPPER_H
#define SIMPLEWRAPPER_H

#include "lodepng\lodepng.h"
#include "freeglut\freeglut.h"
#include <string>
#include <iostream>
#include <vector>
#include <assert.h>
using namespace std;

namespace Graphics
{


	struct Texture
	{
		Texture():imageWidth(0),imageHeight(0),scaledImageWidth(0),scaledImageHeight(0),id(0){}
		unsigned int imageWidth, imageHeight;
		float scaledImageWidth, scaledImageHeight;
		unsigned int id;
	};

	class AnimatedTexture
	{
	private:
		vector<Texture> textures;
		int textureCount;
		int currentTexture;
		vector<int> framesPerTexture;
		int currentFrame;
	public:
		AnimatedTexture(vector<Texture> &_textures, size_t _textureCount, vector<int> &_framesPerTexture)
		{
			assert(_textures.size()>=_textureCount && _framesPerTexture.size()>=_textureCount);
			textures = _textures;
			textureCount = _textureCount;
			framesPerTexture = _framesPerTexture;
			currentTexture = 0;
			currentFrame = 0;
		}

		inline Texture getTexture(int index)
		{
			//assert(index<textureCount && index>=0);
			return textures[index];
		}

		inline Texture getCurrentTexture()
		{
			return getTexture(currentTexture);
		}

		inline Texture updateAndGetTexture()
		{
			return getTexture(((currentFrame+=1)%=framesPerTexture[currentTexture])?currentTexture:(currentTexture+=1)%=textureCount);
		}

		inline int getTextureCount()
		{
			return textureCount;
		}
		inline int getCurrentCount()
		{
			return textureCount;
		}
	};
	
GLuint loadTexture(vector<unsigned char> &image, string fileName, unsigned int &imageWidth, unsigned int &imageHeight, float &scaledImageWidth, float &scaledImageHeight, bool buildMipMaps=false);
	//void loadTextures(vector<unsigned char> &image, vector<string> fileName, vector<Texture> textures);
	
	//This draws the texture flipped, so perfect for a directly loaded png!
	void drawImage(unsigned int width, unsigned int height, GLfloat scaledWidth, GLfloat scaledHeight);

	inline void loadATexture(string fileName, Texture *texture,vector<unsigned char> &image, bool buildMipMaps=false)
	{
		texture->id = loadTexture(image, fileName, texture->imageWidth,texture->imageHeight,texture->scaledImageWidth,texture->scaledImageHeight, buildMipMaps);
	}
	
	inline void loadATexture(string fileName, Texture *texture, bool buildMipMaps=false)
	{
		vector<unsigned char> image;
		texture->id = loadTexture(image, fileName, texture->imageWidth,texture->imageHeight,texture->scaledImageWidth,texture->scaledImageHeight, buildMipMaps);
	}

	/* This method assumes that this is the only method that is being called to draw textures */
	inline void drawImageOptimistically(Graphics::Texture *texture);

	inline void drawImage(Graphics::Texture *texture)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture->id);
		drawImage(texture->imageWidth,texture->imageHeight,texture->scaledImageWidth,texture->scaledImageHeight);
	}

}
#endif
