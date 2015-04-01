
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

struct ImageBits {
	uint32_t width;
	uint32_t height;
	uint8_t* image;

	ImageBits() {
		width = 0;
		height = 0;
		image = NULL;
	}

	void free() {
		if (image) {
			::free((void*) image);
			image = NULL;
		}
	}
};

ImageBits GetFileBits(const char* filename) {
	ImageBits ret;
	
	//check the file signature and deduce its format
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(filename, 0);
	//if still unknown, try to guess the file format from the file extension
	if(fif == FIF_UNKNOWN) 
		fif = FreeImage_GetFIFFromFilename(filename);
	//if still unkown, return failure
	if(fif == FIF_UNKNOWN)
		return ret;
	
	//pointer to the image, once loaded
	FIBITMAP *dib(0);
	//check that the plugin has reading capabilities and load the file
	if(FreeImage_FIFSupportsReading(fif))
		dib = FreeImage_Load(fif, filename);
	//if the image failed to load, return failure
	if(!dib)
		return ret;
	
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
		return ret;

	bool loaded = false;

	ret.width = width;
	ret.height = height;
	ret.image = (uint8_t*) malloc(width * height * 4);

	if (bpp == 16 && type == FIC_MINISBLACK) {
		unsigned short* src = (unsigned short*) bits;
		for (uint32_t i = 0; i < width * height * 4; i++) {
			ret.image[i] = (unsigned char) (src[i/4] / 256);
		}
		loaded = true;
	} 
	if (bpp == 24) {
		unsigned char* src = (unsigned char*) bits;
		for (uint32_t i = 0; i < width * height * 4; i += 4) {
			ret.image[i] = src[2];
			ret.image[i+1] = src[1];
			ret.image[i+2] = src[0];
			ret.image[i+3] = 255;
			src += 3;
		}
		loaded = true;
	}
	
	if (!loaded) {
		// default behavior just loads it
		memcpy(ret.image, bits, width * height * 4);
	}

	//Free FreeImage's copy of the data
	FreeImage_Unload(dib);

	//return success
	return ret;
}

Texture* Texture::CreateFromFile(const char* filename, GLenum desiredFormat)
{
	ImageBits image = GetFileBits(filename);

	//if image loaded failed return failure
	if (image.image == NULL)
		return false;

	Texture* ret = NULL;

	if (desiredFormat == GL_LUMINANCE8) {
		unsigned char* newBits = (unsigned char*) malloc(image.width * image.height);
		for (uint32_t i = 0; i < image.width * image.height; i++) {
			newBits[i] = (unsigned char) (image.image[i * 4]);
		}
		ret = new Texture(newBits, image.width, image.height, desiredFormat);
		free(newBits);
	}
	assert(ret || desiredFormat == GL_RGBA8);
	
	if (!ret) {
		// default behavior just loads it
		ret = new Texture(image.image, image.width, image.height, desiredFormat);
	}

	// free up image
	image.free();

	//return success
	return ret;
}

Texture* Texture::CreateFromFileCombined(const char* rgbPath, const char* alphaPath) {
	ImageBits imageRGB = GetFileBits(rgbPath);
	ImageBits imageAlpha = GetFileBits(alphaPath);

	//if RGB image loaded failed return failure
	if (imageRGB.image == NULL) {
		imageAlpha.free();
		return false;
	}

	// if alpha image loaded, transform alpha R channgel to imageRGB A channel
	if (imageAlpha.image != NULL) {
		assert(imageAlpha.width == imageRGB.width && imageAlpha.height == imageRGB.height);
		for (uint32_t i = 0; i < imageRGB.width * imageRGB.height; i++) {
			imageRGB.image[i*4+3] = imageAlpha.image[i*4];
		}
	}

	Texture* ret = new Texture(imageRGB.image, imageRGB.width, imageRGB.height, GL_RGBA8);

	imageRGB.free();
	imageAlpha.free();

	return ret;
}

void Texture::SavePNG(const char* filename, uint32_t width, uint32_t height, uint8_t* colors, uint32_t destWidth, uint32_t destHeight) {
	FIBITMAP* Image = FreeImage_ConvertFromRawBits(colors, width, height, 3*width, 24, 0xFF0000, 0x00FF00, 0x0000FF, false);

	if (destWidth != 0 && destHeight != 0) {
		FIBITMAP* OldImage = Image;
		Image = FreeImage_Rescale(Image, destWidth, destHeight, FILTER_BILINEAR);
		FreeImage_Unload(OldImage);
	}

	FreeImage_Save(FIF_PNG, Image, filename, 0);
	FreeImage_Unload(Image);
}