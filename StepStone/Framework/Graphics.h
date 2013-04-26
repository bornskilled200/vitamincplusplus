
#ifndef SIMPLEWRAPPER_H
#define SIMPLEWRAPPER_H

#include "lodepng\lodepng.h"
//#include "glui/glui.h"
#include "freeglut\freeglut.h"
#include <string>
#include <iostream>
#include <vector>
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
		Texture* textureArray;
		int textureCount;
		int currentTexture;
	public:
		AnimatedTexture(Texture _textureArray[], int _textureCount)
		{
			textureArray = _textureArray;
			textureCount = _textureCount;
			currentTexture = 0;
		}

		inline Texture getTexture(int index)
		{
			//assert(index<textureCount && index>=0);
			return textureArray[index];
		}

		inline Texture getCurrentTexture()
		{
			return getTexture(currentTexture);
		}

		inline Texture updateAndGetTexture()
		{
			return getTexture((currentTexture++)%textureCount);
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

	inline GLuint loadTexture(vector<unsigned char> &image, string fileName, unsigned int &imageWidth, unsigned int &imageHeight, float &scaledImageWidth, float &scaledImageHeight)
	{
		image.clear();
		unsigned error = lodepng::decode(image, imageWidth, imageHeight, fileName);
		//if there's an error, display it
		if(error) 
		{
			std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
			return 0;
		}

		// Texture size must be power of two for the primitive OpenGL version this is written for. Find next power of two.
		size_t u2 = 1; while(u2 < imageWidth) u2 *= 2;
		size_t v2 = 1; while(v2 < imageHeight) v2 *= 2;
		// Ratio for power of two version compared to actual version, to render the non power of two image with proper size.
		scaledImageWidth = (float)imageWidth / u2;
		scaledImageHeight= (float)imageHeight / v2;

		GLuint id;
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);//evrything we're about to do is about this texture
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		if (u2==imageWidth && v2 == imageHeight)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, 4, u2, v2, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
		}
		else // Make power of two version of the image.
		{
			std::cout<<fileName<<" needs to be fixed to be a power of 2!"<<std::endl;
			std::vector<unsigned char> image2(u2 * v2 * 4);
			for(size_t y = 0; y < imageHeight; y++)
				for(size_t x = 0; x < imageWidth; x++)
					for(size_t c = 0; c < 4; c++)
					{
						image2[4 * u2 * y + 4 * x + c] = image[4 * imageWidth * y + 4 * x + c];
					}
			glTexImage2D(GL_TEXTURE_2D, 0, 4, u2, v2, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image2[0]);
		}
		//std::cout<<"hello3"<<std::endl;
				// Enable the texture for OpenGL.
				//glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
				//gluBuild2DMipmaps(GL_TEXTURE_2D, 4, u2, v2, GL_RGBA, GL_UNSIGNED_BYTE, &image2[0] ); // i should not be doing this but we need an updated version of gl.h
				
		return id;
	}
	inline void loadATexture(string fileName, Texture *texture,vector<unsigned char> &image)
	{
		texture->id = loadTexture(image, fileName,texture->imageWidth,texture->imageHeight,texture->scaledImageWidth,texture->scaledImageHeight);
	}

	//This draws the texture flipped, so perfect for a directly loaded png!
	inline void drawImage(GLubyte id, unsigned int width, unsigned int height, GLfloat scaledWidth, GLfloat scaledHeight)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, id);
		glBegin(GL_QUADS);
		glTexCoord2f(		   0, scaledHeight); glVertex2i(	0,	    0);
		glTexCoord2f(scaledWidth, scaledHeight); glVertex2i(width,	    0);
		glTexCoord2f(scaledWidth,			 0); glVertex2i(width, height);
		glTexCoord2f(		   0,			 0); glVertex2i(	0, height);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}

	inline void drawImage(Graphics::Texture *texture)
	{
		drawImage(texture->id,texture->imageWidth,texture->imageHeight,texture->scaledImageWidth,texture->scaledImageHeight);
	}

}
#endif
