/*
  TIFF loader, by Malcolm McLean

  Meant to be a pretty comprehensive, one file, portable TIFF reader
  We just read everything in as 8 bit RGBs.

  Current limitations - no JPEG, no alpha, a few odd formats still
    unsupported
  No sophisticated colour handling

  Free for public use
  Acknowlegements, Lode Vandevenne for the Zlib decompressor
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#define TAG_BYTE 1
#define TAG_ASCII 2
#define TAG_SHORT 3
#define TAG_LONG 4
#define TAG_RATIONAL 5

/* data types
1 BYTE 8 - bit unsigned integer 

2 ASCII 8 - bit, NULL - terminated string

3 SHORT 16 - bit unsigned integer

4 LONG 32 - bit unsigned integer

5 RATIONAL Two 32 - bit unsigned integers


6 SBYTE 8 - bit signed integer

7 UNDEFINE 8 - bit byte

8 SSHORT 16 - bit signed integer 
9 SLONG 32 - bit signed integer 
10 SRATIONAL Two 32 - bit signed integers

11 FLOAT 4 - byte single - precision IEEE floating - point value

12 DOUBLE 8 - byte double - precision IEEE floating - point value

*/

#define COMPRESSION_NONE  1
#define COMPRESSION_CCITTRLE 2
#define COMPRESSION_CCITTFAX3 3
#define  COMPRESSION_CCITT_T4 3
#define COMPRESSION_CCITTFAX4 4
#define COMPRESSION_CCITT_T6  4
#define COMPRESSION_LZW 5
#define COMPRESSION_OJPEG  6
#define COMPRESSION_JPEG  7
#define COMPRESSION_NEXT  32766
#define COMPRESSION_CCITTRLEW 32771
#define COMPRESSION_PACKBITS 32773
#define COMPRESSION_THUNDERSCAN  32809
#define COMPRESSION_IT8CTPAD  32895
#define COMPRESSION_IT8LW  32896
#define COMPRESSION_IT8MP  32897
#define COMPRESSION_IT8BL  32898
#define COMPRESSION_PIXARFILM  32908
#define COMPRESSION_PIXARLOG  32909
#define COMPRESSION_DEFLATE  32946
#define COMPRESSION_ADOBE_DEFLATE  8
#define COMPRESSION_DCS  32947
#define COMPRESSION_JBIG  34661
#define COMPRESSION_SGILOG 34676
#define COMPRESSION_SGILOG24  34677
#define COMPRESSION_JP2000  34712

#define PI_WhiteIsZero 0
#define PI_BlackIsZero 1 
#define PI_RGB 2 
#define PI_RGB_Palette 3
#define PI_Tranparency_Mask 4
#define PI_CMYK 5
#define PI_YCbCr 6 
#define PI_CIELab 8

#define TID_IMAGEWIDTH 256
#define TID_IMAGEHEIGHT 257
#define TID_BITSPERSAMPLE 258
#define TID_COMPRESSION 259
#define TID_PHOTOMETRICINTERPRETATION 262
#define TID_FILLORDER 266
#define TID_STRIPOFFSETS 273
#define TID_SAMPLESPERPIXEL 277
#define TID_ROWSPERSTRIP 278
#define TID_STRIPBYTECOUNTS 279
#define TID_PLANARCONFIGUATION 284 
#define TID_T4OPTIONS 292
#define TID_PREDICTOR 317 
#define TID_COLORMAP 320


#define TID_TILEWIDTH 322
#define TID_TILELENGTH 323 
#define TID_TILEOFFSETS 324 
#define TID_TILEBYTECOUNTS 325
#define TID_SAMPLEFORMAT 339 
#define TID_SMINSAMPLEVALUE 340
#define TID_SMAXSAMPLEVALUE 341
#define TID_YCBCRCOEFFICIENTS 529
#define TID_YCBCRSUBSAMPLING 530 
#define TID_YCBCRPOSITIONING 531 

#define SAMPLEFORMAT_UINT 1
#define SAMPLEFORMAT_INT 2
#define SAMPLEFORMAT_IEEEFP 3
#define SAMPLEFORMAT_VOID 4
//#define SAMPLEFORMAT_COMPLEXINT  5
//#define SAMPLEFORMAT_COMPLEXIEEEFP 6
/* Artist 315 ASCII - * *

BadFaxLines[1] 326 SHORT or LONG - - -

BitsPerSample 258 SHORT * * *

CellLength 265 SHORT * * *

CellWidth 264 SHORT * * *

CleanFaxData[1] 327 SHORT - - -

ColorMap 320 SHORT - * *

ColorResponseCurve 301 SHORT * * x

ColorResponseUnit 300 SHORT * x x

Compression 259 SHORT * * *

Uncompressed 1 * * *

CCITT 1D 2 * * *

CCITT Group 3 3 * * *

CCITT Group 4 4 * * *

LZW 5 - * *

JPEG 6 - - *

Uncompressed 32771 * x x

Packbits 32773 * * *

ConsecutiveBadFaxLines[1] 328 LONG or SHORT - - -

Copyright 33432 ASCII - - *

DateTime 306 ASCII - * *

DocumentName 269 ASCII * * *

DotRange 336 BYTE or SHORT - - *

ExtraSamples 338 BYTE - - *

FillOrder 266 SHORT * * *

FreeByteCounts 289 LONG * * *

FreeOffsets 288 LONG * * *

GrayResponseCurve 291 SHORT * * *

GrayResponseUnit 290 SHORT * * *

HalftoneHints 321 SHORT - - *

HostComputer 316 ASCII - * *

ImageDescription 270 ASCII * * *

ImageHeight 257 SHORT or LONG * * *

ImageWidth 256 SHORT or LONG * * *

InkNames 333 ASCII - - *

InkSet 332 SHORT - - *

JPEGACTTables 521 LONG - - *

JPEGDCTTables 520 LONG - - *

JPEGInterchangeFormat 513 LONG - - *

JPEGInterchangeFormatLength 514 LONG - - *

JPEGLosslessPredictors 517 SHORT - - *

JPEGPointTransforms 518 SHORT - - *

JPEGProc 512 SHORT - - *

JPEGRestartInterval 515 SHORT - - *

JPEGQTables 519 LONG - - *

Make 271 ASCII * * *

MaxSampleValue 281 SHORT * * *

MinSampleValue 280 SHORT * * *

Model 272 ASCII * * *

NewSubFileType 254 LONG - * *

NumberOfInks 334 SHORT - - *

Orientation 274 SHORT * * *

PageName 285 ASCII * * *

PageNumber 297 SHORT * * *

PhotometricInterpretation 262 SHORT * * *

WhiteIsZero 0 * * *

BlackIsZero 1 * * * RGB 2 * * *

RGB Palette 3 - * *

Tranparency Mask 4 - - *

CMYK 5 - - *

YCbCr 6 - - *

CIELab 8 - - *

PlanarConfiguration 284 SHORT * * *

Predictor 317 SHORT - * *

PrimaryChromaticities 319 RATIONAL - * *

ReferenceBlackWhite 532 LONG - - *

ResolutionUnit 296 SHORT * * *

RowsPerStrip 278 SHORT or LONG * * *

SampleFormat 339 SHORT - - *

SamplesPerPixel 277 SHORT * * *

SMaxSampleValue 341 Any - - *

SMinSampleValue 340 Any - - *

Software 305 ASCII - * *

StripByteCounts 279 LONG or SHORT * * *

StripOffsets 273 SHORT or LONG * * *

SubFileType 255 SHORT * x x

T4Options[2] 292 LONG * * *

T6Options[3] 293 LONG * * *

TargetPrinter 337 ASCII - - *

Thresholding 263 SHORT * * *

TileByteCounts 325 SHORT or LONG - - *

TileLength 323 SHORT or LONG - - *

TileOffsets 324 LONG - - *

TileWidth 322 SHORT or LONG - - *

TransferFunction[4] 301 SHORT - - *

TransferRange 342 SHORT - - *

XPosition 286 RATIONAL * * *

XResolution 282 RATIONAL * * *

YCbCrCoefficients 529 RATIONAL - - *

YCbCrPositioning 531 SHORT - - *

YCbCrSubSampling 530 SHORT - - *

YPosition 287 RATIONAL * * *

YResolution 283 RATIONAL * * *

WhitePoint 318 RATIONAL - * *

*/


typedef struct basic_header
{
	unsigned long newsubfiletype;
	int imagewidth;
	int imageheight;
	int bitspersample[16];
	int compression;
	int fillorder;
	int photometricinterpretation;
	unsigned long *stripoffsets;
	int Nstripoffsets;
	int samplesperpixel;
	int rowsperstrip;
	unsigned long *stripbytecounts;
	int Nstripbytecounts;
	double xresolution;
	double yresolution;
	int resolutionunit;
/*  Palette files */
	unsigned char *colormap;
	int Ncolormap;
	/* RGB */
	int planarconfiguration;
	int predictor;
	/* YCbCr*/


	//double YCbCrCoefficients[3];
	double LumaRed;
	double LumaGreen;
	double LumaBlue;
	int YCbCrSubSampling_h;
	int YCbCrSubSampling_v;
	int YCbCrPositioning;
	int ReferenceBlackWhite;

/* Class F */

	int BadFaxLines;
	int CleanFaxData;
	int ConsecutiveBadFaxLines;
	unsigned long T4options;

	/* tiling */
	int tilewidth;
	int tileheight;
	unsigned long *tileoffsets;
	int Ntileoffsets;
	unsigned long *tilebytecounts;
	int Ntilebytecounts;
	/* Malcolm easier to support this now*/
	int sampleformat[16];
	double *smaxsamplevalue;
	int Nsmaxsamplevalue;
	double *sminsamplevalue;
	int Nsminsamplevalue;
	int endianness;
} BASICHEADER;

struct tifftag
{
	unsigned short tagid;
	unsigned short datatype;
	unsigned long datacount;
	unsigned long dataoffset;
};

typedef struct
{
	unsigned short tagid;
	unsigned short datatype;
	unsigned long datacount;
	double scalar;
	char *ascii;
	void *vector;
	int bad;
} TAG;

typedef struct
{
	unsigned char *data;
	int N;
	int pos;
	int bit;
	int endianness;
} BSTREAM;

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 1
#endif
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 2
#endif

static void header_defaults(BASICHEADER *header);
static void freeheader(BASICHEADER *header);
static int header_fixupsections(BASICHEADER *header);
static int header_not_ok(BASICHEADER *header);
static int fillheader(BASICHEADER *header, TAG *tags, int Ntags);

static unsigned char *decompress(FILE *fp, unsigned long count, int compression, unsigned long *Nret, int width, int height, unsigned long T4options );
static void header_defaults(BASICHEADER *header);
static TAG *floadheader(int type, FILE *fp, int *Ntags);
static void killtags(TAG *tags, int N);
static int floadtag(TAG *tag, int type, FILE *fp);
static double tag_getentry(TAG *tag, int index);

static unsigned char *loadraster(BASICHEADER *header, FILE *fp);
static unsigned char *readstrip(BASICHEADER *header, int index, int *strip_width, int *strip_height, FILE *fp);
static unsigned char *readtile(BASICHEADER *header, int index, int *tile_width, int *tile_height, FILE *fp);
static unsigned char *readchannel(BASICHEADER *header, int index, int *channel_width, int *channel_height, FILE *fp);

static BSTREAM *bstream(unsigned char *data, int N, int endinaness);
static void killbstream(BSTREAM *bs);
static int getbit(BSTREAM *bs);
static int getbits(BSTREAM *bs, int nbits);
static int synchtobyte(BSTREAM *bs);
static int writebit(BSTREAM *bs, int bit);

static void rgbapaste(unsigned char *rgba, int width, int height, unsigned char *tile, int twidth, int theight, int x, int y);

static long fget32(int type, FILE *fp);
static int fget16(int type, FILE *fp);
static long fget32be(FILE *fp);
static int fget16be(FILE *fp);
static long fget32le(FILE *fp);
static int fget16le(FILE *fp);
static unsigned long fget32u(int type, FILE *fp);
static unsigned int fget16u(int type, FILE *fp);
static char *freadasciiz(FILE *fp);

static double memreadieee754(unsigned char *buff, int bigendian);
static float memreadieee754f(unsigned char*buff, int bigendian);
static int YcbcrToRGB(int Y, int cb, int cr, unsigned char *red, unsigned char *green, unsigned char *blue);

unsigned char *floadtiff(FILE *fp, int *width, int *height)
{
	int enda, endb;
	int type;
	int magic;
	unsigned long offset;
	TAG *tags = 0;
	int Ntags = 0;
	int err;
	BASICHEADER header;
	unsigned char *answer;

	enda = fgetc(fp);
	endb = fgetc(fp);
	if (enda == 'I' && endb == 'I')
	{
		type = LITTLE_ENDIAN;
	}
	else if (endb == 'M' && endb == 'M')
	{
		type = BIG_ENDIAN;
	}
	else
		goto parse_error;
	magic = fget16(type, fp);
	if (magic != 42)
		goto parse_error;
	offset = fget32(type, fp);
	fseek(fp, offset, SEEK_SET);

	//printf("%c%c %d %ld\n", enda, endb, magic, offset);

	tags = floadheader(type, fp, &Ntags);
	if (!tags)
		goto out_of_memory;
	//getchar();
	header_defaults(&header);
	header.endianness = type;
	fillheader(&header, tags, Ntags);
	err = header_fixupsections(&header);
	if (err)
		goto parse_error;
	//printf("here %d %d\n", header.imagewidth, header.imageheight);
	//printf("Bitspersample%d %d %d\n", header.bitspersample[0], header.bitspersample[1], header.bitspersample[2]);
	err = header_not_ok(&header);
	if (err)
		goto parse_error;
	answer = loadraster(&header, fp);
	//getchar();
	*width = header.imagewidth;
	*height = header.imageheight;
	freeheader(&header);
	killtags(tags, Ntags);
	return answer;

parse_error:
	freeheader(&header);
	killtags(tags, Ntags);
	return 0;
out_of_memory:
	freeheader(&header);
	killtags(tags, Ntags);
	return 0;
}


static void header_defaults(BASICHEADER *header)
{
	int i;
	header->newsubfiletype = 0;
	header->imagewidth = -1;
	header->imageheight = -1;
	header->bitspersample[0] = -1;
	header->compression = 1;
	header->fillorder = 1;
	header->photometricinterpretation =-1;
	header->stripoffsets = 0;;
	header->Nstripoffsets = 0;
	header->samplesperpixel = 0;
	header->rowsperstrip = 0;
	header->stripbytecounts = 0;
	header->Nstripbytecounts = 0;
	header->xresolution = 1.0;
	header->yresolution = 1.0;
	header->resolutionunit = 0;
	/*  Palette files */
	header->colormap = 0;
	header->Ncolormap = 0;
	/* RGB */
	header->planarconfiguration =1;
	header->predictor = 1;
	/* YCbCr*/

	header->LumaRed = 0.299;
	header->LumaGreen = 0.587;
	header->LumaBlue = 0.114;
	//header->YCbCrCoefficients = 0;
	header->YCbCrSubSampling_h = 2;
	header->YCbCrSubSampling_v = 2;
	header->YCbCrPositioning = 1;
	header->ReferenceBlackWhite = 0;

	/* Class F */

	//int BadFaxLines;
	//int CleanFaxData;
	//int ConsecutiveBadFaxLines;
	header->T4options = 0;

	header->tilewidth = 0;
	header->tileheight = 0;
	header->tileoffsets = 0;
	header->Ntileoffsets = 0;
	header->tilebytecounts = 0;
	header->Ntilebytecounts = 0;

	for (i = 0; i < 16;i++)
		header->sampleformat[i] = 1;
	header->smaxsamplevalue = 0;
	header->Nsmaxsamplevalue = 0;
	header->sminsamplevalue = 0;
	header->Nsminsamplevalue = 0;
	header->endianness = -1;


}

static void freeheader(BASICHEADER *header)
{
	free(header->stripbytecounts);
	free(header->stripoffsets);
	free(header->tilebytecounts);
	free(header->tileoffsets);
	free(header->colormap);
	free(header->smaxsamplevalue);
	free(header->sminsamplevalue);
}
/*
  Some TIFF files have tiles in the strip byte counts and so on
  fixc this up
*/
static int header_fixupsections(BASICHEADER *header)
{
	if (header->tilewidth == 0 && header->tileheight == 0)
	{
		if (header->Nstripbytecounts > 0 &&
			header->Nstripoffsets > 0 &&
			header->Nstripbytecounts == header->Nstripoffsets
			&& header->Nstripbytecounts < INT_MAX
			&& header->Ntileoffsets == 0 &&
			header->Ntilebytecounts == 0)
			return 0;
		else
			return -1;
	}
	else if (header->tilewidth > 0 && header->tileheight > 0)
	{
		if (header->Ntilebytecounts == 0)
		{
			header->Ntilebytecounts = header->Nstripbytecounts;
			header->Nstripbytecounts = 0;
			header->tilebytecounts = header->stripbytecounts;
			header->stripbytecounts = 0;
		}
		if (header->Ntileoffsets == 0)
		{
			header->Ntileoffsets = header->Nstripoffsets;
			header->Nstripoffsets = 0;
			header->tileoffsets = header->stripoffsets;
			header->stripoffsets = 0;
		}
		if (header->Ntilebytecounts > 0 &&
			header->Ntileoffsets > 0 &&
			header->Ntilebytecounts == header->Ntileoffsets
			&& header->Ntilebytecounts < INT_MAX
			&& header->Nstripoffsets == 0 &&
			header->Nstripbytecounts == 0)
			return 0;
		else
			return -1;
	}
	else
	{
		return -1;
	}
	return -1;
}

static int header_not_ok(BASICHEADER *header)
{
	int i;
	//header->newsubfiletype = 0;
	//header->imagewidth = -1;
	//header->imageheight = -1;
	if (header->imagewidth <= 0 || header->imageheight <= 0
		|| (double)header->imagewidth * (double)header->imageheight > LONG_MAX / 4)
		goto parse_error;
	
	//header->bitspersample[0] = -1;
	if (header->samplesperpixel < 1 || header->samplesperpixel > 16)
		goto parse_error;
	for (i = 0; i < header->samplesperpixel; i++)
		if (header->bitspersample[i] < 1 || (header->bitspersample[i] > 32 && header->bitspersample[i] % 8))
			goto parse_error;
	//header->compression = 1;
	//header->fillorder = 1;
	//header->photometricinterpretation = -1;
	//header->stripoffsets = 0;;
	//header->Nstripoffsets = 0;
	//header->samplesperpixel = 0;
	//header->rowsperstrip;
	//header->stripbytecounts;
	//header->Nstripbytecounts;
	//header->xresolution = 1.0;
	//header->yresolution = 1.0;
	//header->resolutionunit = 0;
	/*  Palette files */
	//header->colormap = 0;
	//header->Ncolormap = 0;
	if (header->Ncolormap < 0 || header->Ncolormap > INT_MAX / 4)
		goto parse_error;
	/* RGB */
	//header->planarconfiguration = 1;
	if (header->planarconfiguration != 1 && header->planarconfiguration != 2)
		goto parse_error;
	/* YCbCr*/

	//header->LumaRed = 0.299;
	//header->LumaGreen = 0.587;
	//header->LumaBlue = 0.114;
	if (header->LumaRed <= 0 || header->LumaGreen <= 0 || header->LumaBlue <= 0)
		goto parse_error;
	//header->YCbCrCoefficients = 0;
	//header->YCbCrSubSampling_h = 2;
	//header->YCbCrSubSampling_v = 2;
	//header->YCbCrPositioning = 1;
	//header->ReferenceBlackWhite = 0;

	/* Class F */

	//int BadFaxLines;
	//int CleanFaxData;
	//int ConsecutiveBadFaxLines;

	//header->tilewidth = 0;
	//header->tileheight = 0;
	if (header->tilewidth < 0 || header->tileheight < 0)
		goto parse_error;
	if ((double)header->tilewidth * (double)header->tileheight > LONG_MAX / 4)
		goto parse_error;
	//header->tileoffsets = 0;
	//header->Ntileoffsets = 0;
	//header->tilebytecounts = 0;
	//header->Ntilebytecounts = 0;

	//header->sampleformat = 1;
	//header->smaxsamplevalue = 0;
	//header->Nsmaxsamplevalue = 0;
	//header->sminsamplevalue = 0;
	//header->Nsminsamplevalue = 0;
	//header->endianness = -1;
	return 0;
parse_error:
	return -1;

}

static int fillheader(BASICHEADER *header, TAG *tags, int Ntags)
{
	int i;
	unsigned long ii;
	int jj;

	for (i = 0; i < Ntags; i++)
	{
		if (tags[i].bad)
			continue;
		switch (tags[i].tagid)
		{
		case TID_IMAGEWIDTH:
			header->imagewidth = (int)tags[i].scalar;
			break;
		case TID_IMAGEHEIGHT:
			header->imageheight = (int)tags[i].scalar;
			break;
		case TID_BITSPERSAMPLE:
			if (tags[i].datacount > 16)
				goto parse_error;
			for (ii = 0; ii < tags[i].datacount; ii++)
					header->bitspersample[ii] = (int) tag_getentry(&tags[i], ii);
			break;
		case TID_COMPRESSION:
			header->compression = (int) tags[i].scalar;
			break;
		case TID_PHOTOMETRICINTERPRETATION:
			header->photometricinterpretation = (int)tags[i].scalar;
			break;
		case TID_FILLORDER:
			header->fillorder = (int)tags[i].scalar;
			break;
		case TID_STRIPOFFSETS:
			header->stripoffsets = malloc(tags[i].datacount * sizeof(unsigned long));
			if (!header->stripoffsets)
				goto out_of_memory;
			for (ii = 0; ii < tags[i].datacount; ii++)
				header->stripoffsets[ii] = (int) tag_getentry(&tags[i], ii);
			header->Nstripoffsets = tags[i].datacount;
			break;
		case TID_SAMPLESPERPIXEL:
			header->samplesperpixel = (int)tags[i].scalar;
			break;
		case TID_ROWSPERSTRIP:
			header->rowsperstrip = (int)tags[i].scalar;
			break;
		case TID_STRIPBYTECOUNTS:
			header->stripbytecounts = malloc(tags[i].datacount * sizeof(unsigned long));
			if (!header->stripbytecounts)
				goto out_of_memory;
			for (ii = 0; ii < tags[i].datacount; ii++)
				header->stripbytecounts[ii] = (unsigned long)tag_getentry(&tags[i], ii);
			header->Nstripbytecounts =  tags[i].datacount;
			break;
		case TID_PLANARCONFIGUATION:
			header->planarconfiguration = (int) tags[i].scalar;
			break;
		case TID_T4OPTIONS:
			header->T4options = (unsigned long)tags[i].scalar;
			break;
		case TID_PREDICTOR:
			header->predictor = (int)tags[i].scalar;
			break;
		case TID_COLORMAP:
			if (tags[i].datacount > INT_MAX / 3)
				goto parse_error;
			header->Ncolormap = tags[i].datacount / 3;
			header->colormap = malloc(header->Ncolormap * 3);
			if (!header->colormap)
				goto out_of_memory;
			for (jj = 0; jj < header->Ncolormap; jj++)
				header->colormap[jj *3] = (int)tag_getentry(&tags[i], jj) / 256;
			for (jj = 0; jj < header->Ncolormap; jj++)
				header->colormap[jj * 3+1] = (int)tag_getentry(&tags[i], jj + header->Ncolormap) / 256;
			for (jj = 0; jj < header->Ncolormap; jj++)
				header->colormap[jj * 3 + 2] = (int)tag_getentry(&tags[i], jj + header->Ncolormap*2) / 256;
			break;
		case TID_TILEWIDTH:
			header->tilewidth = (int) tags[i].scalar;
			break;
		case TID_TILELENGTH:
			header->tileheight = (int)tags[i].scalar;
			break;
		case TID_TILEOFFSETS:
			header->tileoffsets = malloc(tags[i].datacount * sizeof(unsigned long));
			if (!header->tileoffsets)
				goto out_of_memory;
			for (ii = 0; ii < tags[i].datacount; ii++)
				header->tileoffsets[ii] = (unsigned long)tag_getentry(&tags[i], ii);
			header->Ntileoffsets = tags[i].datacount;
			break;
		case TID_TILEBYTECOUNTS:
			header->tilebytecounts = malloc(tags[i].datacount * sizeof(unsigned long));
			if (!header->tilebytecounts)
				goto out_of_memory;
			for (ii = 0; ii < tags[i].datacount; ii++)
				header->tilebytecounts[ii] = (unsigned long)tag_getentry(&tags[i], ii);
			header->Ntilebytecounts = tags[i].datacount;
			break;
		case TID_SAMPLEFORMAT:
			if (tags[i].datacount != header->samplesperpixel)
				goto parse_error;
			for (ii = 0; ii < tags[i].datacount;ii++)
				header->sampleformat[ii] = (int)tag_getentry(&tags[i], ii);
			break;
		case TID_SMINSAMPLEVALUE:
			header->sminsamplevalue = malloc(tags[i].datacount * sizeof(double));
			if (!header->sminsamplevalue)
				goto out_of_memory;
			for (ii = 0; ii < tags[i].datacount; ii++)
				header->sminsamplevalue[ii] = (unsigned long)tag_getentry(&tags[i], ii);
			header->Nsminsamplevalue = tags[i].datacount;
			break;
		case TID_SMAXSAMPLEVALUE:
			header->smaxsamplevalue = malloc(tags[i].datacount * sizeof(double));
			if (!header->smaxsamplevalue)
				goto out_of_memory;
			for (ii = 0; ii < tags[i].datacount; ii++)
				header->smaxsamplevalue[ii] = (unsigned long)tag_getentry(&tags[i], ii);
			header->Nsmaxsamplevalue = tags[i].datacount;
			break;
		case TID_YCBCRCOEFFICIENTS:
			if (tags[i].datacount < 3)
				goto parse_error;
			header->LumaRed = tag_getentry(&tags[i], 0);
			header->LumaGreen = tag_getentry(&tags[i], 1);
			header->LumaBlue = tag_getentry(&tags[i], 2);
			break;
		case TID_YCBCRSUBSAMPLING:
			if (tags[i].datacount < 2)
				goto parse_error;
			header->YCbCrSubSampling_h = (int)tag_getentry(&tags[i], 0);
			header->YCbCrSubSampling_v = (int)tag_getentry(&tags[i], 1);
			break;
		case TID_YCBCRPOSITIONING:
			header->YCbCrPositioning = (int)tags[i].scalar;
			break;

		}
	}

	return 0;
out_of_memory:
	return -1;
parse_error:
	return -2;
}

static unsigned char *loadraster(BASICHEADER *header, FILE *fp)
{
	unsigned char *rgba = 0;
	unsigned char *strip = 0;
	int i;
	unsigned long ii;
	int row = 0;
	int swidth, sheight;
	int tilesacross = 0;
	int sample_index;

	rgba = malloc(header->imagewidth * header->imageheight * 4);
	if (!rgba)
		goto out_of_memory;
	if (header->tilewidth)
		tilesacross = (header->imagewidth + header->tilewidth - 1) / header->tilewidth;

	if (header->planarconfiguration == 2)
	{
		if (header->photometricinterpretation == PI_RGB)
		{
			sample_index = 0;
			for (i = 0; i < header->Nstripoffsets; i++)
			{
				if (sample_index > 4)
					continue;
				strip = readchannel(header, i, &swidth, &sheight, fp);
				if (!strip)
					goto out_of_memory;
				for (ii = 0; ii < (unsigned long) (swidth * sheight); ii++)
				{
					rgba[((row + ii / swidth)*header->imagewidth + (ii%swidth)) * 4 + sample_index] = strip[ii];
				}
			
				row += sheight;
				if (row >= header->imageheight)
				{
					row = 0;
					sample_index++;
				}
			
				free(strip);
			}
			return rgba;
		}
		if (header->photometricinterpretation == PI_CMYK)
		{
			sample_index = 0;
			for (i = 0; i < header->Nstripoffsets; i++)
			{
				if (sample_index > 4)
					continue;
				strip = readchannel(header, i, &swidth, &sheight, fp);
				if (!strip)
					goto out_of_memory;
				for (ii = 0; ii < (unsigned long)(swidth * sheight); ii++)
				{
					rgba[((row + ii / swidth)*header->imagewidth + (ii%swidth)) * 4 + sample_index] = strip[ii];
				}

				row += sheight;
				if (row >= header->imageheight)
				{
					row = 0;
					sample_index++;
				}

				free(strip);
			}

			for (i = 0; i < header->imagewidth * header->imageheight; i++)
			{
				int red, green, blue;

				red = ((255 - rgba[i*4])*(255 - rgba[i*4+3])) / 255;
				green = ((255 - rgba[i*4+1])*(255 - rgba[i*4+3])) / 255;
				blue = ((255 - rgba[i*4+2])*(255 - rgba[i*4+3])) / 255;
				rgba[i*4] = red;
				rgba[i*4+1] = green;
				rgba[i*4+2] = blue;
				rgba[i*4+3] = 255;
			}
			return rgba;
		}
	}

	if (header->Nstripoffsets > 0 && tilesacross == 0)
	{
		for (i = 0; i < header->Nstripoffsets; i++)
		{
			strip = readstrip(header, i, &swidth, &sheight, fp);
			if (!strip)
				goto out_of_memory;
			memcpy(rgba + row * header->imagewidth * 4, strip, swidth *sheight * 4);
			row += sheight;
			free(strip);
		}
	}
	if (header->Nstripoffsets > 0 && tilesacross != 0)
	{
		header->Ntileoffsets = header->Nstripoffsets;
		header->tileoffsets = header->stripoffsets;
		header->stripoffsets = 0;
		header->Nstripoffsets = 0;
	}
	if (header->Ntilebytecounts == 0 && tilesacross != 0)
	{
		header->Ntilebytecounts = header->Nstripoffsets;
		header->tilebytecounts = header->stripbytecounts;
		header->stripbytecounts = 0;
		header->Nstripbytecounts = 0;
	}
    if (header->Ntileoffsets > 0)
	{
		
		for (i = 0; i < header->Ntileoffsets; i++)
		{
			strip = readtile(header, i, &swidth, &sheight, fp);
			if (!strip)
				goto out_of_memory;
			rgbapaste(rgba, header->imagewidth, header->imageheight, strip, swidth, sheight,
				(i % tilesacross) * header->tilewidth, (i / tilesacross) * header->tileheight);
			free(strip);
		}
	}
	return rgba;

out_of_memory:
	free(rgba);
	free(strip);
	return 0;
}

/*//////////////////////////////////////////////////////////////////////////////////////////////////*/
/* stip tile and plane loading section*/
/*//////////////////////////////////////////////////////////////////////////////////////////////////*/

static int planetochannel(unsigned char *out, int width, int height, unsigned char *bits, unsigned long Nbytes, BASICHEADER *header, int sample_index);

static int greytorgba(unsigned char *rgba, int width, int height, unsigned char *bits, unsigned long N, BASICHEADER *header);
static int paltorgba(unsigned char *rgba, int width, int height, unsigned char *bits, unsigned long Nbytes, BASICHEADER *header);
static int ycbcrtorgba(unsigned char *rgba, int width, int height, unsigned char *bits, unsigned long N, BASICHEADER *header);
static int cmyktorgba(unsigned char *rgba, int width, int height, unsigned char *bits, unsigned long Nbytes, BASICHEADER *header);
static int bitstreamtorgba(unsigned char *rgba, int width, int height, unsigned char *bits, unsigned long Nbytes, BASICHEADER *header);


static void unpredictsamples(unsigned char *rgba, int width, int height, BASICHEADER *header);
static int readbytesample(unsigned char *bytes, BASICHEADER *header, int sample_index);
static int readintsample(unsigned char *bytes, BASICHEADER *header, int sample_index);

static unsigned char *readtile(BASICHEADER *header, int index, int *tile_width, int *tile_height, FILE *fp)
{
	unsigned char *data = 0;
	unsigned char *rgba = 0;
	unsigned long N;

	fseek(fp, header->tileoffsets[index], SEEK_SET);
	data = decompress(fp, header->tilebytecounts[index], header->compression, &N, header->tilewidth, header->tileheight, header->T4options);
	if (!data)
		goto out_of_memory;
	rgba = malloc(4 * header->tilewidth * header->tileheight);
	if (!rgba)
		goto out_of_memory;
	*tile_width = header->tilewidth;
	*tile_height = header->tileheight;

	switch (header->photometricinterpretation)
	{
	case PI_WhiteIsZero:
	case PI_BlackIsZero:
		greytorgba(rgba, header->tilewidth, header->tileheight, data, N, header);
		if (header->predictor == 2)
		{
			unpredictsamples(rgba, header->tilewidth, header->tileheight, header);
		}
		break;
	case PI_RGB:
		bitstreamtorgba(rgba, header->tilewidth, header->tileheight, data, N, header);
		if (header->predictor == 2)
		{
			unpredictsamples(rgba, header->tilewidth, header->tileheight, header);
		}
		break;
	case PI_RGB_Palette:
		paltorgba(rgba, header->tilewidth, header->tileheight, data, N, header);
		break;
	case PI_CMYK:
		cmyktorgba(rgba, header->tilewidth, header->tileheight, data, N, header);
		break;
	case PI_YCbCr:
		ycbcrtorgba(rgba, header->tilewidth, header->tileheight, data, N, header);
		break;
	}
	
	free(data);
	return rgba;
out_of_memory:
	free(data);
	free(rgba);
	return 0;


}

static unsigned char *readstrip(BASICHEADER *header, int index, int *strip_width, int *strip_height, FILE *fp)
{
	unsigned char *data = 0;
	unsigned char *rgba = 0;
	unsigned long N;
	int stripheight;

	fseek(fp, header->stripoffsets[index], SEEK_SET);
	if (index == header->Nstripoffsets - 1)
	{
		stripheight = header->imageheight - header->rowsperstrip *index;
	}
	else
		stripheight = header->rowsperstrip;
	data = decompress(fp, header->stripbytecounts[index], header->compression, &N, header->imagewidth, stripheight, header->T4options);
	if (!data)
		goto out_of_memory;
	
	rgba = malloc(4 * header->imagewidth * stripheight);
	if (!rgba)
		goto out_of_memory;
	*strip_width = header->imagewidth;
	*strip_height = stripheight;
	switch (header->photometricinterpretation)
	{
	case PI_WhiteIsZero:
	case PI_BlackIsZero:
		greytorgba(rgba, header->imagewidth, stripheight, data, N, header);
		if (header->predictor == 2)
		{
			unpredictsamples(rgba, header->imagewidth, stripheight, header);
		}
		break;
	case PI_RGB:
		bitstreamtorgba(rgba, header->imagewidth, stripheight, data, N, header);
		if (header->predictor == 2)
		{
			unpredictsamples(rgba, header->imagewidth, stripheight, header);
		}
		break;
	case PI_RGB_Palette:
		paltorgba(rgba, header->imagewidth, stripheight, data, N, header);
		break;
	case PI_CMYK:
		cmyktorgba(rgba, header->imagewidth, stripheight, data, N, header);
		break;
	case PI_YCbCr:
		ycbcrtorgba(rgba, header->imagewidth, stripheight, data, N, header);
		break;
	}
	
	free(data);
	return rgba;
out_of_memory:
	free(data);
	free(rgba);
	return 0;
}

static unsigned char *readchannel(BASICHEADER *header, int index, int *channel_width, int *channel_height, FILE *fp)
{
	unsigned char *data = 0;
	unsigned char *out = 0;
	unsigned long N;
	int stripheight;
	int stripsperimage = (header->imageheight + header->rowsperstrip - 1) / header->rowsperstrip;
	int sample_index;

	sample_index = index / stripsperimage;
	if (sample_index < 0 || sample_index >= header->samplesperpixel)
		return 0;

	fseek(fp, header->stripoffsets[index], SEEK_SET);
	if ((index % stripsperimage) == stripsperimage - 1)
	{
		stripheight = header->imageheight - header->rowsperstrip *(index%stripsperimage);
	}
	else
		stripheight = header->rowsperstrip;
	data = decompress(fp, header->stripbytecounts[index], header->compression, &N, header->imagewidth, stripheight, header->T4options);
	if (!data)
		goto out_of_memory;
	
	out = malloc(header->imagewidth * stripheight);
	if (!out)
		goto out_of_memory;
	*channel_width = header->imagewidth;
	*channel_height = stripheight;
	planetochannel(out, header->imagewidth, stripheight, data, N, header, index / stripsperimage);

	free(data);
	return out;
out_of_memory:
	free(data);
	free(out);
	return 0;
}

static int planetochannel(unsigned char *out, int width, int height, unsigned char *bits, unsigned long Nbytes, BASICHEADER *header, int sample_index)
{
	int bitstreamflag = 0;
	unsigned long i;
	int ii;
	int val;
	unsigned long counter = 0;

	if ((header->bitspersample[sample_index] % 8) != 0)
		bitstreamflag = 1;
	if (bitstreamflag == 0)
	{
		i = 0;

		while (i < Nbytes)
		{
			out[0] = readbytesample(bits, header, sample_index);
			if (header->photometricinterpretation == PI_WhiteIsZero)
				out[0] = 255 - out[0];
			bits += header->bitspersample[sample_index] / 8;
			i += header->bitspersample[sample_index] / 8;
			out++;
			if (counter++ > (unsigned long) (width * height) )
				return -1;
		}
	}
	else
	{
		BSTREAM *bs = bstream(bits, Nbytes, BIG_ENDIAN);
		for (i = 0; i < height; i++)
		{
			for (ii = 0; ii < width; ii++)
			{
				val = getbits(bs, header->bitspersample[sample_index]);
				val = (val * 255) / ((1 << header->bitspersample[sample_index]) - 1);
				*out++ = val;
			}
			synchtobyte(bs);
		}
		killbstream(bs);
		return 0;
	}
	return 0;
}

static int greytorgba(unsigned char *rgba, int width, int height, unsigned char *bits, unsigned long Nbytes, BASICHEADER *header)
{

	unsigned long i;
	int ii, iii;
	int totbits = 0;
	int bitstreamflag = 0;
	unsigned long counter = 0;

	for (i = 0; i < header->samplesperpixel; i++)
		totbits += header->bitspersample[i];
	for (i = 0; i < header->samplesperpixel; i++)
		if ((header->bitspersample[i] % 8) != 0)
			bitstreamflag = 1;

	if (bitstreamflag == 0)
	{
		i = 0;

		while (i < Nbytes - totbits/8 + 1)
		{
			rgba[0] = readbytesample(bits, header, 0);
			if (header->photometricinterpretation == PI_WhiteIsZero)
				rgba[0] = 255 - rgba[0];
			bits += header->bitspersample[0] / 8;
			i += header->bitspersample[0] / 8;
			rgba[1] = rgba[0];
			rgba[2] = rgba[1];

			for (ii = 1; ii < header->samplesperpixel; ii++)
			{
				bits += header->bitspersample[ii] / 8;
				i += header->bitspersample[ii] / 8;
			}
			rgba += 4;

			if (counter++ > width * height)
				break;
		}

		return 0;
	}
	else
	{
		BSTREAM *bs = bstream(bits, Nbytes, BIG_ENDIAN);
		for (i = 0; i < height; i++)
		{
			for (ii = 0; ii < width; ii++)
			{
				int val = getbits(bs, header->bitspersample[0]);
				rgba[0] = (val * 255) / ((1 << (header->bitspersample[0])) -1);
				rgba[1] = rgba[0];
				rgba[2] = rgba[0];
				for (iii = 1; iii < header->samplesperpixel; iii++)
				{
					getbits(bs, header->bitspersample[iii]);
				}
				rgba += 4;
			}
			synchtobyte(bs);
		}

		return 0;
	}
	
}

static int paltorgba(unsigned char *rgba, int width, int height, unsigned char *bits, unsigned long Nbytes, BASICHEADER *header)
{
	unsigned long i;
	unsigned long counter = 0;
	int index;
	int ii, iii;
	int totbits = 0;
	int bitstreamflag = 0;


	for (i = 0; i < header->samplesperpixel; i++)
		totbits += header->bitspersample[i];
	for (i = 0; i < header->samplesperpixel; i++)
		if ((header->bitspersample[i] % 8) != 0)
			bitstreamflag = 1;

	if (bitstreamflag == 0)
	{
		i = 0;

		while (i < Nbytes -totbits/8 + 1)
		{
			index = readintsample(bits, header, 0);
			bits += header->bitspersample[0] / 8;
			i += header->bitspersample[0] / 8;

			if (index >= 0 && index < header->Ncolormap)
			{
				rgba[0] = header->colormap[index * 3];
				rgba[1] = header->colormap[index * 3 + 1];
				rgba[2] = header->colormap[index * 3 + 2];
			}

			for (ii = 1; ii < header->samplesperpixel; ii++)
			{
				bits += header->bitspersample[ii] / 8;
				i += header->bitspersample[ii] / 8;
			}
			rgba += 4;
			counter++;
			if (counter > (unsigned long) (width * height) )
			{
				break;
			}
		}

		return 0;
	}
	else
	{
		BSTREAM *bs = bstream(bits, Nbytes, BIG_ENDIAN);
		for (i = 0; i < height; i++)
		{
			for (ii = 0; ii < width; ii++)
			{
				index = getbits(bs, header->bitspersample[0]);
				if (index >= 0 && index < header->Ncolormap)
				{
					rgba[0] = header->colormap[index * 3];
					rgba[1] = header->colormap[index * 3 + 1];
				    rgba[2] = header->colormap[index * 3 + 2];
				}
				for (iii = 1; iii < header->samplesperpixel; iii++)
				{
					getbits(bs, header->bitspersample[iii]);
				}
				rgba += 4;
			}
			synchtobyte(bs);
		}
		killbstream(bs);
		return 0;
	}


	
	return -1;
}

static int ycbcrtorgba(unsigned char *rgba, int width, int height, unsigned char *bits, unsigned long Nbytes, BASICHEADER *header)
{
	unsigned long i;
	int ii;
	int totbits = 0;
	int bitstreamflag = 0;
	int Y[16];
	int Cb, Cr;
	int x, y;
	int ix, iy;
	unsigned long counter = 0;

	if (header->YCbCrSubSampling_h * header->YCbCrSubSampling_v > 16)
		goto parse_error;
	if (header->LumaGreen == 0.0)
		header->LumaGreen = 1.0;

	for (i = 0; i < header->samplesperpixel; i++)
		totbits += header->bitspersample[i];
	for (i = 0; i < header->samplesperpixel; i++)
		if ((header->bitspersample[i] % 8) != 0)
			bitstreamflag = 1;

	x = 0; y = 0;
	// Need for some YcbCr images -
	//bits += 3;
	if (bitstreamflag == 0)
	{
		i = 0;

		while (i < Nbytes - totbits/8 +1)
		{
			for (ii = 0; ii < header->YCbCrSubSampling_h * header->YCbCrSubSampling_v; ii++)
			{
				Y[ii] = readbytesample(bits, header, 0);
				bits += header->bitspersample[0] / 8;
				i += header->bitspersample[0] / 8;
			}
			Cb = readbytesample(bits, header, 1);
			bits += header->bitspersample[1] / 8;
			i += header->bitspersample[1] / 8;
			Cr = readbytesample(bits, header, 2);
			bits += header->bitspersample[2] / 8;
			i += header->bitspersample[2] / 8;
			
			
			for (ii = 0; ii < header->YCbCrSubSampling_h * header->YCbCrSubSampling_v; ii++)
			{
				int red, green, blue;
				red = (int) ((Cr - 127 )*(2 - 2 * header->LumaRed) + Y[ii]);
				blue = (int) ((Cb -127) * (2 - 2 * header->LumaBlue) + Y[ii]);
				green = (int) ((Y[ii] - header->LumaBlue * blue - header->LumaRed * red) / header->LumaGreen);

				red = red < 0 ? 0 : red > 255 ? 255 : red;
				green = green < 0 ? 0 : green > 255 ? 255 : green;
				blue = blue < 0 ? 0 : blue > 255 ? 255 : blue;
				ix = x + (ii % header->YCbCrSubSampling_h);
				iy = y + (ii / header->YCbCrSubSampling_h);
				//YcbcrToRGB(Y[ii], Cb, Cr, &r, &g, &b);
				if (ix < width && iy < height)
				{
					rgba[(iy * width + ix) * 4] = red;
					rgba[(iy * width + ix) * 4 + 1] = green;
					rgba[(iy * width + ix) * 4 + 2] = blue;
				}

			}
			x += header->YCbCrSubSampling_h;
			if (x >= width)
			{
				x = 0;
				y += header->YCbCrSubSampling_v;
			}
			for (ii = 3; ii < header->samplesperpixel; ii++)
			{
		        bits += header->bitspersample[ii] / 8;
				i += header->bitspersample[ii] / 8;
			}
		}

		return 0;
	}

parse_error:
	return -2;
}

static int cmyktorgba(unsigned char *rgba, int width, int height, unsigned char *bits, unsigned long Nbytes, BASICHEADER *header)
{
	unsigned long i;
	int ii;
	int totbits = 0;
	int bitstreamflag = 0;
	int C, M, Y, K;
	int Cprev = 0, Yprev = 0, Mprev = 0, Kprev = 0;
	int red, green, blue;
	int x, y;
	unsigned long counter = 0;


	for (i = 0; i < header->samplesperpixel; i++)
		totbits += header->bitspersample[i];
	for (i = 0; i < header->samplesperpixel; i++)
		if ((header->bitspersample[i] % 8) != 0)
			bitstreamflag = 1;

	x = 0; y = 0;

	if (bitstreamflag == 0)
	{
		i = 0;

		while (i < Nbytes - totbits/8 +1)
		{
			C = readbytesample(bits, header, 0);
			bits += header->bitspersample[0] / 8;
			i += header->bitspersample[0] / 8;
			M = readbytesample(bits, header, 1);
			bits += header->bitspersample[1] / 8;
			i += header->bitspersample[1] / 8;
			Y = readbytesample(bits, header, 2);
			bits += header->bitspersample[2] / 8;
			i += header->bitspersample[2] / 8;
			K = readbytesample(bits, header, 3);
			bits += header->bitspersample[3] / 8;
			i += header->bitspersample[3] / 8;
			
			if (header->predictor == 2)
			{
				C = (C + Cprev) & 0xFF;
				M = (M + Mprev) & 0xFF;
				Y = (Y + Yprev) & 0xFF;
				K = (K + Kprev) & 0xFF;
				Cprev = C;
				Mprev = M;
				Yprev = Y;
				Kprev = K;
			}
				
			red = (255 * (255 - C) * (255 - K))/(255 * 255);
			green = (255 * (255 - M) * (255 - K))/(255 * 255);
			blue = (255 * (255 - Y) * (255 - K))/(255 * 255);

			rgba[0] = red;
			rgba[1] = green;
			rgba[2] = blue;
			
		    rgba += 4;
			
			for (ii = 4; ii < header->samplesperpixel; ii++)
			{
				bits += header->bitspersample[ii] / 8;
				i += header->bitspersample[ii] / 8;
			}

			x++;
			if (x == width)
			{
				x = 0;
				y++;
				Cprev = 0;
				Mprev = 0;
				Yprev = 0;
				Kprev = 0;
			}
			if (counter++ > width * height)
				goto parse_error;
		}

		return 0;
	}
	else
	{
		goto parse_error;
	}
	return 0;

parse_error:
	return -2;
}

/*
  
*/
static int bitstreamtorgba(unsigned char *rgba, int width, int height, unsigned char *bits, unsigned long Nbytes, BASICHEADER *header)
{
	unsigned long i;
	int ii, iii;
	int totbits = 0;
	int bitstreamflag = 0;
	int red, green, blue;
	unsigned long counter = 0;

	for (i = 0; i < header->samplesperpixel; i++)
		totbits += header->bitspersample[i];
	for (i = 0; i < header->samplesperpixel; i++)
		if ((header->bitspersample[i] % 8) != 0)
			bitstreamflag = 1;

	if (bitstreamflag == 0)
	{
		i = 0;

		while (i < Nbytes -totbits/8 +1)
		{
			rgba[0] = readbytesample(bits, header, 0);
			bits += header->bitspersample[0] / 8;
			i += header->bitspersample[0] / 8;
			rgba[1] = readbytesample(bits, header, 1);
			bits += header->bitspersample[1] / 8;
			i += header->bitspersample[1] / 8;
			rgba[2] = readbytesample(bits, header, 2);
			bits += header->bitspersample[2] / 8;
			i += header->bitspersample[2] / 8;

			for (ii = 3; ii < header->samplesperpixel; ii++)
			{
				bits += header->bitspersample[ii] / 8;
				i += header->bitspersample[ii] / 8;
			}
			rgba += 4;
		}

		return 0;
	}
	else
	{
		BSTREAM *bs = bstream(bits, Nbytes, BIG_ENDIAN);
		for (i = 0; i < height; i++)
		{
			for (ii = 0; ii < width; ii++)
			{
				red = getbits(bs, header->bitspersample[0]);
				red = (red * 255) / ((1 << (header->bitspersample[0])) - 1);
				green = getbits(bs, header->bitspersample[1]);
				green = (green * 255) / ((1 << (header->bitspersample[1])) - 1);
				blue = getbits(bs, header->bitspersample[2]);
				blue = (blue * 255) / ((1 << (header->bitspersample[2])) - 1);
				rgba[0] = red;
				rgba[1] = green;
				rgba[2] = blue;
				for (iii = 3; iii < header->samplesperpixel; iii++)
				{
					getbits(bs, header->bitspersample[iii]);
				}
				rgba += 4;
			}
			synchtobyte(bs);
		}
		killbstream(bs);
	}

	return 0;
}

/*
  read a sample that is an integer
    Called for colour-index modes
*/
static int readintsample(unsigned char *bytes, BASICHEADER *header, int sample_index)
{
	int i;
	int answer = 0;
	if (header->sampleformat[sample_index] == SAMPLEFORMAT_UINT)
	{
		if (header->endianness == BIG_ENDIAN)
		{
			for (i = 0; i < header->bitspersample[sample_index]/8; i++)
			{
				answer <<= 8;
				answer |= bytes[i];
			}
		}
		else
		{
			for (i = 0; i < header->bitspersample[sample_index]/8; i++)
			{
				answer |= bytes[i] << (i*8);
			}
		}
	}
	else
	{
		// suppress errors by returning 0
		return 0;
	}

	return answer;
}

/*
  read a sample froma  byte stream ( as tream where all the filedsa re whole byte multiples)
  Returns: sample in range 0 - 255
*/
static int readbytesample(unsigned char *bytes, BASICHEADER *header, int sample_index)
{
	int answer = -1;
	double real;
	double low, high;
	static double highest = 0.0;

	if (header->sampleformat[sample_index] == SAMPLEFORMAT_UINT)
	{
		if (header->endianness == BIG_ENDIAN)
		{
			answer = bytes[0];
		}
		else
		{
			answer = bytes[header->bitspersample[sample_index] / 8 -1];
		}
	}
	else if (header->sampleformat[sample_index] == SAMPLEFORMAT_IEEEFP)
	{
		if (header->bitspersample[sample_index] == 64)
			real = memreadieee754(bytes, header->endianness == BIG_ENDIAN ? 1 : 0);
		else if (header->bitspersample[sample_index] == 32)
			real = memreadieee754f(bytes, header->endianness == BIG_ENDIAN ? 1 : 0);
		if (header->sminsamplevalue)
			low = header->smaxsamplevalue[sample_index];
		else
			low = 0;

		if (header->smaxsamplevalue)
			high = header->smaxsamplevalue[sample_index];
		else
			high = 512.0;
		if (highest < real)
		{
			highest = real;
		}
		return (int)((real - low) * 255.0 / (high - low));
	}

	return answer;
}

static void unpredictsamples(unsigned char *rgba, int width, int height, BASICHEADER *header)
{
	int i, ii;

	for (i = 0; i < height; i++)
	{
		for (ii = 1; ii < width; ii++)
		{
			rgba[(i*width + ii) * 4] += rgba[(i*width + ii - 1) * 4];
			rgba[(i*width + ii) * 4+1] += rgba[(i*width + ii - 1) * 4+1];
			rgba[(i*width + ii) * 4+2] += rgba[(i*width + ii - 1) * 4+2];
		}
	}
}

/*///////////////////////////////////////////////////////////////////////////////////////*/
/* data decompression section */
/*///////////////////////////////////////////////////////////////////////////////////////*/
typedef struct LodePNGDecompressSettings
{
	unsigned ignore_adler32; /*if 1, continue and don't give an error message if the Adler32 checksum is corrupted*/
	unsigned custom_decoder; /*use custom decoder if LODEPNG_CUSTOM_ZLIB_DECODER and LODEPNG_COMPILE_ZLIB are enabled*/
} LodePNGDecompressSettings;

static void invert(unsigned char *bits, unsigned long N);
static unsigned char *unpackbits(FILE *fp, unsigned long count, unsigned long *Nret);
static unsigned char *ccittdecompress(unsigned char *in, unsigned long count, unsigned long *Nret, int width, int height, int eol);
static unsigned char *ccittgroup4decompress(unsigned char *in, unsigned long count, unsigned long *Nret, int width, int height, int eol);
static int loadlzw(unsigned char *out, FILE *fp, unsigned long count, unsigned long *Nret);
static unsigned lodepng_zlib_decompress(unsigned char** out, size_t* outsize, const unsigned char* in,
	size_t insize, const LodePNGDecompressSettings* settings);

/*
  Master decompression function
  Params:
    fp - input file, pointing to start of data section
	count - number of bytes in stream to decompress
	Nret - return for number of decompressed bytes
	width, height - width and height of strip or tile
	T4option - T4 twiddle
  Returns: pointer to decompressed dta, 0 on fail

*/
static unsigned char *decompress(FILE *fp, unsigned long count, int compression, unsigned long *Nret, int width, int height, unsigned long T4options)
{
	unsigned char *answer = 0;
	if (compression == 1)
	{
		answer = malloc(count);
		if (!answer)
			goto out_of_memory;
		fread(answer, 1, count, fp);
		*Nret = count;
		return answer;
	}
	else if (compression == COMPRESSION_CCITTRLE)
	{
		unsigned char *buff = malloc(count);
		unsigned long i;
		if (!buff)
			goto out_of_memory;
		fread(buff, 1, count, fp);
		answer = ccittdecompress(buff, count, Nret, width, height, 0);
		if (answer)
			invert(answer, *Nret);
		free(buff);
		return answer;
	}
	else if (compression == COMPRESSION_CCITTFAX3)
	{
		unsigned char *buff = malloc(count);
		if (!buff)
			goto out_of_memory;
		fread(buff, 1, count, fp);
		if ((T4options & 0x04) == 0)
			answer = ccittdecompress(buff, count, Nret, width, height, 1);
		else
			answer = 0; /* not handling for now */
		if (answer)
			invert(answer, *Nret);
		free(buff);
		return answer;
	}
	else if (compression == COMPRESSION_CCITTFAX4)
	{
		unsigned char *buff = malloc(count);
		if (!buff)
			goto out_of_memory;
		fread(buff, 1, count, fp);
		answer = ccittgroup4decompress(buff, count, Nret, width, height, 0);
		if (answer)
			invert(answer, *Nret);
		free(buff);
		return answer;
	}
	else if (compression == COMPRESSION_PACKBITS)
	{
		answer = unpackbits(fp, count, Nret);
		return answer;
	}
	else if (compression == COMPRESSION_LZW)
	{
		unsigned long pos = ftell(fp);
		int err;
		err = loadlzw(0, fp, count, Nret);
		answer = malloc(*Nret);
		if (!answer)
			goto out_of_memory;
		fseek(fp, pos, SEEK_SET);
		loadlzw(answer, fp, count, Nret);
		return answer;
	}
	else if (compression == COMPRESSION_ADOBE_DEFLATE || compression == COMPRESSION_DEFLATE)
	{
		LodePNGDecompressSettings settings;
		settings.custom_decoder = 0;
		settings.ignore_adler32 = 0;
		unsigned char *buff = malloc(count);
		if (!buff)
			goto out_of_memory;
		fread(buff, 1, count, fp);
		*Nret = 0;
		answer = 0;
		lodepng_zlib_decompress(&answer, Nret, buff, count, &settings);
		free(buff);
		return answer;
	}
	return 0;

out_of_memory:
	return 0;
}

static void invert(unsigned char *bits, unsigned long N)
{
	unsigned long i;

	for (i = 0; i < N; i++)
		bits[i] ^= 0xFF;
}

/*
  unpackbits decompressor. 
  Nice and easy compression scheme
*/
static unsigned char *unpackbits(FILE *fp, unsigned long count, unsigned long *Nret)
{
	unsigned long pos = ftell(fp);
	unsigned long N = 0;
	unsigned long i, j;
	long Nleft;
	signed char header;
	int ch;
	unsigned char *answer = 0;

	Nleft = count;
	while (Nleft > 0)
	{
		if ((ch = fgetc(fp)) == EOF)
			goto parse_error;
		Nleft--;
		header = (signed char) ch;
		if (header > 0)
		{
			N += header + 1;
			for (i = 0; i < header +1; i++)
			{
				fgetc(fp);
				Nleft--;
			}
		}
		else if (header > -128)
		{
			N += 1 - header;
			fgetc(fp);
			Nleft--;
		}
		else
		{
		}
	}
	fseek(fp, pos, SEEK_SET);
	Nleft = count;
	answer = malloc(N);
	if (!answer)
		goto out_of_memory;
	j = 0;
	while (Nleft > 0)
	{
		if ((ch = fgetc(fp)) == EOF)
			goto parse_error;
		Nleft--;
		header = (signed char) ch;
		if (header > 0)
		{
			for (i = 0; i < header +1; i++)
			{
				answer[j++] = fgetc(fp);
				Nleft--;
			}
		}
		else if (header > -128)
		{
			ch  = fgetc(fp);
			Nleft--;
			for (i = 0; i < 1 - header; i++)
				answer[j++] = ch;
		}
		else
		{
		}

	}

	*Nret = N;
	return answer;
parse_error:
out_of_memory:
	free(answer);
    *Nret = 0;
	return 0;
}



typedef struct
{
	int code;
	int prefix;
	int suffix;
	int len;
	unsigned char *ptr;
} ENTRY;


typedef struct huffnode
{
	struct huffnode *zero;
	struct huffnode *one;
	int symbol;
} HUFFNODE;

static HUFFNODE *addhuffmansymbol(HUFFNODE *root, const char *str, int symbol, int *err);
static void killhuffmantree(HUFFNODE *root);


/*
  add a symbol to the tree
  root - original tree (start off with null)
         str - the code, to add, encoded in ascii
		 symbol - associated symbol
		 err - error return sticky, 0 = success
  Returns: new tree (meaningful internally, after the first external call will
           always return the root)
   
*/
static HUFFNODE *addhuffmansymbol(HUFFNODE *root, const char *str, int symbol, int *err)
{
	HUFFNODE *answer;
	if (root)
	{
		if (str[0] == '0')
			root->zero = addhuffmansymbol(root->zero, str + 1, symbol, err);
		else if (str[0] == '1')
			root->one = addhuffmansymbol(root->one, str + 1, symbol, err);
		else if (str[0] == 0)
			root->symbol = symbol;

		return root;
	}
	else
	{
		answer = malloc(sizeof(HUFFNODE));
		if (!answer)
		{
			*err = -1;
			return 0;
		}
		answer->zero = 0;
		answer->one = 0;
		answer->symbol = -1;
		return addhuffmansymbol(answer, str, symbol, err);
	}
	return 0;
}

/*
  Huffman tree destructor
*/
static void killhuffmantree(HUFFNODE *root)
{
	if (root)
	{
		killhuffmantree(root->zero);
		killhuffmantree(root->one);
		free(root);
	}
}

/*
static void debughufftree(HUFFNODE *root, int N)
{
	int i;
		for (i = 0; i < N * 2; i++)
			printf(" ");
		printf("%d %p %p\n", root->symbol, root->zero, root->one);
		if (root->zero)
			debug(root->zero, N + 1);
		if (root->one)
			debug(root->one, N+1);

}
*/

static int gethuffmansymbol(HUFFNODE *root, BSTREAM *bs)
{
	int bit;

	if (root->zero == 0 && root->one == 0)
	{
		//printf("\n");
		return root->symbol;
	}
	bit = getbit(bs);
	//printf("%d", bit);
	if (bit == 0 && root->zero)
		return gethuffmansymbol(root->zero, bs);
	else if (bit == 1 && root->one)
		return gethuffmansymbol(root->one, bs);
	else
		return root->symbol;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////////*/
/*   CCITT decoding section*/
/*///////////////////////////////////////////////////////////////////////////////////////////////////*/
#define CCITT_PASS 101
#define CCITT_HORIZONTAL 102
#define CCITT_VERTICAL_0 103
#define CCITT_VERTICAL_R1 104
#define CCITT_VERTICAL_R2 105
#define CCITT_VERTICAL_R3 106
#define CCITT_VERTICAL_L1 107
#define CCITT_VERTICAL_L2 108
#define CCITT_VERTICAL_L3 109
#define CCITT_EXTENSION 110
#define CCITT_ENDOFFAXBLOCK 110

struct ccitt2dcode { int symbol; const char *code; };

static struct ccitt2dcode ccitt2dtable[11] =
{
	{ CCITT_PASS, "0001" },
	{ CCITT_HORIZONTAL, "001" },
	{ CCITT_VERTICAL_0, "1" },
	{ CCITT_VERTICAL_R1, "011" },
	{ CCITT_VERTICAL_R2, "000011" },
	{ CCITT_VERTICAL_R3, "0000011" },
	{ CCITT_VERTICAL_L1, "010" },
	{ CCITT_VERTICAL_L2, "000010" },
	{ CCITT_VERTICAL_L3, "0000010" },
	{ CCITT_EXTENSION, "0000001" },
    { CCITT_ENDOFFAXBLOCK, "000000000001"}
};

struct ccittcode { int whitelen; const char *whitecode; int blacklen; const char *blackcode; };
#define EOL -2
static struct ccittcode ccitttable[105] =
{
	{ 0, "00110101", 0, "0000110111" },
	{ 1, "000111", 1, "010" },
	{ 2, "0111", 2, "11" },
	{ 3, "1000", 3, "10" },
	{ 4, "1011", 4, "011" },
	{ 5, "1100", 5, "0011" },
	{ 6, "1110", 6, "0010" },
	{ 7, "1111", 7, "00011" },
	{ 8, "10011", 8, "000101" },
	{ 9, "10100", 9, "000100" },
	{ 10, "00111", 10, "0000100" },
	{ 11, "01000", 11, "0000101" },
	{ 12, "001000", 12, "0000111" },
	{ 13, "000011", 13, "00000100" },
	{ 14, "110100", 14, "00000111" },
	{ 15, "110101", 15, "000011000" },
	{ 16, "101010", 16, "0000010111" },
	{ 17, "101011", 17, "0000011000" },
	{ 18, "0100111", 18, "0000001000" },
	{ 19, "0001100", 19, "00001100111" },
	{ 20, "0001000", 20, "00001101000" },
	{ 21, "0010111", 21, "00001101100" },
	{ 22, "0000011", 22, "00000110111" },
	{ 23, "0000100", 23, "00000101000" },
	{ 24, "0101000", 24, "00000010111" },
	{ 25, "0101011", 25, "00000011000" },
	{ 26, "0010011", 26, "000011001010" },
	{ 27, "0100100", 27, "000011001011" },
	{ 28, "0011000", 28, "000011001100" },
	{ 29, "00000010", 29, "000011001101" },
	{ 30, "00000011", 30, "000001101000" },
	{ 31, "00011010", 31, "000001101001" },
	{ 32, "00011011", 32, "000001101010" },
	{ 33, "00010010", 33, "000001101011" },
	{ 34, "00010011", 34, "000011010010" },
	{ 35, "00010100", 35, "000011010011" },
	{ 36, "00010101", 36, "000011010100" },
	{ 37, "00010110", 37, "000011010101" },
	{ 38, "00010111", 38, "000011010110" },
	{ 39, "00101000", 39, "000011010111" },
	{ 40, "00101001", 40, "000001101100" },
	{ 41, "00101010", 41, "000001101101" },
	{ 42, "00101011", 42, "000011011010" },
	{ 43, "00101100", 43, "000011011011" },
	{ 44, "00101101", 44, "000001010100" },
	{ 45, "00000100", 45, "000001010101" },
	{ 46, "00000101", 46, "000001010110" },
	{ 47, "00001010", 47, "000001010111" },
	{ 48, "00001011", 48, "000001100100" },
	{ 49, "01010010", 49, "000001100101" },
	{ 50, "01010011", 50, "000001010010" },
	{ 51, "01010100", 51, "000001010011" },
	{ 52, "01010101", 52, "000000100100" },
	{ 53, "00100100", 53, "000000110111" },
	{ 54, "00100101", 54, "000000111000" },
	{ 55, "01011000", 55, "000000100111" },
	{ 56, "01011001", 56, "000000101000" },
	{ 57, "01011010", 57, "000001011000" },
	{ 58, "01011011", 58, "000001011001" },
	{ 59, "01001010", 59, "000000101011" },
	{ 60, "01001011", 60, "000000101100" },
	{ 61, "00110010", 61, "000001011010" },
	{ 62, "00110011", 62, "000001100110" },
	{ 63, "00110100", 63, "000001100111" },
	{ 64, "11011", 64, "0000001111" },
	{ 128, "10010", 128, "000011001000" },
	{ 192, "010111", 192, "000011001001" },
	{ 256, "0110111", 256, "000001011011" },
	{ 320, "00110110", 320, "000000110011" },
	{ 384, "00110111", 384, "000000110100" },
	{ 448, "01100100", 448, "000000110101" },
	{ 512, "01100101", 512, "0000001101100" },
	{ 576, "01101000", 576, "0000001101101" },
	{ 640, "01100111", 640, "0000001001010" },
	{ 704, "011001100", 704, "0000001001011" },
	{ 768, "011001101", 768, "0000001001100" },
	{ 832, "011010010", 832, "0000001001101" },
	{ 896, "011010011", 896, "0000001110010" },
	{ 960, "011010100", 960, "0000001110011" },
	{ 1024, "011010101", 1024, "0000001110100" },
	{ 1088, "011010110", 1088, "0000001110101" },
	{ 1152, "011010111", 1152, "0000001110110" },
	{ 1216, "011011000", 1216, "0000001110111" },
	{ 1280, "011011001", 1280, "0000001010010" },
	{ 1344, "011011010", 1344, "0000001010011" },
	{ 1408, "011011011", 1408, "0000001010100" },
	{ 1472, "010011000", 1472, "0000001010101" },
	{ 1536, "010011001", 1536, "0000001011010" },
	{ 1600, "010011010", 1600, "0000001011011" },
	{ 1664, "011000", 1664, "0000001100100" },
	{ 1728, "010011011", 1728, "0000001100101" },
	{ EOL, "000000000001", EOL, "00000000000" },
	{ 1792, "00000001000", 1792, "00000001000" },
	{ 1856, "00000001100", 1856, "00000001100" },
	{ 1920, "00000001101", 1920, "00000001101" },
	{ 1984, "000000010010", 1984, "000000010010" },
	{ 2048, "000000010011", 2048, "000000010011" },
	{ 2112, "000000010100", 2112, "000000010100" },
	{ 2176, "000000010101", 2176, "000000010101" },
	{ 2240, "00000001011", 2240, "000000010110" },
	{ 2304, "000000010111", 2304, "000000010111" },
	{ 2368, "000000011100", 2368, "000000011100" },
	{ 2432, "000000011101", 2432, "000000011101" },
	{ 2496, "000000011110", 2496, "000000011110" },
	{ 2560, "000000011111", 2560, "000000011111" },
};

static unsigned char *ccittdecompress(unsigned char *in, unsigned long count, unsigned long *Nret, int width, int height, int eol)
{
	HUFFNODE * whitetree = 0;
	HUFFNODE * blacktree = 0;
	BSTREAM *bs = 0;
	BSTREAM *bout = 0;
	int i, ii;
	int err = 0;
	int totlen;
	int len;
	int whitelen, blacklen;
	unsigned char *answer = 0;
	int Nout;

	Nout = (width + 7) / 8 * height;
	answer = malloc(Nout);
	if (!answer)
		goto out_of_memory;
	bout = bstream(answer, Nout, BIG_ENDIAN);
	if (!bout)
		goto out_of_memory;
	bs = bstream(in, count, LITTLE_ENDIAN);
	if (!bs)
		goto out_of_memory;
	for (i = 0; i < 105; i++)
		whitetree = addhuffmansymbol(whitetree, ccitttable[i].whitecode, ccitttable[i].whitelen, &err);
	for (i = 0; i < 105; i++)
		blacktree = addhuffmansymbol(blacktree, ccitttable[i].blackcode, ccitttable[i].blacklen, &err);
	if (err)
		goto out_of_memory;
	
	//debug(whitetree, 0);
	if(eol)
		len = gethuffmansymbol(whitetree, bs);
	for (i = 0; i < height; i++)
	{
		totlen = 0;
		while (totlen < width)
		{ 
			len = gethuffmansymbol(whitetree, bs);
			if (len == -1)
				goto parse_error;
			if (len == -2)
				whitelen = width - totlen;
			else
			  whitelen = len;
			while (len >= 64)
			{
				len = gethuffmansymbol(whitetree, bs);
				if (len == EOL || len == -1 || totlen + len + whitelen > width)
					goto parse_error;
				whitelen += len;
			}
			for (ii = 0; ii < whitelen; ii++)
				writebit(bout, 0);
			totlen += whitelen;
			if (totlen >= width)
				break;
			len = gethuffmansymbol(blacktree, bs);
			if (len < 0)
				goto parse_error;
			blacklen = len;
			while (len >= 64)
			{
				len = gethuffmansymbol(blacktree, bs);
				if (len == EOL || len == -1 || totlen + len + blacklen > width)
					goto parse_error;
				blacklen += len;
			}
			for (ii = 0; ii < blacklen; ii++)
				writebit(bout, 1);
			totlen += blacklen;

		}
		//synchtobyte(bs);
		if (width & 0x07)
		{
			for (ii = 0; ii < 8 - (width & 0x07); ii++)
				writebit(bout, 0);
		}
		if(eol)
			gethuffmansymbol(whitetree, bs);
		///synchtobyte(bs);
	}
	*Nret = Nout;
	killhuffmantree(whitetree);
	killhuffmantree(blacktree);
	free(bs);
	free(bout);
	return answer;
parse_error:
out_of_memory:
	killhuffmantree(whitetree);
	killhuffmantree(blacktree);
	free(bs);
	free(bout);
	free(answer);
	return 0;
}

static unsigned char *ccittgroup4decompress(unsigned char *in, unsigned long count, unsigned long *Nret, int width, int height, int eol)
{
	unsigned char *reference;
	unsigned char *current;
	unsigned char *temp;
	int i;
	int a0;
	int a1;
	int a2;
	int a1span, a2span;
	int seg;
	int b1, b2;
	HUFFNODE * twodtree = 0;
	HUFFNODE * whitetree = 0;
	HUFFNODE * blacktree = 0;
	BSTREAM *bs = 0;
	BSTREAM *bout = 0;
	unsigned long Nout;
	unsigned char *answer;
	int err =0;
	int mode;
	int colour;

	Nout = (width + 7) / 8 * height;
	answer = malloc(Nout);
	if (!answer)
		goto out_of_memory;
	//debug memset
	memset(answer, 0, Nout);
	bout = bstream(answer, Nout, BIG_ENDIAN);
	if (!bout)
		goto out_of_memory;
	bs = bstream(in, count, eol ? LITTLE_ENDIAN : BIG_ENDIAN);
	if (!bs)
		goto out_of_memory;
	for (i = 0; i < 105; i++)
		whitetree = addhuffmansymbol(whitetree, ccitttable[i].whitecode, ccitttable[i].whitelen, &err);
	for (i = 0; i < 105; i++)
		blacktree = addhuffmansymbol(blacktree, ccitttable[i].blackcode, ccitttable[i].blacklen, &err);
	for (i = 0; i < 11; i++)
		twodtree = addhuffmansymbol(twodtree, ccitt2dtable[i].code, ccitt2dtable[i].symbol, &err);
	if (err)
		goto out_of_memory;



	reference = malloc(width);
	for (i = 0; i < width; i++)
		reference[i] = 0;
	current = malloc(width);
	if (!current || !reference)
		goto out_of_memory;


	if (eol)
	{
		int len;

		while (getbit(bs) == 0)
			continue;
	}
	for (i = 0; i < height; i++)
	{
		a0 = -1;
		colour = 0;
		while (a0 < width)
		{
			b1 = a0 +1;
			while (b1 < width)
			{
				if (reference[b1] != colour && (b1 ==  0 || reference[b1] != reference[b1 - 1]) )
					break;
				b1++;
			}
			for (b2 = b1; b2 < width && reference[b2] == reference[b1]; b2++)
				;

			if (a0 == -1)
				a0 = 0;
			
			mode = gethuffmansymbol(twodtree, bs);
			a2 = -1;
			switch (mode)
			{
			case CCITT_PASS:
				a1 = b2;
				break;
			case CCITT_HORIZONTAL:
				a1span = 0;
				a2span = 0;
				if (colour == 0)
				{
					do
					{
						seg = gethuffmansymbol(whitetree, bs);
						a1span += seg;
					} while (seg >= 64 && a0 + a1span <= width);

					do
					{
						seg = gethuffmansymbol(blacktree, bs);
						a2span += seg;
					} while (seg >= 64 && a0 + a1span <= width);
				}
				else
				{
					do
					{
						seg = gethuffmansymbol(blacktree, bs);
						a1span += seg;
					} while (seg >= 64 && a0 + a1span  <= width);

					do
					{
						seg = gethuffmansymbol(whitetree, bs);
						a2span += seg;
					} while (seg >= 64 && a0 + a1span + a2span <= width);
				}
				a1 = a0 + a1span;
				a2 = a1 + a2span;
				if (a1 > width)
				{
					//printf("Bad span1 %d span2 %d here\n", a1span, a2span);
					goto parse_error;
				}
				break;
			case CCITT_VERTICAL_0:
				a1 = b1;
				break;
			case CCITT_VERTICAL_R1:
				a1 = b1 + 1;
				break;
			case CCITT_VERTICAL_R2:
				a1 = b1 + 2;
				break;
			case CCITT_VERTICAL_R3:
				a1 = b1 + 3;
				break;
			case CCITT_VERTICAL_L1:
				a1 = b1 - 1;
				break;
			case CCITT_VERTICAL_L2:
				a1 = b1 - 2;
				break;
			case CCITT_VERTICAL_L3:
				a1 = b1 - 3;
				break;
			case CCITT_ENDOFFAXBLOCK:
				goto endofblock;
				break;
			default:
				//printf("bad %d\n", mode);
				//for (i = 0; i < 32; i++)
					//printf("%d", getbit(bs));
				//printf("\n");
				//getchar();
				goto parse_error;
				break;
			}
			if (a1 <= a0 && a0 != 0 )
			{
				//printf("bad here a0 %d a1 %d\n", a0, a1);
				//getchar();
				goto parse_error;
			}
			//printf("a0 %d a1 %d a2 %d\n", a0, a1, a2);
			if (a0 < 0 || a1 < 0)
			{
				goto parse_error;
			}
			while (a0 < a1 && a0 < width)
			{
				current[a0++] = colour;
				writebit(bout, colour);
			}
			if (mode != CCITT_PASS)
				colour ^= 1;
			if(a1 >= 0)
				a0 = a1;
			if (a2 != -1)
			{
				while (a0 < a2 && a0 < width)
				{
					current[a0++] = colour;
					writebit(bout, colour);
				}
				colour ^= 1;
			}
		}
		//printf("here a0 %d\n", a0);
		if (eol)
		{
			while (getbit(bs) == 0)
				continue;
			//printf("mode %d\n", getbit(bs));
			//int endline = gethuffmansymbol(whitetree, bs);
			//printf("end %d\n", endline);
		}
		temp = reference;
		reference = current;
		current = temp;
		if (width % 8)
		{
			while (a0 % 8)
			{
				writebit(bout, 0);
				a0++;
			}
		}

	}
endofblock:
	*Nret = Nout;
	killhuffmantree(twodtree);
	killhuffmantree(whitetree);
	killhuffmantree(blacktree);
	free(reference);
	free(current);
	return answer;

parse_error:
	//printf("parse error\n");
	//getchar();
	*Nret = Nout;
	return answer;
out_of_memory:
	return 0;
}

/*
load the raster data
Params: out - return pointer for raster data, 0 for size run
fp - pointer to an open file
Nret - number of bytes read
Returns: 0 on success, -1 on fail.
*/
static int loadlzw(unsigned char *out, FILE *fp, unsigned long count, unsigned long *Nret)
{
	int codesize;
	//int block;
	int clear;
	int end;
	int nextcode;
	int codelen;
	unsigned char *stream = 0;
	int blen = 0;
	BSTREAM *bs;
	ENTRY *table;
	int pos = 0;
	int ii;
	int len;
	int second;
	int first;
	int tempcode;
	int ch;

	codesize = 8; 

	clear = 1 << codesize;
	end = clear + 1;
	nextcode = end + 1;
	codelen = codesize + 1;

	stream = malloc(count);
	if (!stream)
		return -1;
	fread(stream, count, 1, fp);

	table = malloc(sizeof(ENTRY) * (1 << 12));

	for (ii = 0; ii<nextcode; ii++)
	{
		table[ii].prefix = 0;
		table[ii].len = 1;
		table[ii].suffix = ii;
	}
	bs = bstream(stream, count, BIG_ENDIAN);

	first = getbits(bs, codelen);
	if (first != clear)
	{
		free(bs);
		bs = bstream(stream, count, LITTLE_ENDIAN);
		first = getbits(bs, codelen);
		if (first != clear)
			goto parse_error;
	}
	while (first == clear)
	{
		first = getbits(bs, codelen);
	}
	ch = first;
	if (out)
		out[0] = ch;
	pos = 1;

	while (1)
	{
		second = getbits(bs, codelen);
		if (second < 0)
			goto parse_error;
		if (second == clear)
		{
			nextcode = end + 1;
			codelen = codesize + 1;
			first = getbits(bs, codelen);

			while (first == clear)
				first = getbits(bs, codelen);
			if (first == end)
				break;
			ch = first;
			if (out)
				out[pos++] = first;
			else
				pos++;
			continue;
		}
		if (second == end)
		{
			break;
		}

		if (second >= nextcode)
		{
			len = table[first].len;
			//if (len + pos >= width * height)
			//	break;

			tempcode = first;
			for (ii = 0; ii<len; ii++)
			{
				if (out)
					out[pos + len - ii - 1] = (unsigned char)table[tempcode].suffix;
				tempcode = table[tempcode].prefix;
			}
			if (out)
				out[pos + len] = (unsigned char)ch;
			pos += len + 1;
		}
		else
		{
			len = table[second].len;
			//if (pos + len > width * height)
				//break;
			tempcode = second;

			for (ii = 0; ii<len; ii++)
			{
				ch = table[tempcode].suffix;
				if (out)
					out[pos + len - ii - 1] = (unsigned char)table[tempcode].suffix;
				tempcode = table[tempcode].prefix;
			}
			pos += len;
		}

		if (nextcode < 4096)
		{
			table[nextcode].prefix = first;
			table[nextcode].len = table[first].len + 1;
			table[nextcode].suffix = ch;

			nextcode++;
			if (nextcode == (1 << codelen) - ((bs->endianness == BIG_ENDIAN) ? 1 : 0))
			{
				codelen++;

				if (codelen == 13)
					codelen = 12;
			}
		}

		first = second;
	}


	free(table);
	free(stream);

	*Nret = pos;

	return 0;
parse_error:
	free(table);
	free(stream);
	return -1;
}

/*
create a bitstream.
Params: data - the data buffer
N - size of data buffer
Returns: constructed object
*/
static BSTREAM *bstream(unsigned char *data, int N, int endianness)
{
	BSTREAM *answer = malloc(sizeof(BSTREAM));

	if (!answer)
		return 0;
	answer->data = data;
	answer->pos = 0;
	answer->N = N;
	answer->endianness = endianness;

	if (answer->endianness == BIG_ENDIAN)
		answer->bit = 128;
	else
		answer->bit = 1;

	return answer;
}

/*
destroy a bitstream:
Params: bs - the bitstream to destroy.
*/
static void killbstream(BSTREAM *bs)
{
	free(bs);
}

/*
read a bit from the bitstream:
Params: bs - the bitstream;
Returns: the bit read
*/
static int getbit(BSTREAM *bs)
{
	int answer;
	
	if (bs->pos >= bs->N)
		return -1;
	answer = (bs->data[bs->pos] & bs->bit) ? 1 : 0;

	if (bs->endianness == BIG_ENDIAN)
	{
		bs->bit >>= 1;
		if (bs->bit == 0)
		{
			bs->bit = 128;
			bs->pos++;
		}
	}
	else
	{
		bs->bit <<= 1;
		if (bs->bit == 0x100)
		{
			bs->bit = 1;
			bs->pos++;
		}
	}
	return answer;
}

/*
read several bits from a bitstream:
Params: bs - the bitstream:
nbits - number of bits to read
Returns: the bits read (little-endian)
*/
static int getbits(BSTREAM *bs, int nbits)
{
	int i;
	int answer = 0;

	if (bs->endianness == BIG_ENDIAN)
	{
		for (i = 0; i < nbits; i++)
		{
			answer |= (getbit(bs) << (nbits - i - 1));
		}
	}
	else
	{
		for (i = 0; i < nbits; i++)
		{
			answer |= (getbit(bs) << i);
		}
	}

	return answer;
}

static int synchtobyte(BSTREAM *bs)
{
	if (bs->endianness == BIG_ENDIAN)
	{
		while (bs->bit != 0x80 && bs->pos < bs->N)
			getbit(bs);
	}
	else
	{
		while (bs->bit != 0x01 && bs->pos < bs->N)
			getbit(bs);
	}
	return 0;
}

static int writebit(BSTREAM *bs, int bit)
{
	if (bs->pos >= bs->N)
		return -1;
	if (bit)
		bs->data[bs->pos] |= bs->bit;
	else
		bs->data[bs->pos] &= (unsigned char)~bs->bit;
	getbit(bs);
	return 0;
}

/*
  sizeof() for a TIFF data type
  we default to 1
*/
static int tiffsizeof(int datatype)
{
	switch (datatype)
	{
	case TAG_BYTE: return 1;
	case TAG_ASCII: return 1;
	case TAG_SHORT: return 2;
	case TAG_LONG: return 4;
	case TAG_RATIONAL: return 8;
	default:
		return 1;
	}
}

/*
  load tag header
  Params: type - big endian or litle endian
          fp - file pointer
		  Ntags - return for number of tags
  Returns: the tags, 0 on error
*/
static TAG *floadheader(int type, FILE *fp, int *Ntags)
{
	TAG *answer;
	int N; 
	int i;
	int err;

	N = fget16u(type, fp);
	//printf("%d tags\n", N);
	answer = malloc(N * sizeof(TAG));
	if (!answer)
		goto out_of_memory;
	for (i = 0; i < N;i++)
	{
		answer[i].vector = 0;
		answer[i].ascii = 0;
	}

	for (i = 0; i < N; i++)
	{
		err = floadtag(&answer[i], type, fp);
		if (err)
			goto out_of_memory;
		/*
		if (answer[i].datacount == 1)
			printf("tag %d %f\n", answer[i].tagid, answer[i].scalar);
		else if (answer[i].datatype == TAG_ASCII)
			printf("tag %d %s\n", answer[i].tagid, answer[i].ascii);
		else
			printf("\n");
			*/

	}
	*Ntags = N;
	return answer;
out_of_memory:
	killtags(answer, N);
	return 0;
}

/*
  tags destructor
    Params: tags - items to destroy
	        N - number of tags
*/
static void killtags(TAG *tags, int N)
{
	int i;

	if (tags)
	{
		for (i = 0; i < N; i++)
		{
			free(tags[i].vector);
			free(tags[i].ascii);
		}
		free(tags);
	}
}

/*
  Load a tag froma file
    tag - the tag
	type - big endian or little endia
	fp - pointer to file
  Returns: 0 on success -1 on out of memory, -2 on parse error
*/
static int floadtag(TAG *tag, int type, FILE *fp)
{
	unsigned long offset;
	unsigned long pos;
	unsigned long num, denom;
	unsigned long datasize;
	unsigned long i;

	tag->tagid = fget16(type, fp);
	tag->datatype = fget16(type, fp);
	tag->datacount = fget32(type, fp);
	tag->vector = 0;
	tag->ascii = 0;
	tag->bad = 0;

	//printf("tag %d type %d N %ld ", tag->tagid, tag->datatype, tag->datacount);
	datasize = tag->datacount * tiffsizeof(tag->datatype);
	if (tag->datacount == 1)
	{
		switch (tag->datatype)
		{
		case TAG_BYTE:
			tag->scalar = (double)fgetc(fp);
			fgetc(fp);
			fgetc(fp);
			fgetc(fp);
			break;
		case TAG_ASCII:
			if (datasize > 4)
			{
				offset = fget32(type, fp);
				pos = ftell(fp);
				fseek(fp, offset, SEEK_SET);
			}
			tag->ascii = freadasciiz(fp);
			if (datasize > 4)
			{
				fseek(fp, pos, SEEK_SET);
			}
			break;
		case TAG_SHORT:
			tag->scalar = (double) fget16u(type, fp);
			fgetc(fp);
			fgetc(fp);
			break;
		case TAG_LONG:
			tag->scalar = (double) fget32u(type, fp);
			break;
		case TAG_RATIONAL:
			offset = fget32u(type, fp);
			pos = ftell(fp);
			fseek(fp, offset, SEEK_SET);
			num = fget32u(type, fp);
			denom = fget32u(type, fp);
			if (denom)
				tag->scalar = ((double)num) / denom;
			fseek(fp, pos, SEEK_SET);
			break;
		default:
			tag->bad = -1;
			break;
		}
	}
	else
	{
		if (datasize > 4)
		{
			offset = fget32(type, fp);
			pos = ftell(fp);
			fseek(fp, offset, SEEK_SET);
		}
		switch (tag->datatype)
		{
		case TAG_BYTE:
			tag->vector = malloc(datasize);
			if (!tag->vector)
				goto out_of_memory;
			fread(tag->vector, 1, datasize, fp);
			break;
		case TAG_ASCII:
			tag->ascii = freadasciiz(fp);
			if (!tag->ascii)
				goto out_of_memory;
			break;
		case TAG_SHORT:
			tag->vector = malloc(tag->datacount * sizeof(short));
			if (!tag->vector)
				goto out_of_memory;
			for (i = 0; i < tag->datacount; i++)
				((unsigned short *)tag->vector)[i] = fget16u(type, fp);
			break;
		case TAG_LONG:
			tag->vector = malloc(tag->datacount * sizeof(long));
			if (!tag->vector)
				goto out_of_memory;
			for (i = 0; i < tag->datacount; i++)
				((unsigned long *)tag->vector)[i] = fget32u(type, fp);
			break;
		case TAG_RATIONAL:
			tag->vector = malloc(tag->datacount * sizeof(double));
			if (!tag->vector)
				goto out_of_memory;
			for (i = 0; i < tag->datacount; i++)
			{
				num = fget32u(type, fp);
				denom = fget32u(type, fp);
				if (denom)
				((double *)tag->vector)[i] = ((double)num) / denom;
			}
			break;
		default:
			fgetc(fp);
			fgetc(fp);
			fgetc(fp);
			fgetc(fp);
			tag->bad = -1;
		}
		if (datasize > 4)
		{
			fseek(fp, pos, SEEK_SET);
		}
	}

	if (feof(fp))
		return -2;
	return 0;
out_of_memory:
	tag->bad = -1;
	return -1;
}

/*
  read an entry for a tag
    Tag - the tag read in
	index - index of data item in tag
Returns: value (as double) 
*/
static double tag_getentry(TAG *tag, int index)
{
	if (index >= tag->datacount)
		return -1.0;
	if (tag->datacount == 1)
		return tag->scalar;
	if (tag->bad)
		return -1;
	switch (tag->datatype)
	{
	case TAG_BYTE:
		return (double)((unsigned char *)tag->vector)[index];
	case TAG_ASCII:
		return (double)((char *)tag->vector)[index];
	case TAG_SHORT:
		return (double)((unsigned short *)tag->vector)[index];
	case TAG_LONG:
		return (double)((unsigned long *)tag->vector)[index];
	case TAG_RATIONAL:
		return (double)((double *)tag->vector)[index];
	default:
		return -1;
	}
}

/*
  safe paste function
     rgba - destination buffer
	 width, height - buffer dimensions
	 tile - the tile to paste
	 twisth, theight tile width tile height
	 x, y, x y co-ordinates to paste
*/
static void rgbapaste(unsigned char *rgba, int width, int height, unsigned char *tile, int twidth, int theight, int x, int y)
{
	int ix, iy;
	int tx, ty;

	for (ty = 0; ty < theight; ty++)
	{
		iy = y + ty;
		if (iy < 0)
			continue;
		if (iy >= height)
			break;
		for (tx = 0; tx < twidth; tx++)
		{
			ix = x + tx;
			if (ix < 0)
				continue;
			if (ix >= width)
				break;

			rgba[(iy*width + ix) * 4] = tile[(ty*twidth + tx) * 4];
			rgba[(iy*width + ix) * 4+1] = tile[(ty*twidth + tx) * 4+1];
			rgba[(iy*width + ix) * 4+2] = tile[(ty*twidth + tx) * 4+2];
			rgba[(iy*width + ix) * 4+3] = tile[(ty*twidth + tx) * 4+3];
		}

	}

}
unsigned long fget32u(int type, FILE *fp)
{
	int a, b, c, d;

	a = fgetc(fp);
	b = fgetc(fp);
	c = fgetc(fp);
	d = fgetc(fp);

	if (type == BIG_ENDIAN)
		return (a << 24) | (b << 16) | (c << 8) | d;
	else
		return (d << 24) | (c << 16) | (b << 8) | a;
}

unsigned int fget16u(int type, FILE *fp)
{
	int a, b;
	a = fgetc(fp);
	b = fgetc(fp);
	if (type == BIG_ENDIAN)
		return (a << 8) | b;
	else
		return (b << 8) | a;
}


static int fget16(int type, FILE *fp)
{
	if (type == BIG_ENDIAN)
		return fget16be(fp);
	else
		return fget16le(fp);
}

static long fget32(int type, FILE *fp)
{
	if (type == BIG_ENDIAN)
		return fget32be(fp);
	else
		return fget32le(fp);
}

static int fget16be(FILE *fp)
{
	int c1, c2;

	c2 = fgetc(fp);
	c1 = fgetc(fp);

	return ((c2 ^ 128) - 128) * 256 + c1;
}

static long fget32be(FILE *fp)
{
	int c1, c2, c3, c4;

	c4 = fgetc(fp);
	c3 = fgetc(fp);
	c2 = fgetc(fp);
	c1 = fgetc(fp);
	return ((c4 ^ 128) - 128) * 256 * 256 * 256 + c3 * 256 * 256 + c2 * 256 + c1;
}

static int fget16le(FILE *fp)
{
	int c1, c2;

	c1 = fgetc(fp);
	c2 = fgetc(fp);

	return ((c2 ^ 128) - 128) * 256 + c1;
}

static long fget32le(FILE *fp)
{
	int c1, c2, c3, c4;

	c1 = fgetc(fp);
	c2 = fgetc(fp);
	c3 = fgetc(fp);
	c4 = fgetc(fp);
	return ((c4 ^ 128) - 128) * 256 * 256 * 256 + c3 * 256 * 256 + c2 * 256 + c1;
}

static char *freadasciiz(FILE *fp)
{
	char *buff;
	char *temp;
	size_t bufflen = 64;
	size_t N = 0;
	int ch;

	buff = malloc(bufflen);
	while ((ch = fgetc(fp)) != EOF)
	{
		buff[N++] = ch;
		if (N == bufflen)
		{
			if ((bufflen + bufflen / 2) < bufflen)
				goto out_of_memory;
			temp = realloc(buff, bufflen + bufflen / 2);
			if (!temp)
				goto out_of_memory;
			buff = temp;
			bufflen = bufflen + bufflen / 2;
		}
		if (ch == 0)
			return realloc(buff, N);
	}

out_of_memory:
	free(buff);
	return 0;
}

/*
* read a double from a stream in ieee754 format regardless of host
*  encoding.
*  fp - the stream
*  bigendian - set to if big bytes first, clear for little bytes
*              first
*
*/
static double memreadieee754(unsigned char *buff, int bigendian)
{
	int i;
	double fnorm = 0.0;
	unsigned char temp;
	int sign;
	int exponent;
	double bitval;
	int maski, mask;
	int expbits = 11;
	int significandbits = 52;
	int shift;
	double answer;

	/* just reverse if not big-endian*/
	if (!bigendian)
	{
		for (i = 0; i < 4; i++)
		{
			temp = buff[i];
			buff[i] = buff[8 - i - 1];
			buff[8 - i - 1] = temp;
		}
	}
	sign = buff[0] & 0x80 ? -1 : 1;
	/* exponet in raw format*/
	exponent = ((buff[0] & 0x7F) << 4) | ((buff[1] & 0xF0) >> 4);

	/* read inthe mantissa. Top bit is 0.5, the successive bits half*/
	bitval = 0.5;
	maski = 1;
	mask = 0x08;
	for (i = 0; i < significandbits; i++)
	{
		if (buff[maski] & mask)
			fnorm += bitval;

		bitval /= 2.0;
		mask >>= 1;
		if (mask == 0)
		{
			mask = 0x80;
			maski++;
		}
	}
	/* handle zero specially */
	if (exponent == 0 && fnorm == 0)
		return 0.0;

	shift = exponent - ((1 << (expbits - 1)) - 1); /* exponent = shift + bias */
	/* nans have exp 1024 and non-zero mantissa */
	if (shift == 1024 && fnorm != 0)
		return sqrt(-1.0);
	/*infinity*/
	if (shift == 1024 && fnorm == 0)
	{

#ifdef INFINITY
		return sign == 1 ? INFINITY : -INFINITY;
#else
		return	(sign * 1.0) / 0.0;
#endif
	}
	if (shift > -1023)
	{
		answer = ldexp(fnorm + 1.0, shift);
		return answer * sign;
	}
	else
	{
		/* denormalised numbers */
		if (fnorm == 0.0)
			return 0.0;
		shift = -1022;
		while (fnorm < 1.0)
		{
			fnorm *= 2;
			shift--;
		}
		answer = ldexp(fnorm, shift);
		return answer * sign;
	}
}


static float memreadieee754f(unsigned char*mem, int bigendian)
{
	unsigned long buff = 0;
	unsigned long buff2 = 0;
	unsigned long mask;
	int sign;
	int exponent;
	int shift;
	int i;
	int significandbits = 23;
	int expbits = 8;
	double fnorm = 0.0;
	double bitval;
	double answer;

	if (bigendian)
		buff = (mem[0] << 24) | (mem[1] << 16) | (mem[2] << 8) | mem[3];
	else
		buff = (mem[3] << 24) | (mem[2] << 16) | (mem[1] << 8) | mem[0];

	sign = (buff & 0x80000000) ? -1 : 1;
	mask = 0x00400000;
	exponent = (buff & 0x7F800000) >> 23;
	bitval = 0.5;
	for(i=0;i<significandbits;i++)
	{
		if(buff & mask)
			fnorm += bitval;
		bitval /= 2;
		mask >>= 1;
	}
	if(exponent == 0 && fnorm == 0.0)
		return 0.0f;
	shift = exponent - ((1 << (expbits - 1)) - 1); /* exponent = shift + bias */

	if(shift == 128 && fnorm != 0.0)
		return (float) sqrt(-1.0);
	if(shift == 128 && fnorm == 0.0)
	{
#ifdef INFINITY
		return sign == 1 ? INFINITY : -INFINITY;
#else
		return (sign * 1.0f)/0.0f;
#endif
	}
	if(shift > -127)
	{
		answer = ldexp(fnorm + 1.0, shift);
		return (float) answer * sign;
	}
	else
	{
		if(fnorm == 0.0)
		{
			return 0.0f;
		}
		shift = -126;
		while (fnorm < 1.0)
		{
			fnorm *= 2;
			shift--;
		}
		answer = ldexp(fnorm, shift);
		return (float) answer * sign;
	}
}

static int YcbcrToRGB(int Y, int cb, int cr, unsigned char *red, unsigned char *green, unsigned char *blue)
{
	double r, g, b;

	r = (Y - 16) * 1.164 + (cr - 128) * 1.596;
	g = (Y - 16) * 1.164 + (cb - 128) * -0.392 + (cr - 128) *  -0.813;
	b = (Y - 16) * 1.164 + (cb - 128) * 2.017;

	*red = r < 0.0 ? 0 : r > 255 ? 255 : (unsigned char) r;
	*green = g < 0.0 ? 0 : g > 255 ? 255 : (unsigned char)g;
	*blue = b < 0.0 ? 0 : b > 255 ? 255 : (unsigned char)b;

	return 0;
}



/*

This section is a zlib decompressor written by Lode Vandevenne as part of his
PNG loader. Lode's asked for the following notice to be retained in derived
version of his software, so please don't remove.

The only change I've made is to separate out the fucntions and mark the
public ones as static (Malcolm)

LodePNG version 20120729

Copyright (c) 2005-2012 Lode Vandevenne

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

/// Stream
#define READBIT(bitpointer, bitstream) ((bitstream[bitpointer >> 3] >> (bitpointer & 0x7)) & (unsigned char)1)

static unsigned char readBitFromStream(size_t* bitpointer, const unsigned char* bitstream)
{
	unsigned char result = (unsigned char)(READBIT(*bitpointer, bitstream));
	(*bitpointer)++;
	return result;
}

static unsigned readBitsFromStream(size_t* bitpointer, const unsigned char* bitstream, size_t nbits)
{
	unsigned result = 0, i;
	for (i = 0; i < nbits; i++)
	{
		result += ((unsigned)READBIT(*bitpointer, bitstream)) << i;
		(*bitpointer)++;
	}
	return result;
}


/* ////////////////////////////////////////////////////////////////////////// */
/* / Adler32                                                                  */
/* ////////////////////////////////////////////////////////////////////////// */

/* ////////////////////////////////////////////////////////////////////////// */

static unsigned lodepng_read32bitInt(const unsigned char* buffer)
{
	return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}
static unsigned update_adler32(unsigned adler, const unsigned char* data, unsigned len)
{
	unsigned s1 = adler & 0xffff;
	unsigned s2 = (adler >> 16) & 0xffff;

	while (len > 0)
	{
		/*at least 5550 sums can be done before the sums overflow, saving a lot of module divisions*/
		unsigned amount = len > 5550 ? 5550 : len;
		len -= amount;
		while (amount > 0)
		{
			s1 = (s1 + *data++);
			s2 = (s2 + s1);
			amount--;
		}
		s1 %= 65521;
		s2 %= 65521;
	}

	return (s2 << 16) | s1;
}

/*Return the adler32 of the bytes data[0..len-1]*/
static unsigned adler32(const unsigned char* data, unsigned len)
{
	return update_adler32(1L, data, len);
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / Deflate - Huffman                                                      / */
/* ////////////////////////////////////////////////////////////////////////// */
#if 0
typedef struct LodePNGDecompressSettings
{
	unsigned ignore_adler32; /*if 1, continue and don't give an error message if the Adler32 checksum is corrupted*/
	unsigned custom_decoder; /*use custom decoder if LODEPNG_CUSTOM_ZLIB_DECODER and LODEPNG_COMPILE_ZLIB are enabled*/
} LodePNGDecompressSettings;
#endif

#define ERROR_BREAK(c) exit(0)

#define mymalloc malloc
#define myfree free
#define myrealloc realloc

#define FIRST_LENGTH_CODE_INDEX 257
#define LAST_LENGTH_CODE_INDEX 285
/*256 literals, the end code, some length codes, and 2 unused codes*/
#define NUM_DEFLATE_CODE_SYMBOLS 288
/*the distance codes have their own symbols, 30 used, 2 unused*/
#define NUM_DISTANCE_SYMBOLS 32
/*the code length codes. 0-15: code lengths, 16: copy previous 3-6 times, 17: 3-10 zeros, 18: 11-138 zeros*/
#define NUM_CODE_LENGTH_CODES 19

/*the base lengths represented by codes 257-285*/
static const unsigned LENGTHBASE[29]
= { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
67, 83, 99, 115, 131, 163, 195, 227, 258 };

/*the extra bits used by codes 257-285 (added to base length)*/
static const unsigned LENGTHEXTRA[29]
= { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
4, 4, 4, 4, 5, 5, 5, 5, 0 };

/*the base backwards distances (the bits of distance codes appear after length codes and use their own huffman tree)*/
static const unsigned DISTANCEBASE[30]
= { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513,
769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };

/*the extra bits of backwards distances (added to base)*/
static const unsigned DISTANCEEXTRA[30]
= { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8,
8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

/*the order in which "code length alphabet code lengths" are stored, out of this
the huffman tree of the dynamic huffman tree lengths is generated*/
static const unsigned CLCL_ORDER[NUM_CODE_LENGTH_CODES]
= { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

/* ////////////////////////////////////////////////////////////////////////// */

/*
Huffman tree struct, containing multiple representations of the tree
*/
typedef struct HuffmanTree
{
	unsigned* tree2d;
	unsigned* tree1d;
	unsigned* lengths; /*the lengths of the codes of the 1d-tree*/
	unsigned maxbitlen; /*maximum number of bits a single code can get*/
	unsigned numcodes; /*number of symbols in the alphabet = number of codes*/
} HuffmanTree;


static unsigned getTreeInflateDynamic(HuffmanTree* tree_ll, HuffmanTree* tree_d,
	const unsigned char* in, size_t* bp, size_t inlength);
static unsigned generateFixedLitLenTree(HuffmanTree* tree);
static unsigned generateFixedDistanceTree(HuffmanTree* tree);
static void HuffmanTree_init(HuffmanTree* tree);
static void HuffmanTree_cleanup(HuffmanTree* tree);
static void getTreeInflateFixed(HuffmanTree* tree_ll, HuffmanTree* tree_d);
static unsigned HuffmanTree_make2DTree(HuffmanTree* tree);
static unsigned HuffmanTree_makeFromLengths(HuffmanTree* tree, const unsigned* bitlen,
	size_t numcodes, unsigned maxbitlen);
static unsigned huffmanDecodeSymbol(const unsigned char* in, size_t* bp,
	const HuffmanTree* codetree, size_t inbitlength);

/* /////////////////////////////////////////////////////////////////////////// */

/*dynamic vector of unsigned chars*/
typedef struct ucvector
{
  unsigned char* data;
  size_t size; /*used size*/
  size_t allocsize; /*allocated size*/
} ucvector;

static unsigned inflateNoCompression(ucvector* out, const unsigned char* in, size_t* bp, size_t* pos, size_t inlength);
static unsigned inflateHuffmanBlock(ucvector* out, const unsigned char* in, size_t* bp,
	size_t* pos, size_t inlength, unsigned btype);

static unsigned lodepng_inflate(unsigned char** out, size_t* outsize,
	const unsigned char* in, size_t insize,
	const LodePNGDecompressSettings* settings);
static unsigned lodepng_inflatev(ucvector* out,
	const unsigned char* in, size_t insize,
	const LodePNGDecompressSettings* settings);


static void ucvector_cleanup(void* p)
{
  ((ucvector*)p)->size = ((ucvector*)p)->allocsize = 0;
  myfree(((ucvector*)p)->data);
  ((ucvector*)p)->data = NULL;
}

/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned ucvector_resize(ucvector* p, size_t size)
{
  if(size * sizeof(unsigned char) > p->allocsize)
  {
    size_t newsize = size * sizeof(unsigned char) * 2;
    void* data = myrealloc(p->data, newsize);
    if(data)
    {
      p->allocsize = newsize;
      p->data = (unsigned char*)data;
      p->size = size;
    }
    else return 0; /*error: not enough memory*/
  }
  else p->size = size;
  return 1;
}

static void ucvector_init(ucvector* p)
{
	p->data = NULL;
	p->size = p->allocsize = 0;
}

/*you can both convert from vector to buffer&size and vica versa. If you use
init_buffer to take over a buffer and size, it is not needed to use cleanup*/
static void ucvector_init_buffer(ucvector* p, unsigned char* buffer, size_t size)
{
	p->data = buffer;
	p->allocsize = p->size = size;
}

/*function used for debug purposes to draw the tree in ascii art with C++*/
/*#include <iostream>
static void HuffmanTree_draw(HuffmanTree* tree)
{
std::cout << "tree. length: " << tree->numcodes << " maxbitlen: " << tree->maxbitlen << std::endl;
for(size_t i = 0; i < tree->tree1d.size; i++)
{
if(tree->lengths.data[i])
std::cout << i << " " << tree->tree1d.data[i] << " " << tree->lengths.data[i] << std::endl;
}
std::cout << std::endl;
}*/

/*dynamic vector of unsigned ints*/
typedef struct uivector
{
	unsigned* data;
	size_t size; /*size in number of unsigned longs*/
	size_t allocsize; /*allocated size in bytes*/
} uivector;

static void uivector_cleanup(void* p)
{
	((uivector*)p)->size = ((uivector*)p)->allocsize = 0;
	myfree(((uivector*)p)->data);
	((uivector*)p)->data = NULL;
}

/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned uivector_resize(uivector* p, size_t size)
{
	if (size * sizeof(unsigned) > p->allocsize)
	{
		size_t newsize = size * sizeof(unsigned) * 2;
		void* data = myrealloc(p->data, newsize);
		if (data)
		{
			p->allocsize = newsize;
			p->data = (unsigned*)data;
			p->size = size;
		}
		else return 0;
	}
	else p->size = size;
	return 1;
}

/*resize and give all new elements the value*/
static unsigned uivector_resizev(uivector* p, size_t size, unsigned value)
{
	size_t oldsize = p->size, i;
	if (!uivector_resize(p, size)) return 0;
	for (i = oldsize; i < size; i++) p->data[i] = value;
	return 1;
}

static void uivector_init(uivector* p)
{
	p->data = NULL;
	p->size = p->allocsize = 0;
}




static unsigned inflateNoCompression(ucvector* out, const unsigned char* in, size_t* bp, size_t* pos, size_t inlength)
{
	/*go to first boundary of byte*/
	size_t p;
	unsigned LEN, NLEN, n, error = 0;
	while (((*bp) & 0x7) != 0) (*bp)++;
	p = (*bp) / 8; /*byte position*/

	/*read LEN (2 bytes) and NLEN (2 bytes)*/
	if (p >= inlength - 4) return 52; /*error, bit pointer will jump past memory*/
	LEN = in[p] + 256 * in[p + 1]; p += 2;
	NLEN = in[p] + 256 * in[p + 1]; p += 2;

	/*check if 16-bit NLEN is really the one's complement of LEN*/
	if (LEN + NLEN != 65535) return 21; /*error: NLEN is not one's complement of LEN*/

	if ((*pos) + LEN >= out->size)
	{
		if (!ucvector_resize(out, (*pos) + LEN)) return 83; /*alloc fail*/
	}

	/*read the literal data: LEN bytes are now stored in the out buffer*/
	if (p + LEN > inlength) return 23; /*error: reading outside of in buffer*/
	for (n = 0; n < LEN; n++) out->data[(*pos)++] = in[p++];

	(*bp) = p * 8;

	return error;
}

/*inflate a block with dynamic of fixed Huffman tree*/
static unsigned inflateHuffmanBlock(ucvector* out, const unsigned char* in, size_t* bp,
	size_t* pos, size_t inlength, unsigned btype)
{
	unsigned error = 0;
	HuffmanTree tree_ll; /*the huffman tree for literal and length codes*/
	HuffmanTree tree_d; /*the huffman tree for distance codes*/
	size_t inbitlength = inlength * 8;

	HuffmanTree_init(&tree_ll);
	HuffmanTree_init(&tree_d);

	if (btype == 1) getTreeInflateFixed(&tree_ll, &tree_d);
	else if (btype == 2)
	{
		error = getTreeInflateDynamic(&tree_ll, &tree_d, in, bp, inlength);
	}

	while (!error) /*decode all symbols until end reached, breaks at end code*/
	{
		/*code_ll is literal, length or end code*/
		unsigned code_ll = huffmanDecodeSymbol(in, bp, &tree_ll, inbitlength);
		if (code_ll <= 255) /*literal symbol*/
		{
			if ((*pos) >= out->size)
			{
				/*reserve more room at once*/
				if (!ucvector_resize(out, ((*pos) + 1) * 2)) ERROR_BREAK(83 /*alloc fail*/);
			}
			out->data[(*pos)] = (unsigned char)(code_ll);
			(*pos)++;
		}
		else if (code_ll >= FIRST_LENGTH_CODE_INDEX && code_ll <= LAST_LENGTH_CODE_INDEX) /*length code*/
		{
			unsigned code_d, distance;
			unsigned numextrabits_l, numextrabits_d; /*extra bits for length and distance*/
			size_t start, forward, backward, length;

			/*part 1: get length base*/
			length = LENGTHBASE[code_ll - FIRST_LENGTH_CODE_INDEX];

			/*part 2: get extra bits and add the value of that to length*/
			numextrabits_l = LENGTHEXTRA[code_ll - FIRST_LENGTH_CODE_INDEX];
			if (*bp >= inbitlength) ERROR_BREAK(51); /*error, bit pointer will jump past memory*/
			length += readBitsFromStream(bp, in, numextrabits_l);

			/*part 3: get distance code*/
			code_d = huffmanDecodeSymbol(in, bp, &tree_d, inbitlength);
			if (code_d > 29)
			{
				if (code_ll == (unsigned)(-1)) /*huffmanDecodeSymbol returns (unsigned)(-1) in case of error*/
				{
					/*return error code 10 or 11 depending on the situation that happened in huffmanDecodeSymbol
					(10=no endcode, 11=wrong jump outside of tree)*/
					error = (*bp) > inlength * 8 ? 10 : 11;
				}
				else error = 18; /*error: invalid distance code (30-31 are never used)*/
				break;
			}
			distance = DISTANCEBASE[code_d];

			/*part 4: get extra bits from distance*/
			numextrabits_d = DISTANCEEXTRA[code_d];
			if (*bp >= inbitlength) ERROR_BREAK(51); /*error, bit pointer will jump past memory*/

			distance += readBitsFromStream(bp, in, numextrabits_d);

			/*part 5: fill in all the out[n] values based on the length and dist*/
			start = (*pos);
			if (distance > start) ERROR_BREAK(52); /*too long backward distance*/
			backward = start - distance;
			if ((*pos) + length >= out->size)
			{
				/*reserve more room at once*/
				if (!ucvector_resize(out, ((*pos) + length) * 2)) ERROR_BREAK(83 /*alloc fail*/);
			}

			for (forward = 0; forward < length; forward++)
			{
				out->data[(*pos)] = out->data[backward];
				(*pos)++;
				backward++;
				if (backward >= start) backward = start - distance;
			}
		}
		else if (code_ll == 256)
		{
			break; /*end code, break the loop*/
		}
		else /*if(code == (unsigned)(-1))*/ /*huffmanDecodeSymbol returns (unsigned)(-1) in case of error*/
		{
			/*return error code 10 or 11 depending on the situation that happened in huffmanDecodeSymbol
			(10=no endcode, 11=wrong jump outside of tree)*/
			error = (*bp) > inlength * 8 ? 10 : 11;
			break;
		}
	}

	HuffmanTree_cleanup(&tree_ll);
	HuffmanTree_cleanup(&tree_d);

	return error;
}

/*get the tree of a deflated block with fixed tree, as specified in the deflate specification*/
static void getTreeInflateFixed(HuffmanTree* tree_ll, HuffmanTree* tree_d)
{
	/*TODO: check for out of memory errors*/
	generateFixedLitLenTree(tree_ll);
	generateFixedDistanceTree(tree_d);
}

/*get the tree of a deflated block with dynamic tree, the tree itself is also Huffman compressed with a known tree*/
static unsigned getTreeInflateDynamic(HuffmanTree* tree_ll, HuffmanTree* tree_d,
	const unsigned char* in, size_t* bp, size_t inlength)
{
	/*make sure that length values that aren't filled in will be 0, or a wrong tree will be generated*/
	unsigned error = 0;
	unsigned n, HLIT, HDIST, HCLEN, i;
	size_t inbitlength = inlength * 8;

	/*see comments in deflateDynamic for explanation of the context and these variables, it is analogous*/
	unsigned* bitlen_ll = 0; /*lit,len code lengths*/
	unsigned* bitlen_d = 0; /*dist code lengths*/
	/*code length code lengths ("clcl"), the bit lengths of the huffman tree used to compress bitlen_ll and bitlen_d*/
	unsigned* bitlen_cl = 0;
	HuffmanTree tree_cl; /*the code tree for code length codes (the huffman tree for compressed huffman trees)*/

	if ((*bp) >> 3 >= inlength - 2) return 49; /*error: the bit pointer is or will go past the memory*/

	/*number of literal/length codes + 257. Unlike the spec, the value 257 is added to it here already*/
	HLIT = readBitsFromStream(bp, in, 5) + 257;
	/*number of distance codes. Unlike the spec, the value 1 is added to it here already*/
	HDIST = readBitsFromStream(bp, in, 5) + 1;
	/*number of code length codes. Unlike the spec, the value 4 is added to it here already*/
	HCLEN = readBitsFromStream(bp, in, 4) + 4;

	HuffmanTree_init(&tree_cl);

	while (!error)
	{
		/*read the code length codes out of 3 * (amount of code length codes) bits*/

		bitlen_cl = (unsigned*)mymalloc(NUM_CODE_LENGTH_CODES * sizeof(unsigned));
		if (!bitlen_cl) ERROR_BREAK(83 /*alloc fail*/);

		for (i = 0; i < NUM_CODE_LENGTH_CODES; i++)
		{
			if (i < HCLEN) bitlen_cl[CLCL_ORDER[i]] = readBitsFromStream(bp, in, 3);
			else bitlen_cl[CLCL_ORDER[i]] = 0; /*if not, it must stay 0*/
		}

		error = HuffmanTree_makeFromLengths(&tree_cl, bitlen_cl, NUM_CODE_LENGTH_CODES, 7);
		if (error) break;

		/*now we can use this tree to read the lengths for the tree that this function will return*/
		bitlen_ll = (unsigned*)mymalloc(NUM_DEFLATE_CODE_SYMBOLS * sizeof(unsigned));
		bitlen_d = (unsigned*)mymalloc(NUM_DISTANCE_SYMBOLS * sizeof(unsigned));
		if (!bitlen_ll || !bitlen_d) ERROR_BREAK(83 /*alloc fail*/);
		for (i = 0; i < NUM_DEFLATE_CODE_SYMBOLS; i++) bitlen_ll[i] = 0;
		for (i = 0; i < NUM_DISTANCE_SYMBOLS; i++) bitlen_d[i] = 0;

		/*i is the current symbol we're reading in the part that contains the code lengths of lit/len and dist codes*/
		i = 0;
		while (i < HLIT + HDIST)
		{
			unsigned code = huffmanDecodeSymbol(in, bp, &tree_cl, inbitlength);
			if (code <= 15) /*a length code*/
			{
				if (i < HLIT) bitlen_ll[i] = code;
				else bitlen_d[i - HLIT] = code;
				i++;
			}
			else if (code == 16) /*repeat previous*/
			{
				unsigned replength = 3; /*read in the 2 bits that indicate repeat length (3-6)*/
				unsigned value; /*set value to the previous code*/

				if (*bp >= inbitlength) ERROR_BREAK(50); /*error, bit pointer jumps past memory*/
				if (i == 0) ERROR_BREAK(54); /*can't repeat previous if i is 0*/

				replength += readBitsFromStream(bp, in, 2);

				if (i < HLIT + 1) value = bitlen_ll[i - 1];
				else value = bitlen_d[i - HLIT - 1];
				/*repeat this value in the next lengths*/
				for (n = 0; n < replength; n++)
				{
					if (i >= HLIT + HDIST) ERROR_BREAK(13); /*error: i is larger than the amount of codes*/
					if (i < HLIT) bitlen_ll[i] = value;
					else bitlen_d[i - HLIT] = value;
					i++;
				}
			}
			else if (code == 17) /*repeat "0" 3-10 times*/
			{
				unsigned replength = 3; /*read in the bits that indicate repeat length*/
				if (*bp >= inbitlength) ERROR_BREAK(50); /*error, bit pointer jumps past memory*/

				replength += readBitsFromStream(bp, in, 3);

				/*repeat this value in the next lengths*/
				for (n = 0; n < replength; n++)
				{
					if (i >= HLIT + HDIST) ERROR_BREAK(14); /*error: i is larger than the amount of codes*/

					if (i < HLIT) bitlen_ll[i] = 0;
					else bitlen_d[i - HLIT] = 0;
					i++;
				}
			}
			else if (code == 18) /*repeat "0" 11-138 times*/
			{
				unsigned replength = 11; /*read in the bits that indicate repeat length*/
				if (*bp >= inbitlength) ERROR_BREAK(50); /*error, bit pointer jumps past memory*/

				replength += readBitsFromStream(bp, in, 7);

				/*repeat this value in the next lengths*/
				for (n = 0; n < replength; n++)
				{
					if (i >= HLIT + HDIST) ERROR_BREAK(15); /*error: i is larger than the amount of codes*/

					if (i < HLIT) bitlen_ll[i] = 0;
					else bitlen_d[i - HLIT] = 0;
					i++;
				}
			}
			else /*if(code == (unsigned)(-1))*/ /*huffmanDecodeSymbol returns (unsigned)(-1) in case of error*/
			{
				if (code == (unsigned)(-1))
				{
					/*return error code 10 or 11 depending on the situation that happened in huffmanDecodeSymbol
					(10=no endcode, 11=wrong jump outside of tree)*/
					error = (*bp) > inbitlength ? 10 : 11;
				}
				else error = 16; /*unexisting code, this can never happen*/
				break;
			}
		}
		if (error) break;

		if (bitlen_ll[256] == 0) ERROR_BREAK(64); /*the length of the end code 256 must be larger than 0*/

		/*now we've finally got HLIT and HDIST, so generate the code trees, and the function is done*/
		error = HuffmanTree_makeFromLengths(tree_ll, bitlen_ll, NUM_DEFLATE_CODE_SYMBOLS, 15);
		if (error) break;
		error = HuffmanTree_makeFromLengths(tree_d, bitlen_d, NUM_DISTANCE_SYMBOLS, 15);

		break; /*end of error-while*/
	}

	myfree(bitlen_cl);
	myfree(bitlen_ll);
	myfree(bitlen_d);
	HuffmanTree_cleanup(&tree_cl);

	return error;
}

/*get the literal and length code tree of a deflated block with fixed tree, as per the deflate specification*/
static unsigned generateFixedLitLenTree(HuffmanTree* tree)
{
	unsigned i, error = 0;
	unsigned* bitlen = (unsigned*)mymalloc(NUM_DEFLATE_CODE_SYMBOLS * sizeof(unsigned));
	if (!bitlen) return 83; /*alloc fail*/

	/*288 possible codes: 0-255=literals, 256=endcode, 257-285=lengthcodes, 286-287=unused*/
	for (i = 0; i <= 143; i++) bitlen[i] = 8;
	for (i = 144; i <= 255; i++) bitlen[i] = 9;
	for (i = 256; i <= 279; i++) bitlen[i] = 7;
	for (i = 280; i <= 287; i++) bitlen[i] = 8;

	error = HuffmanTree_makeFromLengths(tree, bitlen, NUM_DEFLATE_CODE_SYMBOLS, 15);

	myfree(bitlen);
	return error;
}

/*get the distance code tree of a deflated block with fixed tree, as specified in the deflate specification*/
static unsigned generateFixedDistanceTree(HuffmanTree* tree)
{
	unsigned i, error = 0;
	unsigned* bitlen = (unsigned*)mymalloc(NUM_DISTANCE_SYMBOLS * sizeof(unsigned));
	if (!bitlen) return 83; /*alloc fail*/

	/*there are 32 distance codes, but 30-31 are unused*/
	for (i = 0; i < NUM_DISTANCE_SYMBOLS; i++) bitlen[i] = 5;
	error = HuffmanTree_makeFromLengths(tree, bitlen, NUM_DISTANCE_SYMBOLS, 15);

	myfree(bitlen);
	return error;
}

static void HuffmanTree_init(HuffmanTree* tree)
{
	tree->tree2d = 0;
	tree->tree1d = 0;
	tree->lengths = 0;
}

static void HuffmanTree_cleanup(HuffmanTree* tree)
{
	myfree(tree->tree2d);
	myfree(tree->tree1d);
	myfree(tree->lengths);
}

/*the tree representation used by the decoder. return value is error*/
static unsigned HuffmanTree_make2DTree(HuffmanTree* tree)
{
	unsigned nodefilled = 0; /*up to which node it is filled*/
	unsigned treepos = 0; /*position in the tree (1 of the numcodes columns)*/
	unsigned n, i;

	tree->tree2d = (unsigned*)mymalloc(tree->numcodes * 2 * sizeof(unsigned));
	if (!tree->tree2d) return 83; /*alloc fail*/

	/*
	convert tree1d[] to tree2d[][]. In the 2D array, a value of 32767 means
	uninited, a value >= numcodes is an address to another bit, a value < numcodes
	is a code. The 2 rows are the 2 possible bit values (0 or 1), there are as
	many columns as codes - 1.
	A good huffmann tree has N * 2 - 1 nodes, of which N - 1 are internal nodes.
	Here, the internal nodes are stored (what their 0 and 1 option point to).
	There is only memory for such good tree currently, if there are more nodes
	(due to too long length codes), error 55 will happen
	*/
	for (n = 0; n < tree->numcodes * 2; n++)
	{
		tree->tree2d[n] = 32767; /*32767 here means the tree2d isn't filled there yet*/
	}

	for (n = 0; n < tree->numcodes; n++) /*the codes*/
	{
		for (i = 0; i < tree->lengths[n]; i++) /*the bits for this code*/
		{
			unsigned char bit = (unsigned char)((tree->tree1d[n] >> (tree->lengths[n] - i - 1)) & 1);
			if (treepos > tree->numcodes - 2) return 55; /*oversubscribed, see comment in lodepng_error_text*/
			if (tree->tree2d[2 * treepos + bit] == 32767) /*not yet filled in*/
			{
				if (i + 1 == tree->lengths[n]) /*last bit*/
				{
					tree->tree2d[2 * treepos + bit] = n; /*put the current code in it*/
					treepos = 0;
				}
				else
				{
					/*put address of the next step in here, first that address has to be found of course
					(it's just nodefilled + 1)...*/
					nodefilled++;
					/*addresses encoded with numcodes added to it*/
					tree->tree2d[2 * treepos + bit] = nodefilled + tree->numcodes;
					treepos = nodefilled;
				}
			}
			else treepos = tree->tree2d[2 * treepos + bit] - tree->numcodes;
		}
	}

	for (n = 0; n < tree->numcodes * 2; n++)
	{
		if (tree->tree2d[n] == 32767) tree->tree2d[n] = 0; /*remove possible remaining 32767's*/
	}

	return 0;
}

/*
Second step for the ...makeFromLengths and ...makeFromFrequencies functions.
numcodes, lengths and maxbitlen must already be filled in correctly. return
value is error.
*/
static unsigned HuffmanTree_makeFromLengths2(HuffmanTree* tree)
{
	uivector blcount;
	uivector nextcode;
	unsigned bits, n, error = 0;

	uivector_init(&blcount);
	uivector_init(&nextcode);

	tree->tree1d = (unsigned*)mymalloc(tree->numcodes * sizeof(unsigned));
	if (!tree->tree1d) error = 83; /*alloc fail*/

	if (!uivector_resizev(&blcount, tree->maxbitlen + 1, 0)
		|| !uivector_resizev(&nextcode, tree->maxbitlen + 1, 0))
		error = 83; /*alloc fail*/

	if (!error)
	{
		/*step 1: count number of instances of each code length*/
		for (bits = 0; bits < tree->numcodes; bits++) blcount.data[tree->lengths[bits]]++;
		/*step 2: generate the nextcode values*/
		for (bits = 1; bits <= tree->maxbitlen; bits++)
		{
			nextcode.data[bits] = (nextcode.data[bits - 1] + blcount.data[bits - 1]) << 1;
		}
		/*step 3: generate all the codes*/
		for (n = 0; n < tree->numcodes; n++)
		{
			if (tree->lengths[n] != 0) tree->tree1d[n] = nextcode.data[tree->lengths[n]]++;
		}
	}

	uivector_cleanup(&blcount);
	uivector_cleanup(&nextcode);

	if (!error) return HuffmanTree_make2DTree(tree);
	else return error;
}

/*
given the code lengths (as stored in the PNG file), generate the tree as defined
by Deflate. maxbitlen is the maximum bits that a code in the tree can have.
return value is error.
*/
static unsigned HuffmanTree_makeFromLengths(HuffmanTree* tree, const unsigned* bitlen,
	size_t numcodes, unsigned maxbitlen)
{
	unsigned i;
	tree->lengths = (unsigned*)mymalloc(numcodes * sizeof(unsigned));
	if (!tree->lengths) return 83; /*alloc fail*/
	for (i = 0; i < numcodes; i++) tree->lengths[i] = bitlen[i];
	tree->numcodes = (unsigned)numcodes; /*number of symbols*/
	tree->maxbitlen = maxbitlen;
	return HuffmanTree_makeFromLengths2(tree);
}


/*
returns the code, or (unsigned)(-1) if error happened
inbitlength is the length of the complete buffer, in bits (so its byte length times 8)
*/
static unsigned huffmanDecodeSymbol(const unsigned char* in, size_t* bp,
	const HuffmanTree* codetree, size_t inbitlength)
{
	unsigned treepos = 0, ct;
	for (;;)
	{
		if (*bp >= inbitlength) return (unsigned)(-1); /*error: end of input memory reached without endcode*/
		/*
		decode the symbol from the tree. The "readBitFromStream" code is inlined in
		the expression below because this is the biggest bottleneck while decoding
		*/
		ct = codetree->tree2d[(treepos << 1) + READBIT(*bp, in)];
		(*bp)++;
		if (ct < codetree->numcodes) return ct; /*the symbol is decoded, return it*/
		else treepos = ct - codetree->numcodes; /*symbol not yet decoded, instead move tree position*/

		if (treepos >= codetree->numcodes) return (unsigned)(-1); /*error: it appeared outside the codetree*/
	}
}


/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* Main deflate section
/*///////////////////////////////////////////////////////////////////////////////////////////////*/

/*
  (Malcolm comment)
  Lode's main function, call this
     out - malloced return for decompressed output
	       outsize - return for output size
		   in - the zlib stream
		   insize - number of bytes in input stream
		   settings -
			 typedef struct LodePNGDecompressSettings
			 {
			 unsigned ignore_adler32; // 1, continue without warning if Adler checksum corrupted  
             unsigned custom_decoder; //use custom decoder if LODEPNG_CUSTOM_ZLIB_DECODER and LODEPNG_COMPILE_ZLIB are enabled
             } LodePNGDecompressSettings;
			 Pass 1, 0 if unsure what to do.
	returns: an error code, 0 on success
*/
static unsigned lodepng_zlib_decompress(unsigned char** out, size_t* outsize, const unsigned char* in,
	size_t insize, const LodePNGDecompressSettings* settings)
{
	unsigned error = 0;
	unsigned CM, CINFO, FDICT;


	if (insize < 2) return 53; /*error, size of zlib data too small*/
							   /*read information from zlib header*/
	if ((in[0] * 256 + in[1]) % 31 != 0)
	{
		/*error: 256 * in[0] + in[1] must be a multiple of 31, the FCHECK value is supposed to be made that way*/
		return 24;
	}

	CM = in[0] & 15;
	CINFO = (in[0] >> 4) & 15;
	/*FCHECK = in[1] & 31;*/ /*FCHECK is already tested above*/
	FDICT = (in[1] >> 5) & 1;
	/*FLEVEL = (in[1] >> 6) & 3;*/ /*FLEVEL is not used here*/

	if (CM != 8 || CINFO > 7)
	{
		/*error: only compression method 8: inflate with sliding window of 32k is supported by the PNG spec*/
		return 25;
	}
	if (FDICT != 0)
	{
		/*error: the specification of PNG says about the zlib stream:
		"The additional flags shall not specify a preset dictionary."*/
		return 26;
	}
	error = lodepng_inflate(out, outsize, in + 2, insize - 2, settings);
	if (error) return error;

	if (!settings->ignore_adler32)
	{
		unsigned ADLER32 = lodepng_read32bitInt(&in[insize - 4]);
		unsigned checksum = adler32(*out, (unsigned)(*outsize));
		if (checksum != ADLER32) return 58; /*error, adler checksum not correct, data must be corrupted*/
	}

	return 0; /*no error*/
#if LODEPNG_CUSTOM_ZLIB_DECODER == 1
}
#endif /*LODEPNG_CUSTOM_ZLIB_DECODER == 1*/
}

static unsigned lodepng_inflate(unsigned char** out, size_t* outsize,
	const unsigned char* in, size_t insize,
	const LodePNGDecompressSettings* settings)
{
#if LODEPNG_CUSTOM_ZLIB_DECODER == 2
	if (settings->custom_decoder)
	{
		return lodepng_custom_inflate(out, outsize, in, insize, settings);
	}
	else
	{
#endif /*LODEPNG_CUSTOM_ZLIB_DECODER == 2*/
		unsigned error;
		ucvector v;
		ucvector_init_buffer(&v, *out, *outsize);
		error = lodepng_inflatev(&v, in, insize, settings);
		*out = v.data;
		*outsize = v.size;
		return error;
#if LODEPNG_CUSTOM_ZLIB_DECODER == 2
	}
#endif /*LODEPNG_CUSTOM_ZLIB_DECODER == 2*/
}



static unsigned lodepng_inflatev(ucvector* out,
	const unsigned char* in, size_t insize,
	const LodePNGDecompressSettings* settings)
{
	/*bit pointer in the "in" data, current byte is bp >> 3, current bit is bp & 0x7 (from lsb to msb of the byte)*/
	size_t bp = 0;
	unsigned BFINAL = 0;
	size_t pos = 0; /*byte position in the out buffer*/

	unsigned error = 0;

	(void)settings;

	while (!BFINAL)
	{
		unsigned BTYPE;
		if (bp + 2 >= insize * 8) return 52; /*error, bit pointer will jump past memory*/
		BFINAL = readBitFromStream(&bp, in);
		BTYPE = 1 * readBitFromStream(&bp, in);
		BTYPE += 2 * readBitFromStream(&bp, in);

		if (BTYPE == 3) return 20; /*error: invalid BTYPE*/
		else if (BTYPE == 0) error = inflateNoCompression(out, in, &bp, &pos, insize); /*no compression*/
		else error = inflateHuffmanBlock(out, in, &bp, &pos, insize, BTYPE); /*compression, BTYPE 01 or 10*/

		if (error) return error;
	}

	/*Only now we know the true size of out, resize it to that*/
	if (!ucvector_resize(out, pos)) error = 83; /*alloc fail*/

	return error;
}
