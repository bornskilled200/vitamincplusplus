#include "Graphics.h"
#include "Main.h"

GLuint Graphics::loadTexture(vector<unsigned char> &image, string fileName, unsigned int &imageWidth, unsigned int &imageHeight, float &scaledImageWidth, float &scaledImageHeight)
{
		image.clear();

		unsigned error = lodepng::decode(image, imageWidth, imageHeight, fileName);
		//if there's an error, display it
		if(error) 
		{
			std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << " for " << fileName<<std::endl;
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
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
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

		return id;
	}

static GLuint bindedTexture= 0;

inline void drawImageOptimistically(Graphics::Texture *texture)
{
	if (bindedTexture!=texture->id)
		glBindTexture(GL_TEXTURE_2D, bindedTexture=texture->id);
	Graphics::drawImage(texture->imageWidth,texture->imageHeight,texture->scaledImageWidth,texture->scaledImageHeight);
}

void Graphics::drawImage(unsigned int width, unsigned int height, GLfloat scaledWidth, GLfloat scaledHeight)
{
	//glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2f(		   0, scaledHeight); glVertex2i(	0,	    0);
	glTexCoord2f(scaledWidth, scaledHeight); glVertex2i(width,	    0);
	glTexCoord2f(scaledWidth,			 0); glVertex2i(width, height);
	glTexCoord2f(		   0,			 0); glVertex2i(	0, height);
	glEnd();
	//glDisable(GL_TEXTURE_2D);
}

void Graphics::drawIntroImage(unsigned int width, unsigned int height, GLfloat scaledWidth, GLfloat scaledHeight, unsigned int vpW)
{
	float perc = .20f;
	//glEnable(GL_TEXTURE_2D);
	int dis = perc*vpW;
	glBegin(GL_QUADS);
	glTexCoord2f(		  0, scaledHeight); glVertex2i(	dis,	    0);
	glTexCoord2f(scaledWidth, scaledHeight); glVertex2i(width+dis,	    0);
	glTexCoord2f(scaledWidth,			 0); glVertex2i(width+dis, height);
	glTexCoord2f(		  0,			 0); glVertex2i( dis, height);
	glEnd();
	//glDisable(GL_TEXTURE_2D);
}

void Graphics::drawImage(int x, int y, int width, int height, GLfloat scaledWidth, GLfloat scaledHeight)
{
	glBegin(GL_QUADS);
	glTexCoord2f(		   0, scaledHeight); glVertex2i(	x	 ,	    y);
	glTexCoord2f(scaledWidth, scaledHeight); glVertex2i(x + width,	    y);
	glTexCoord2f(scaledWidth,			 0); glVertex2i(x+ width , y + height);
	glTexCoord2f(		   0,			 0); glVertex2i(	x	 , y + height);
	glEnd();
}

void Graphics::drawImage(unsigned int x, unsigned int y, float width, float height, GLfloat scaledWidth, GLfloat scaledHeight)
{
	glBegin(GL_QUADS);
	glTexCoord2f(		   0, scaledHeight); glVertex2f(	x	 ,	    y);
	glTexCoord2f(scaledWidth, scaledHeight); glVertex2f(x + width,	    y);
	glTexCoord2f(scaledWidth,			 0); glVertex2f(x+ width , y + height);
	glTexCoord2f(		   0,			 0); glVertex2f(	x	 , y + height);
	glEnd();
}

