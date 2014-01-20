/*
 * This file is part of tga2exr - a command-line utility for converting
 * high-dynamic-range Targa (TGA) image files to the OpenEXR (EXR) format.
 *
 * https://github.com/toaarnio/tga2exr/
 *
 * Copyright (C) 2011 Tomi Aarnio
 *
 * This software is based on the OpenEXR image file format and library
 * by Industrial Light & Magic, see http://www.openexr.org.
 */

// Include files are located in ./OpenEXR/include; the corresponding
// libraries are in ./OpenEXR/lib. The include and library paths must
// be specified in project settings.

#include <ImfRgbaFile.h>              // EXR loading
#include <ImfOutputFile.h>            // EXR saving
#include <ImfChannelList.h>
#include <ImfArray.h>

using namespace Imf;

////////////////////////////////////////////////////////////////////////////////
// Loads a TGA file from disk. Hardcoded for HDR (48 bpp) images.
////////////////////////////////////////////////////////////////////////////////
static half* loadTGAFile(const char *filename, unsigned short& width, unsigned short& height)
{
  if (filename == 0) {
    return NULL;
  }

  printf("Reading %s...\n", filename);
  FILE *infile;
  fopen_s(&infile, filename, "rb");
  if (infile == NULL) {
    printf("Unable to read from file %s.\n", filename);
    return NULL;
  }

  char discard[12];
  char bitdepth;
  fread(discard, 1, 12, infile);
  fread(&width, 2, 1, infile);
  fread(&height, 2, 1, infile);
  fread(&bitdepth, 1, 1, infile);
  fread(discard, 1, 1, infile);
  printf("width and height: %d %d\n", width, height);
  printf("bits per pixel: %d\n", bitdepth);
  if (bitdepth != 48) {
    printf("%s is not a valid HDR TGA file.\n", filename);
    return NULL;
  }

  half *imageData = (half *)malloc(width*height*3*sizeof(half));
  size_t nb = fread(imageData, sizeof(half), width*height*3, infile);
  printf("read %d pixels\n", nb);
  return imageData;
}

////////////////////////////////////////////////////////////////////////////////
// Saves an EXR file from Array<Rgba> data.
////////////////////////////////////////////////////////////////////////////////
static bool saveEXRFile (const char *filename, 
                          const int width, 
                          const int height, 
                          Array<Rgba>* imageData) 
{

  if (filename == NULL || imageData == NULL || width <= 0 || height <= 0) {
    printf("Cannot write EXR file: invalid filename or image data.\n");
    return false;
  }

  // prepare header
  Header header (width, height);
  header.channels().insert ("R", Channel (HALF));
  header.channels().insert ("G", Channel (HALF));
  header.channels().insert ("B", Channel (HALF));

  // create file
  OutputFile exrFile (filename, header);

  // insert frame buffer
  FrameBuffer frameBuffer;
  frameBuffer.insert ("R",									// name
    Slice (HALF,														// type
    (char *) &(((Rgba*)imageData[0])->r),		// base
    sizeof (Rgba),													// xStride
    sizeof (Rgba) * width));								// yStride

  frameBuffer.insert ("G",									// name
    Slice (HALF,														// type
    (char *) &(((Rgba*)imageData[0])->g),		// base
    sizeof (Rgba),													// xStride
    sizeof (Rgba) * width));								// yStride

  frameBuffer.insert ("B",									// name
    Slice (HALF,														// type
    (char *) &(((Rgba*)imageData[0])->b),		// base
    sizeof (Rgba),											    // xStride
    sizeof (Rgba) * width));								// yStride

  exrFile.setFrameBuffer (frameBuffer);
  exrFile.writePixels (height);

  return true;
}

//======================================================================
// Main
//======================================================================

int main(int argc, const char *argv[])
{
  if (argc < 3) {
    printf("Usage:\ttga2exr <image.tga> <image.exr>\n");
    return -1;
  }

  unsigned short width, height;
  half *imageData = loadTGAFile(argv[1], width, height);
  if (imageData == NULL) {
    return -1;      
  }

  Array<Rgba> *exrImageData = new Array<Rgba>(width*height);
  exrImageData->resizeErase(height*width);
  Rgba *pixels = exrImageData[0];
  for (int i=0, r=height-1; r >= 0; r--) {
    for (int c=0; c < width; c++, i++) {
      pixels[i].r = imageData[ (r*width + c)*3     ];
      pixels[i].g = imageData[ (r*width + c)*3 + 1 ];
      pixels[i].b = imageData[ (r*width + c)*3 + 2 ];
      pixels[i].a = 1.0f;
    }
  }

  free(imageData);

  printf("TGA file read successfully, converting to EXR...\n");
  bool success = saveEXRFile(argv[2], width, height, exrImageData);
  if (success) {
    printf("EXR file %s successfully written.\n", argv[2]);
  } else {
    printf("Failed to write EXR file %s.\n", argv[2]);
  }
  return 0;
}
