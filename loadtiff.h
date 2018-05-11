#ifndef loadtiff_h
#define loadtiff_h

#include <stdio.h>


/*
  By Malcolm McLean


  To use
  int width, height;
  unsigned char *rgba;
  FILE *fp = fopen("tiffile.tiff", "rb");
  if(!fp)
     errorhandling();
  rgba = floadtiff(fp, &width, &height);
  fclose(fp);
  
  if(rgba == 0)
     printf("TIFF file unreadable\n");
   
     rgba is a red, green, blue, alpha 32 bit buffer,
     width is image width, height is image height in pixels
  */

#define FMT_ERROR 0
#define FMT_RGBA 1
#define FMT_CMYK 2
#define FMT_CMYKA 3
#define FMT_GREYALPHA 4
#define FMT_RGB 5
#define FMT_GREY 6

unsigned char *floadtiffwhite(FILE *fp, int *width, int *height, int *format);
unsigned char *floadtiff(FILE *fp, int *width, int *height, int *format);

#endif
