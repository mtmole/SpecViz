
#include "SpecViz.h"
#include <fstream>

GLenum Texture::GetInternal(GLenum format) {
	switch (format) {
		case GL_LUMINANCE8:
		case GL_LUMINANCE16:
			return GL_LUMINANCE;
	}

	return format;
}

GLenum Texture::GetFormat(GLenum format) {
	switch (format) {
		case GL_LUMINANCE8:
		case GL_LUMINANCE16:
			return GL_LUMINANCE;
		case GL_RGBA8:
			return GL_RGBA;
	}

	return 0;
}

GLenum Texture::GetDataType(GLenum format) {
	switch (format) {
		case GL_LUMINANCE8:
		case GL_RGBA8:
			return GL_UNSIGNED_BYTE;
		case GL_LUMINANCE16:
			return GL_UNSIGNED_SHORT;
	}

	return 0;
}

Texture::Texture(void* data, uint32_t withWidth, uint32_t withHeight, GLenum withFormat) {
	width = withWidth;
	height = withHeight;
	format = withFormat;

	glGenTextures(1, &textureId);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId);
	GLCHECK();
		
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	GLCHECK();

	glTexImage2D(GL_TEXTURE_2D, 0, GetInternal(format), width, height, 0, GetFormat(format), GetDataType(format), data);
	GLCHECK();
}

void Texture::Bind(uint32_t samplerId) {
	glActiveTexture(GL_TEXTURE0 + samplerId);
	glBindTexture(GL_TEXTURE_2D, textureId);
}

Texture* Texture::CreateFromFile(const char* filename, GLenum desiredFormat)
{
	//check the file signature and deduce its format
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(filename, 0);
	//if still unknown, try to guess the file format from the file extension
	if(fif == FIF_UNKNOWN) 
		fif = FreeImage_GetFIFFromFilename(filename);
	//if still unkown, return failure
	if(fif == FIF_UNKNOWN)
		return NULL;
	
	//pointer to the image, once loaded
	FIBITMAP *dib(0);
	//check that the plugin has reading capabilities and load the file
	if(FreeImage_FIFSupportsReading(fif))
		dib = FreeImage_Load(fif, filename);
	//if the image failed to load, return failure
	if(!dib)
		return false;
	
	//retrieve the image data
	BYTE* bits = FreeImage_GetBits(dib);
	//get the image width and height
	uint32_t width = FreeImage_GetWidth(dib);
	uint32_t height = FreeImage_GetHeight(dib);
	uint32_t bpp = FreeImage_GetBPP(dib);
	uint32_t numcolors = FreeImage_GetColorsUsed(dib);
	FREE_IMAGE_COLOR_TYPE type = FreeImage_GetColorType(dib);
	uint32_t redMask = FreeImage_GetRedMask(dib);
	uint32_t greenMask = FreeImage_GetGreenMask(dib);
	uint32_t blueMask = FreeImage_GetBlueMask(dib);
	uint32_t pitch = FreeImage_GetPitch(dib);

	//if this somehow one of these failed (they shouldn't), return failure
	if((bits == 0) || (width == 0) || (height == 0))
		return false;

	bool loaded = false;
	Texture* ret = NULL;

	if (desiredFormat == GL_LUMINANCE8) {
		if (bpp == 16 && type == FIC_MINISBLACK) {
			unsigned char* newBits = (unsigned char*) malloc(width * height);
			unsigned short* src = (unsigned short*) bits;
			for (uint32_t i = 0; i < width * height; i++) {
				newBits[i] = (unsigned char) (src[i] / 256);
			}
			ret = new Texture(newBits, width, height, desiredFormat);
			free(newBits);
			loaded = true;
		} 
	}
	if (desiredFormat == GL_RGBA8) {
		if (bpp == 24) {
			unsigned char* newBits = (unsigned char*) malloc(width * height * 4);
			unsigned char* src = (unsigned char*) bits;
			for (uint32_t i = 0; i < width * height * 4; i += 4) {
				newBits[i] = src[2];
				newBits[i+1] = src[1];
				newBits[i+2] = src[0];
				newBits[i+3] = 255;
				src += 3;
			}
			ret = new Texture(newBits, width, height, desiredFormat);
			free(newBits);
			loaded = true;
		}
	}
	
	if (!loaded) {
		// default behavior just loads it
		ret = new Texture(bits, width, height, desiredFormat);
	}

	//Free FreeImage's copy of the data
	FreeImage_Unload(dib);

	//return success
	return ret;
}