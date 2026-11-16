#ifndef IMAGE_H
#define IMAGE_H

// Small image loader (PNG, PPM) with its own inflate decoder

#include <string>
#include <vector>

struct Image
{
	int width = 0;
	int height = 0;
	std::vector<unsigned char> rgb;     // 3 bytes per pixel, first row = TOP of the image
};

// PNG (8/16 bit, grey / RGB / palette / with alpha, non-interlaced) or binary PPM (P6)
bool loadImage(const std::string& path, Image& img);

// Procedurally generated RGB image (colourful swirl pattern), used when no file is given
Image makeProceduralImage(int width, int height);

#endif
