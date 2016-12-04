
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <cassert>

/*
 * BDF to OpenSpades font converter.
 *
 * Copyright (c) 2013 yvt
 * WTFPL except for the targa reader/writer part.
 *
 * This source code is based on:
 ---------------------------------------------------------------------------
 * Truevision Targa Reader/Writer
 * Copyright (C) 2001-2003 Emil Mikulic.
 *
 * Source and binary redistribution of this code, with or without changes, for
 * free or for profit, is allowed as long as this copyright notice is kept
 * intact.  Modified versions must be clearly marked as modified.
 *
 * This code is provided without any warranty.  The copyright holder is
 * not liable for anything bad that might happen as a result of the
 * code.
 * -------------------------------------------------------------------------*/

#pragma mark - targa.c

#define TGA_KEEP_MACROS /* BIT, htole16, letoh16 */

#include <stdio.h>
#ifndef _MSC_VER
# include <inttypes.h>
#else /* MSVC */
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
#endif

#define BIT(index) (1 << (index))

#ifdef _BIG_ENDIAN
# define htole16(x) ( (((x) & 0x00FF) << 8) | (((x) & 0xFF00) >> 8) )
# define letoh16(x) htole16(x)
#else /* little endian */
# define htole16(x) (x)
# define letoh16(x) (x)
#endif /* endianness */



/* Targa image and header fields -------------------------------------------*/
typedef struct
{
	/* Note that Targa is stored in little-endian order */
	uint8_t     image_id_length;

	uint8_t     color_map_type;
	/* color map = palette */
#define TGA_COLOR_MAP_ABSENT    0
#define TGA_COLOR_MAP_PRESENT   1

	uint8_t     image_type;
#define TGA_IMAGE_TYPE_NONE          0 /* no image data */
#define TGA_IMAGE_TYPE_COLORMAP      1 /* uncompressed, color-mapped */
#define TGA_IMAGE_TYPE_BGR           2 /* uncompressed, true-color */
#define TGA_IMAGE_TYPE_MONO          3 /* uncompressed, black and white */
#define TGA_IMAGE_TYPE_COLORMAP_RLE  9 /* run-length, color-mapped */
#define TGA_IMAGE_TYPE_BGR_RLE      10 /* run-length, true-color */
#define TGA_IMAGE_TYPE_MONO_RLE     11 /* run-length, black and white */

	/* color map specification */
	uint16_t    color_map_origin;   /* index of first entry */
	uint16_t    color_map_length;   /* number of entries included */
	uint8_t     color_map_depth;    /* number of bits per entry */

	/* image specification */
	uint16_t    origin_x;
	uint16_t    origin_y;
	uint16_t    width;
	uint16_t    height;
	uint8_t     pixel_depth;

	uint8_t     image_descriptor;
	/* bits 0,1,2,3 - attribute bits per pixel
	 * bit  4       - set if image is stored right-to-left
	 * bit  5       - set if image is stored top-to-bottom
	 * bits 6,7     - unused (must be set to zero)
	 */
#define TGA_ATTRIB_BITS (uint8_t)(BIT(0)|BIT(1)|BIT(2)|BIT(3))
#define TGA_R_TO_L_BIT  (uint8_t)BIT(4)
#define TGA_T_TO_B_BIT  (uint8_t)BIT(5)
#define TGA_UNUSED_BITS (uint8_t)(BIT(6)|BIT(7))
	/* Note: right-to-left order is not honored by some Targa readers */

	uint8_t *image_id;
	/* The length of this field is given in image_id_length, it's read raw
	 * from the file so it's not not guaranteed to be zero-terminated.  If
	 * it's not NULL, it needs to be deallocated.  see: tga_free_buffers()
	 */

	uint8_t *color_map_data;
	/* See the "color map specification" fields above.  If not NULL, this
	 * field needs to be deallocated.  see: tga_free_buffers()
	 */

	uint8_t *image_data;
	/* Follows image specification fields (see above) */

	/* Extension area and developer area are silently ignored.  The Targa 2.0
	 * spec says we're not required to read or write them.
	 */

} tga_image;



/* For decoding header bits ------------------------------------------------*/
uint8_t tga_get_attribute_bits(const tga_image *tga);
int tga_is_right_to_left(const tga_image *tga);
int tga_is_top_to_bottom(const tga_image *tga);
int tga_is_colormapped(const tga_image *tga);
int tga_is_rle(const tga_image *tga);
int tga_is_mono(const tga_image *tga);



/* Error handling ----------------------------------------------------------*/
typedef enum {
	TGA_NOERR,
	TGAERR_FOPEN,
	TGAERR_EOF,
	TGAERR_WRITE,
	TGAERR_CMAP_TYPE,
	TGAERR_IMG_TYPE,
	TGAERR_NO_IMG,
	TGAERR_CMAP_MISSING,
	TGAERR_CMAP_PRESENT,
	TGAERR_CMAP_LENGTH,
	TGAERR_CMAP_DEPTH,
	TGAERR_ZERO_SIZE,
	TGAERR_PIXEL_DEPTH,
	TGAERR_NO_MEM,
	TGAERR_NOT_CMAP,
	TGAERR_RLE,
	TGAERR_INDEX_RANGE,
	TGAERR_MONO
} tga_result;

const char *tga_error(const tga_result errcode);



/* Load/save ---------------------------------------------------------------*/
tga_result tga_read_from_FILE(tga_image *dest, std::istream& fp);
tga_result tga_write_to_FILE(std::ostream& fp, const tga_image *src);



/* Convenient writing functions --------------------------------------------*/
tga_result tga_write_mono(const char *filename, uint8_t *image,
						  const uint16_t width, const uint16_t height);

tga_result tga_write_mono_rle(const char *filename, uint8_t *image,
							  const uint16_t width, const uint16_t height);

tga_result tga_write_bgr(const char *filename, uint8_t *image,
						 const uint16_t width, const uint16_t height, const uint8_t depth);

tga_result tga_write_bgr_rle(const char *filename, uint8_t *image,
							 const uint16_t width, const uint16_t height, const uint8_t depth);

/* These functions will use tga_swap_red_blue to MODIFY your image data */
tga_result tga_write_rgb(const char *filename, uint8_t *image,
						 const uint16_t width, const uint16_t height, const uint8_t depth);

tga_result tga_write_rgb_rle(const char *filename, uint8_t *image,
							 const uint16_t width, const uint16_t height, const uint8_t depth);



/* Manipulation ------------------------------------------------------------*/
tga_result tga_flip_horiz(tga_image *img);
tga_result tga_flip_vert(tga_image *img);
tga_result tga_color_unmap(tga_image *img);

uint8_t *tga_find_pixel(const tga_image *img, uint16_t x, uint16_t y);
tga_result tga_unpack_pixel(const uint8_t *src, const uint8_t bits,
							uint8_t *b, uint8_t *g, uint8_t *r, uint8_t *a);
tga_result tga_pack_pixel(uint8_t *dest, const uint8_t bits,
						  const uint8_t b, const uint8_t g, const uint8_t r, const uint8_t a);

tga_result tga_desaturate(tga_image *img,
						  const int cr, const int cg, const int cb, const int dv);
tga_result tga_desaturate_rec_601_1(tga_image *img);
tga_result tga_desaturate_rec_709(tga_image *img);
tga_result tga_desaturate_itu(tga_image *img);
tga_result tga_desaturate_avg(tga_image *img);
tga_result tga_convert_depth(tga_image *img, const uint8_t bits);
tga_result tga_swap_red_blue(tga_image *img);

void tga_free_buffers(tga_image *img);



#ifndef TGA_KEEP_MACROS /* useful for targa.c */
# undef htole16
# undef letoh16
#endif

#pragma mark - targa.c

#include <stdlib.h>
#include <string.h> /* memcpy, memcmp */

#define SANE_DEPTH(x) ((x) == 8 || (x) == 16 || (x) == 24 || (x) == 32)
#define UNMAP_DEPTH(x)            ((x) == 16 || (x) == 24 || (x) == 32)

static const char tga_id[] =
"\0\0\0\0" /* extension area offset */
"\0\0\0\0" /* developer directory offset */
"TRUEVISION-XFILE.";

static const size_t tga_id_length = 26; /* tga_id + \0 */



/* helpers */
static tga_result tga_read_rle(tga_image *dest, std::istream& fp);
static tga_result tga_write_row_RLE(std::ostream& fp,
									const tga_image *src, const uint8_t *row);
typedef enum { RAW, RLE } packet_type;
static packet_type rle_packet_type(const uint8_t *row, const uint16_t pos,
								   const uint16_t width, const uint16_t bpp);
static uint8_t rle_packet_len(const uint8_t *row, const uint16_t pos,
							  const uint16_t width, const uint16_t bpp, const packet_type type);



uint8_t tga_get_attribute_bits(const tga_image *tga)
{
	return tga->image_descriptor & TGA_ATTRIB_BITS;
}

int tga_is_right_to_left(const tga_image *tga)
{
	return (tga->image_descriptor & TGA_R_TO_L_BIT) != 0;
}

int tga_is_top_to_bottom(const tga_image *tga)
{
	return (tga->image_descriptor & TGA_T_TO_B_BIT) != 0;
}

int tga_is_colormapped(const tga_image *tga)
{
	return (
			tga->image_type == TGA_IMAGE_TYPE_COLORMAP ||
			tga->image_type == TGA_IMAGE_TYPE_COLORMAP_RLE
			);
}

int tga_is_rle(const tga_image *tga)
{
	return (
			tga->image_type == TGA_IMAGE_TYPE_COLORMAP_RLE  ||
			tga->image_type == TGA_IMAGE_TYPE_BGR_RLE       ||
			tga->image_type == TGA_IMAGE_TYPE_MONO_RLE
			);
}

int tga_is_mono(const tga_image *tga)
{
	return (
			tga->image_type == TGA_IMAGE_TYPE_MONO      ||
			tga->image_type == TGA_IMAGE_TYPE_MONO_RLE
			);
}



/* ---------------------------------------------------------------------------
 * Convert the numerical <errcode> into a verbose error string.
 *
 * Returns: an error string
 */
const char *tga_error(const tga_result errcode)
{
	switch (errcode)
	{
		case TGA_NOERR:
			return "no error";
		case TGAERR_FOPEN:
			return "error opening file";
		case TGAERR_EOF:
			return "premature end of file";
		case TGAERR_WRITE:
			return "error writing to file";
		case TGAERR_CMAP_TYPE:
			return "invalid color map type";
		case TGAERR_IMG_TYPE:
			return "invalid image type";
		case TGAERR_NO_IMG:
			return "no image data included";
		case TGAERR_CMAP_MISSING:
			return "color-mapped image without color map";
		case TGAERR_CMAP_PRESENT:
			return "non-color-mapped image with extraneous color map";
		case TGAERR_CMAP_LENGTH:
			return "color map has zero length";
		case TGAERR_CMAP_DEPTH:
			return "invalid color map depth";
		case TGAERR_ZERO_SIZE:
			return "the image dimensions are zero";
		case TGAERR_PIXEL_DEPTH:
			return "invalid pixel depth";
		case TGAERR_NO_MEM:
			return "out of memory";
		case TGAERR_NOT_CMAP:
			return "image is not color mapped";
		case TGAERR_RLE:
			return "RLE data is corrupt";
		case TGAERR_INDEX_RANGE:
			return "color map index out of range";
		case TGAERR_MONO:
			return "image is mono";
		default:
			return "unknown error code";
	}
}



/* ---------------------------------------------------------------------------
 * Read a Targa image from <fp> to <dest>.
 *
 * Returns: TGA_NOERR on success, or a TGAERR_* code on failure.  In the
 *          case of failure, the contents of dest are not guaranteed to be
 *          valid.
 */
tga_result tga_read_from_FILE(tga_image *dest, std::istream& fp)
{
#define BARF(errcode) \
{ tga_free_buffers(dest);  return errcode; }

#define READ(destptr, size) \
if(fp.readsome(reinterpret_cast<char*>(destptr), size) < size) BARF(TGAERR_EOF)
	//if (fread(destptr, size, 1, fp) != 1) BARF(TGAERR_EOF)

#define READ16(dest) \
{ if (fp.readsome(reinterpret_cast<char*>(&dest), 2) != 2) BARF(TGAERR_EOF); \
dest = letoh16(dest); }
	//{ if (fread(&(dest), 2, 1, fp) != 1) BARF(TGAERR_EOF); \
	//dest = letoh16(dest); }

	dest->image_id = NULL;
	dest->color_map_data = NULL;
	dest->image_data = NULL;

	READ(&dest->image_id_length,1);
	READ(&dest->color_map_type,1);
	if (dest->color_map_type != TGA_COLOR_MAP_ABSENT &&
		dest->color_map_type != TGA_COLOR_MAP_PRESENT)
		BARF(TGAERR_CMAP_TYPE);

	READ(&dest->image_type, 1);
	if (dest->image_type == TGA_IMAGE_TYPE_NONE)
		BARF(TGAERR_NO_IMG);

	if (dest->image_type != TGA_IMAGE_TYPE_COLORMAP &&
		dest->image_type != TGA_IMAGE_TYPE_BGR &&
		dest->image_type != TGA_IMAGE_TYPE_MONO &&
		dest->image_type != TGA_IMAGE_TYPE_COLORMAP_RLE &&
		dest->image_type != TGA_IMAGE_TYPE_BGR_RLE &&
		dest->image_type != TGA_IMAGE_TYPE_MONO_RLE)
		BARF(TGAERR_IMG_TYPE);

	if (tga_is_colormapped(dest) &&
		dest->color_map_type == TGA_COLOR_MAP_ABSENT)
		BARF(TGAERR_CMAP_MISSING);

	if (!tga_is_colormapped(dest) &&
		dest->color_map_type == TGA_COLOR_MAP_PRESENT)
		BARF(TGAERR_CMAP_PRESENT);

	READ16(dest->color_map_origin);
	READ16(dest->color_map_length);
	READ(&dest->color_map_depth, 1);
	if (dest->color_map_type == TGA_COLOR_MAP_PRESENT)
	{
		if (dest->color_map_length == 0)
			BARF(TGAERR_CMAP_LENGTH);

		if (!UNMAP_DEPTH(dest->color_map_depth))
			BARF(TGAERR_CMAP_DEPTH);
	}

	READ16(dest->origin_x);
	READ16(dest->origin_y);
	READ16(dest->width);
	READ16(dest->height);

	if (dest->width == 0 || dest->height == 0)
		BARF(TGAERR_ZERO_SIZE);

	READ(&dest->pixel_depth, 1);
	if (!SANE_DEPTH(dest->pixel_depth) ||
		(dest->pixel_depth != 8 && tga_is_colormapped(dest)) )
		BARF(TGAERR_PIXEL_DEPTH);

	READ(&dest->image_descriptor, 1);

	if (dest->image_id_length > 0)
	{
		dest->image_id = (uint8_t*)malloc(dest->image_id_length);
		if (dest->image_id == NULL) BARF(TGAERR_NO_MEM);
		READ(dest->image_id, dest->image_id_length);
	}

	if (dest->color_map_type == TGA_COLOR_MAP_PRESENT)
	{
		dest->color_map_data = (uint8_t*)malloc(
												(dest->color_map_origin + dest->color_map_length) *
												dest->color_map_depth / 8);
		if (dest->color_map_data == NULL) BARF(TGAERR_NO_MEM);
		READ(dest->color_map_data +
			 (dest->color_map_origin * dest->color_map_depth / 8),
			 dest->color_map_length * dest->color_map_depth / 8);
	}

	dest->image_data = (uint8_t*) malloc(
										 dest->width * dest->height * dest->pixel_depth / 8);
	if (dest->image_data == NULL)
		BARF(TGAERR_NO_MEM);

	if (tga_is_rle(dest))
	{
		/* read RLE */
		tga_result result = tga_read_rle(dest, fp);
		if (result != TGA_NOERR) BARF(result);
	}
	else
	{
		/* uncompressed */
		READ(dest->image_data,
			 dest->width * dest->height * dest->pixel_depth / 8);
	}

	return TGA_NOERR;
#undef BARF
#undef READ
#undef READ16
}



/* ---------------------------------------------------------------------------
 * Helper function for tga_read_from_FILE().  Decompresses RLE image data from
 * <fp>.  Assumes <dest> header fields are set correctly.
 */
static tga_result tga_read_rle(tga_image *dest, std::istream& fp)
{
#define RLE_BIT BIT(7)
#define READ(dest, size) \
if (fp.readsome(reinterpret_cast<char*>(dest), size) != size) return TGAERR_EOF

	uint8_t *pos;
	uint32_t p_loaded = 0,
	p_expected = dest->width * dest->height;
	uint8_t bpp = dest->pixel_depth/8; /* bytes per pixel */

	pos = dest->image_data;

	while ((p_loaded < p_expected)/* && !feof(fp)*/)
	{
		uint8_t b;
		READ(&b, 1);
		if (b & RLE_BIT)
		{
			/* is an RLE packet */
			uint8_t count, tmp[4], i;

			count = (b & ~RLE_BIT) + 1;
			READ(tmp, bpp);

			for (i=0; i<count; i++)
			{
				p_loaded++;
				if (p_loaded > p_expected) return TGAERR_RLE;
				memcpy(pos, tmp, bpp);
				pos += bpp;
			}
		}
		else /* RAW packet */
		{
			uint8_t count;

			count = (b & ~RLE_BIT) + 1;
			if (p_loaded + count > p_expected) return TGAERR_RLE;

			p_loaded += count;
			READ(pos, bpp*count);
			pos += count * bpp;
		}
	}
	return TGA_NOERR;
#undef RLE_BIT
#undef READ
}




/* ---------------------------------------------------------------------------
 * Write one row of an image to <fp> using RLE.  This is a helper function
 * called from tga_write_to_FILE().  It assumes that <src> has its header
 * fields set up correctly.
 */
#define PIXEL(ofs) ( row + (ofs)*bpp )
static tga_result tga_write_row_RLE(std::ostream& fp,
									const tga_image *src, const uint8_t *row)
{
#define WRITE(src, size) \
fp.write(reinterpret_cast<const char *>(src), size)

	//if (fwrite(src, size, 1, fp) != 1) return TGAERR_WRITE

	uint16_t pos = 0;
	uint16_t bpp = src->pixel_depth / 8;

	while (pos < src->width)
	{
		packet_type type = rle_packet_type(row, pos, src->width, bpp);
		uint8_t len = rle_packet_len(row, pos, src->width, bpp, type);
		uint8_t packet_header;

		packet_header = len - 1;
		if (type == RLE) packet_header |= BIT(7);

		WRITE(&packet_header, 1);
		if (type == RLE)
		{
			WRITE(PIXEL(pos), bpp);
		}
		else /* type == RAW */
		{
			WRITE(PIXEL(pos), bpp*len);
		}

		pos += len;
	}

	return TGA_NOERR;
#undef WRITE
}



/* ---------------------------------------------------------------------------
 * Determine whether the next packet should be RAW or RLE for maximum
 * efficiency.  This is a helper function called from rle_packet_len() and
 * tga_write_row_RLE().
 */
#define SAME(ofs1, ofs2) (memcmp(PIXEL(ofs1), PIXEL(ofs2), bpp) == 0)

static packet_type rle_packet_type(const uint8_t *row, const uint16_t pos,
								   const uint16_t width, const uint16_t bpp)
{
	if (pos == width - 1) return RAW; /* one pixel */
	if (SAME(pos,pos+1)) /* dupe pixel */
	{
		if (bpp > 1) return RLE; /* inefficient for bpp=1 */

		/* three repeats makes the bpp=1 case efficient enough */
		if ((pos < width - 2) && SAME(pos+1,pos+2)) return RLE;
	}
	return RAW;
}



/* ---------------------------------------------------------------------------
 * Find the length of the current RLE packet.  This is a helper function
 * called from tga_write_row_RLE().
 */
static uint8_t rle_packet_len(const uint8_t *row, const uint16_t pos,
							  const uint16_t width, const uint16_t bpp, const packet_type type)
{
	uint8_t len = 2;

	if (pos == width - 1) return 1;
	if (pos == width - 2) return 2;

	if (type == RLE)
	{
		while (pos + len < width)
		{
			if (SAME(pos, pos+len))
				len++;
			else
				return len;

			if (len == 128) return 128;
		}
	}
	else /* type == RAW */
	{
		while (pos + len < width)
		{
			if (rle_packet_type(row, pos+len, width, bpp) == RAW)
				len++;
			else
				return len;
			if (len == 128) return 128;
		}
	}
	return len; /* hit end of row (width) */
}
#undef SAME
#undef PIXEL



/* ---------------------------------------------------------------------------
 * Writes a Targa image to <fp> from <src>.
 *
 * Returns: TGA_NOERR on success, or a TGAERR_* code on failure.
 *          On failure, the contents of the file are not guaranteed
 *          to be valid.
 */
tga_result tga_write_to_FILE(std::ostream& fp, const tga_image *src)
{
#define WRITE(srcptr, size) \
fp.write(reinterpret_cast<const char *>(srcptr), size)
//fp->Write(srcptr, size)
	//if (fwrite(srcptr, size, 1, fp) != 1) return TGAERR_WRITE

#define WRITE16(src) \
{ uint16_t _temp = htole16(src); \
fp.write(reinterpret_cast<const char *>(&_temp), 2); }

	//if (fwrite(&_temp, 2, 1, fp) != 1) return TGAERR_WRITE; }

	WRITE(&src->image_id_length, 1);

	if (src->color_map_type != TGA_COLOR_MAP_ABSENT &&
		src->color_map_type != TGA_COLOR_MAP_PRESENT)
		return TGAERR_CMAP_TYPE;
	WRITE(&src->color_map_type, 1);

	if (src->image_type == TGA_IMAGE_TYPE_NONE)
		return TGAERR_NO_IMG;
	if (src->image_type != TGA_IMAGE_TYPE_COLORMAP &&
		src->image_type != TGA_IMAGE_TYPE_BGR &&
		src->image_type != TGA_IMAGE_TYPE_MONO &&
		src->image_type != TGA_IMAGE_TYPE_COLORMAP_RLE &&
		src->image_type != TGA_IMAGE_TYPE_BGR_RLE &&
		src->image_type != TGA_IMAGE_TYPE_MONO_RLE)
		return TGAERR_IMG_TYPE;
	WRITE(&src->image_type, 1);

	if (tga_is_colormapped(src) &&
		src->color_map_type == TGA_COLOR_MAP_ABSENT)
		return TGAERR_CMAP_MISSING;
	if (!tga_is_colormapped(src) &&
		src->color_map_type == TGA_COLOR_MAP_PRESENT)
		return TGAERR_CMAP_PRESENT;
	if (src->color_map_type == TGA_COLOR_MAP_PRESENT)
	{
		if (src->color_map_length == 0)
			return TGAERR_CMAP_LENGTH;

		if (!UNMAP_DEPTH(src->color_map_depth))
			return TGAERR_CMAP_DEPTH;
	}
	WRITE16(src->color_map_origin);
	WRITE16(src->color_map_length);
	WRITE(&src->color_map_depth, 1);

	WRITE16(src->origin_x);
	WRITE16(src->origin_y);

	if (src->width == 0 || src->height == 0)
		return TGAERR_ZERO_SIZE;
	WRITE16(src->width);
	WRITE16(src->height);

	if (!SANE_DEPTH(src->pixel_depth) ||
		(src->pixel_depth != 8 && tga_is_colormapped(src)) )
		return TGAERR_PIXEL_DEPTH;
	WRITE(&src->pixel_depth, 1);

	WRITE(&src->image_descriptor, 1);

	if (src->image_id_length > 0)
		WRITE(&src->image_id, src->image_id_length);

	if (src->color_map_type == TGA_COLOR_MAP_PRESENT)
		WRITE(src->color_map_data +
			  (src->color_map_origin * src->color_map_depth / 8),
			  src->color_map_length * src->color_map_depth / 8);

	if (tga_is_rle(src))
	{
		uint16_t row;
		for (row=0; row<src->height; row++)
		{
			tga_result result = tga_write_row_RLE(fp, src,
												  src->image_data + row*src->width*src->pixel_depth/8);
			if (result != TGA_NOERR) return result;
		}
	}
	else
	{
		/* uncompressed */
		WRITE(src->image_data,
			  src->width * src->height * src->pixel_depth / 8);
	}

	WRITE(tga_id, tga_id_length);

	return TGA_NOERR;
#undef WRITE
#undef WRITE16
}



/* Convenient writing functions --------------------------------------------*/

/*
 * This is just a helper function to initialise the header fields in a
 * tga_image struct.
 */
static void init_tga_image(tga_image *img, uint8_t *image,
						   const uint16_t width, const uint16_t height, const uint8_t depth)
{
	img->image_id_length = 0;
	img->color_map_type = TGA_COLOR_MAP_ABSENT;
	img->image_type = TGA_IMAGE_TYPE_NONE; /* override this below! */
	img->color_map_origin = 0;
	img->color_map_length = 0;
	img->color_map_depth = 0;
	img->origin_x = 0;
	img->origin_y = 0;
	img->width = width;
	img->height = height;
	img->pixel_depth = depth;
	img->image_descriptor = TGA_T_TO_B_BIT;
	img->image_id = NULL;
	img->color_map_data = NULL;
	img->image_data = image;
}
/*


 tga_result tga_write_mono(const char *filename, uint8_t *image,
 const uint16_t width, const uint16_t height)
 {
 tga_image img;
 init_tga_image(&img, image, width, height, 8);
 img.image_type = TGA_IMAGE_TYPE_MONO;
 return tga_write(filename, &img);
 }



 tga_result tga_write_mono_rle(const char *filename, uint8_t *image,
 const uint16_t width, const uint16_t height)
 {
 tga_image img;
 init_tga_image(&img, image, width, height, 8);
 img.image_type = TGA_IMAGE_TYPE_MONO_RLE;
 return tga_write(filename, &img);
 }



 tga_result tga_write_bgr(const char *filename, uint8_t *image,
 const uint16_t width, const uint16_t height, const uint8_t depth)
 {
 tga_image img;
 init_tga_image(&img, image, width, height, depth);
 img.image_type = TGA_IMAGE_TYPE_BGR;
 return tga_write(filename, &img);
 }



 tga_result tga_write_bgr_rle(const char *filename, uint8_t *image,
 const uint16_t width, const uint16_t height, const uint8_t depth)
 {
 tga_image img;
 init_tga_image(&img, image, width, height, depth);
 img.image_type = TGA_IMAGE_TYPE_BGR_RLE;
 return tga_write(filename, &img);
 }
 */


/* Note: this function will MODIFY <image> */
/*tga_result tga_write_rgb(const char *filename, uint8_t *image,
 const uint16_t width, const uint16_t height, const uint8_t depth)
 {
 tga_image img;
 init_tga_image(&img, image, width, height, depth);
 img.image_type = TGA_IMAGE_TYPE_BGR;
 (void)tga_swap_red_blue(&img);
 return tga_write(filename, &img);
 }
 */


/* Note: this function will MODIFY <image> */
/*tga_result tga_write_rgb_rle(const char *filename, uint8_t *image,
 const uint16_t width, const uint16_t height, const uint8_t depth)
 {
 tga_image img;
 init_tga_image(&img, image, width, height, depth);
 img.image_type = TGA_IMAGE_TYPE_BGR_RLE;
 (void)tga_swap_red_blue(&img);
 return tga_write(filename, &img);
 }
 */


/* Convenient manipulation functions ---------------------------------------*/

/* ---------------------------------------------------------------------------
 * Horizontally flip the image in place.  Reverses the right-to-left bit in
 * the image descriptor.
 */
tga_result tga_flip_horiz(tga_image *img)
{
	uint16_t row;
	size_t bpp;
	uint8_t *left, *right;
	int r_to_l;

	if (!SANE_DEPTH(img->pixel_depth)) return TGAERR_PIXEL_DEPTH;
	bpp = (size_t)(img->pixel_depth / 8); /* bytes per pixel */

	for (row=0; row<img->height; row++)
	{
		left = img->image_data + row * img->width * bpp;
		right = left + (img->width - 1) * bpp;

		/* reverse from left to right */
		while (left < right)
		{
			uint8_t buffer[4];

			/* swap */
			memcpy(buffer, left, bpp);
			memcpy(left, right, bpp);
			memcpy(right, buffer, bpp);

			left += bpp;
			right -= bpp;
		}
	}

	/* Correct image_descriptor's left-to-right-ness. */
	r_to_l = tga_is_right_to_left(img);
	img->image_descriptor &= ~TGA_R_TO_L_BIT; /* mask out r-to-l bit */
	if (!r_to_l)
	/* was l-to-r, need to set r_to_l */
		img->image_descriptor |= TGA_R_TO_L_BIT;
	/* else bit is already rubbed out */

	return TGA_NOERR;
}



/* ---------------------------------------------------------------------------
 * Vertically flip the image in place.  Reverses the top-to-bottom bit in
 * the image descriptor.
 */
tga_result tga_flip_vert(tga_image *img)
{
	uint16_t col;
	size_t bpp, line;
	uint8_t *top, *bottom;
	int t_to_b;

	if (!SANE_DEPTH(img->pixel_depth)) return TGAERR_PIXEL_DEPTH;
	bpp = (size_t)(img->pixel_depth / 8);   /* bytes per pixel */
	line = bpp * img->width;                /* bytes per line */

	for (col=0; col<img->width; col++)
	{
		top = img->image_data + col * bpp;
		bottom = top + (img->height - 1) * line;

		/* reverse from top to bottom */
		while (top < bottom)
		{
			uint8_t buffer[4];

			/* swap */
			memcpy(buffer, top, bpp);
			memcpy(top, bottom, bpp);
			memcpy(bottom, buffer, bpp);

			top += line;
			bottom -= line;
		}
	}

	/* Correct image_descriptor's top-to-bottom-ness. */
	t_to_b = tga_is_top_to_bottom(img);
	img->image_descriptor &= ~TGA_T_TO_B_BIT; /* mask out t-to-b bit */
	if (!t_to_b)
	/* was b-to-t, need to set t_to_b */
		img->image_descriptor |= TGA_T_TO_B_BIT;
	/* else bit is already rubbed out */

	return TGA_NOERR;
}



/* ---------------------------------------------------------------------------
 * Convert a color-mapped image to unmapped BGR.  Reallocates image_data to a
 * bigger size, then converts the image backwards to avoid using a secondary
 * buffer.  Alters the necessary header fields and deallocates the color map.
 */
tga_result tga_color_unmap(tga_image *img)
{
	uint8_t bpp = img->color_map_depth / 8; /* bytes per pixel */
	int pos;
	void *tmp;

	if (!tga_is_colormapped(img)) return TGAERR_NOT_CMAP;
	if (img->pixel_depth != 8) return TGAERR_PIXEL_DEPTH;
	if (!SANE_DEPTH(img->color_map_depth)) return TGAERR_CMAP_DEPTH;

	tmp = realloc(img->image_data, img->width * img->height * bpp);
	if (tmp == NULL) return TGAERR_NO_MEM;
	img->image_data = (uint8_t*) tmp;

	for (pos = img->width * img->height - 1; pos >= 0; pos--)
	{
		uint8_t c_index = img->image_data[pos];
		uint8_t *c_bgr = img->color_map_data + (c_index * bpp);

		if (c_index >= img->color_map_origin + img->color_map_length)
			return TGAERR_INDEX_RANGE;

		memcpy(img->image_data + (pos*bpp), c_bgr, (size_t)bpp);
	}

	/* clean up */
	img->image_type = TGA_IMAGE_TYPE_BGR;
	img->pixel_depth = img->color_map_depth;

	free(img->color_map_data);
	img->color_map_data = NULL;
	img->color_map_type = TGA_COLOR_MAP_ABSENT;
	img->color_map_origin = 0;
	img->color_map_length = 0;
	img->color_map_depth = 0;

	return TGA_NOERR;
}



/* ---------------------------------------------------------------------------
 * Return a pointer to a given pixel.  Accounts for image orientation (T_TO_B,
 * R_TO_L, etc).  Returns NULL if the pixel is out of range.
 */
uint8_t *tga_find_pixel(const tga_image *img, uint16_t x, uint16_t y)
{
	if (x >= img->width || y >= img->height)
		return NULL;

	if (!tga_is_top_to_bottom(img)) y = img->height - 1 - y;
	if (tga_is_right_to_left(img)) x = img->width - 1 - x;
	return img->image_data + (x + y * img->width) * img->pixel_depth/8;
}



/* ---------------------------------------------------------------------------
 * Unpack the pixel at the src pointer according to bits.  Any of b,g,r,a can
 * be set to NULL if not wanted.  Returns TGAERR_PIXEL_DEPTH if a stupid
 * number of bits is given.
 */
tga_result tga_unpack_pixel(const uint8_t *src, const uint8_t bits,
							uint8_t *b, uint8_t *g, uint8_t *r, uint8_t *a)
{
	switch (bits)
	{
		case 32:
			if (b) *b = src[0];
			if (g) *g = src[1];
			if (r) *r = src[2];
			if (a) *a = src[3];
			break;

		case 24:
			if (b) *b = src[0];
			if (g) *g = src[1];
			if (r) *r = src[2];
			if (a) *a = 0;
			break;

		case 16:
		{
			uint16_t src16 = (uint16_t)(src[1] << 8) | (uint16_t)src[0];

#define FIVE_BITS (BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4))
			if (b) *b = ((src16      ) & FIVE_BITS) << 3;
			if (g) *g = ((src16 >>  5) & FIVE_BITS) << 3;
			if (r) *r = ((src16 >> 10) & FIVE_BITS) << 3;
			if (a) *a = (uint8_t)( (src16 & BIT(15)) ? 255 : 0 );
#undef FIVE_BITS
			break;
		}

		case 8:
			if (b) *b = *src;
			if (g) *g = *src;
			if (r) *r = *src;
			if (a) *a = 0;
			break;

		default:
			return TGAERR_PIXEL_DEPTH;
	}
	return TGA_NOERR;
}



/* ---------------------------------------------------------------------------
 * Pack the pixel at the dest pointer according to bits.  Returns
 * TGAERR_PIXEL_DEPTH if a stupid number of bits is given.
 */
tga_result tga_pack_pixel(uint8_t *dest, const uint8_t bits,
						  const uint8_t b, const uint8_t g, const uint8_t r, const uint8_t a)
{
	switch (bits)
	{
		case 32:
			dest[0] = b;
			dest[1] = g;
			dest[2] = r;
			dest[3] = a;
			break;

		case 24:
			dest[0] = b;
			dest[1] = g;
			dest[2] = r;
			break;

		case 16:
		{
			uint16_t tmp;

#define FIVE_BITS (BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4))
			tmp  =  (b >> 3) & FIVE_BITS;
			tmp |= ((g >> 3) & FIVE_BITS) << 5;
			tmp |= ((r >> 3) & FIVE_BITS) << 10;
			if (a > 127) tmp |= BIT(15);
#undef FIVE_BITS

			dest[0] = (uint8_t) (tmp & 0x00FF);
			dest[1] = (uint8_t)((tmp & 0xFF00) >> 8);
			break;
		}

		default:
			return TGAERR_PIXEL_DEPTH;
	}
	return TGA_NOERR;
}



/* ---------------------------------------------------------------------------
 * Desaturate the specified Targa using the specified coefficients:
 *      output = ( red * cr + green * cg + blue * cb ) / dv
 */
tga_result tga_desaturate(tga_image *img, const int cr, const int cg,
						  const int cb, const int dv)
{
	uint8_t bpp = img->pixel_depth / 8; /* bytes per pixel */
	uint8_t *dest, *src, *tmp;

	if (tga_is_mono(img)) return TGAERR_MONO;
	if (tga_is_colormapped(img))
	{
		tga_result result = tga_color_unmap(img);
		if (result != TGA_NOERR) return result;
	}
	if (!UNMAP_DEPTH(img->pixel_depth)) return TGAERR_PIXEL_DEPTH;

	dest = img->image_data;
	for (src = img->image_data;
		 src < img->image_data + img->width*img->height*bpp;
		 src += bpp)
	{
		uint8_t b, g, r;
		(void)tga_unpack_pixel(src, img->pixel_depth, &b, &g, &r, NULL);

		*dest = (uint8_t)( ( (int)b * cb +
							(int)g * cg +
							(int)r * cr ) / dv );
		dest++;
	}

	/* shrink */
	tmp = (uint8_t *)realloc(img->image_data, img->width * img->height);
	if (tmp == NULL) return TGAERR_NO_MEM;
	img->image_data = tmp;

	img->pixel_depth = 8;
	img->image_type = TGA_IMAGE_TYPE_MONO;
	return TGA_NOERR;
}

tga_result tga_desaturate_rec_601_1(tga_image *img)
{
	return tga_desaturate(img, 2989, 5866, 1145, 10000);
}

tga_result tga_desaturate_rec_709(tga_image *img)
{
	return tga_desaturate(img, 2126, 7152, 722, 10000);
}

tga_result tga_desaturate_itu(tga_image *img)
{
	return tga_desaturate(img, 2220, 7067, 713, 10000);
}

tga_result tga_desaturate_avg(tga_image *img)
{
	return tga_desaturate(img, 1,1,1, 3);
}



/* ---------------------------------------------------------------------------
 * Convert an image to the given pixel depth. (one of 32, 24, 16)  Avoids
 * using a secondary buffer to do the conversion.
 */
tga_result tga_convert_depth(tga_image *img, const uint8_t bits)
{
	size_t src_size, dest_size;
	uint8_t src_bpp, dest_bpp;
	uint8_t *src, *dest;

	if (!UNMAP_DEPTH(bits) ||
		!SANE_DEPTH(img->pixel_depth)
		)    return TGAERR_PIXEL_DEPTH;

	if (tga_is_colormapped(img))
	{
		tga_result result = tga_color_unmap(img);
		if (result != TGA_NOERR) return result;
	}

	if (img->pixel_depth == bits) return TGA_NOERR; /* no op, no err */

	src_bpp = img->pixel_depth / 8;
	dest_bpp = bits / 8;

	src_size  = (size_t)( img->width * img->height * src_bpp );
	dest_size = (size_t)( img->width * img->height * dest_bpp );

	if (src_size > dest_size)
	{
		void *tmp;

		/* convert forwards */
		dest = img->image_data;
		for (src = img->image_data;
			 src < img->image_data + img->width * img->height * src_bpp;
			 src += src_bpp)
		{
			uint8_t r,g,b,a;
			(void)tga_unpack_pixel(src, img->pixel_depth, &r, &g, &b, &a);
			(void)tga_pack_pixel(dest, bits, r, g, b, a);
			dest += dest_bpp;
		}

		/* shrink */
		tmp = realloc(img->image_data, img->width * img->height * dest_bpp);
		if (tmp == NULL) return TGAERR_NO_MEM;
		img->image_data = (unsigned char *)tmp;
	}
	else
	{
		/* expand */
		void *tmp = realloc(img->image_data,
							img->width * img->height * dest_bpp);
		if (tmp == NULL) return TGAERR_NO_MEM;
		img->image_data = (uint8_t*) tmp;

		/* convert backwards */
		dest = img->image_data + (img->width*img->height - 1) * dest_bpp;
		for (src = img->image_data + (img->width*img->height - 1) * src_bpp;
			 src >= img->image_data;
			 src -= src_bpp)
		{
			uint8_t r,g,b,a;
			(void)tga_unpack_pixel(src, img->pixel_depth, &r, &g, &b, &a);
			(void)tga_pack_pixel(dest, bits, r, g, b, a);
			dest -= dest_bpp;
		}
	}

	img->pixel_depth = bits;
	return TGA_NOERR;
}



/* ---------------------------------------------------------------------------
 * Swap red and blue (RGB becomes BGR and vice verse).  (in-place)
 */
tga_result tga_swap_red_blue(tga_image *img)
{
	uint8_t *ptr;
	uint8_t bpp = img->pixel_depth / 8;

	if (!UNMAP_DEPTH(img->pixel_depth)) return TGAERR_PIXEL_DEPTH;

	for (ptr = img->image_data;
		 ptr < img->image_data + (img->width * img->height - 1) * bpp;
		 ptr += bpp)
	{
		uint8_t r,g,b,a;
		(void)tga_unpack_pixel(ptr, img->pixel_depth, &b,&g,&r,&a);
		(void)tga_pack_pixel(ptr, img->pixel_depth, r,g,b,a);
	}
	return TGA_NOERR;
}



/* ---------------------------------------------------------------------------
 * Free the image_id, color_map_data and image_data buffers of the specified
 * tga_image, if they're not already NULL.
 */
void tga_free_buffers(tga_image *img)
{
	if (img->image_id != NULL)
	{
		free(img->image_id);
		img->image_id = NULL;
	}
	if (img->color_map_data != NULL)
	{
		free(img->color_map_data);
		img->color_map_data = NULL;
	}
	if (img->image_data != NULL)
	{
		free(img->image_data);
		img->image_data = NULL;
	}
}

/*
#pragma mark - Interface


namespace spades {
	class TargaWriter: public IBitmapCodec {
	public:
		virtual bool CanLoad(){
			return false;
		}
		virtual bool CanSave(){
			return true;
		}

		virtual bool CheckExtension(const std::string& filename){
			return EndsWith(filename, ".tga");
		}

		virtual std::string GetName(){
			static std::string name("dmr.ath.cx Targa Exporter");
			return name;
		}

		virtual Bitmap *Load(IStream *str){
			SPADES_MARK_FUNCTION();

			SPNotImplemented();
		}
		virtual void Save(IStream *stream, Bitmap *bmp){
			SPADES_MARK_FUNCTION();

			tga_image img;
			std::vector<uint8_t> data;
			data.resize(bmp->GetWidth() * bmp->GetHeight() * 4);
			memcpy(data.data(), bmp->GetPixels(),
				   data.size());
			init_tga_image(&img, (uint8_t *)data.data(),
						   bmp->GetWidth(), bmp->GetHeight(),
						   32);
			img.image_type = TGA_IMAGE_TYPE_BGR_RLE;
			(void)tga_swap_red_blue(&img);
			//(void)tga_flip_vert(&img);
			img.image_descriptor ^= TGA_T_TO_B_BIT;

			tga_result result;
			result = tga_write_to_FILE(stream, &img);

			if(result != TGA_NOERR){
				SPRaise("Targa exporter library failure: %s", tga_error(result));
			}
		}
	};

	static TargaWriter sharedCodec;

}

*/

class Bitmap {
	std::vector<std::uint8_t> pixels;
	int w, h;

public:
	Bitmap(int w = 0, int h = 0) : w(w), h(h) { pixels.resize(w * h); }
	int GetWidth() const { return w; }
	int GetHeight() const { return h; }
	std::uint8_t &operator()(int x, int y) {
		assert(x >= 0);
		assert(y >= 0);
		assert(x < w);
		assert(y < h);
		return pixels[x + y * w];
	}
	std::uint8_t const &operator()(int x, int y) const {
		assert(x >= 0);
		assert(y >= 0);
		assert(x < w);
		assert(y < h);
		return pixels[x + y * w];
	}
	void Resize(int neww, int newh) {
		assert(neww >= 0);
		assert(newh >= 0);
		if (neww == w) {
			pixels.resize(newh * neww);
		} else {
			std::vector<std::uint8_t> newPixels;
			newPixels.resize(neww * newh);
			for (int y = 0; y < newh; y++) {
				if (y < h) {
					int inIndex = y * w;
					int outIndex = y * neww;
					for (int i = std::min(w, neww); i > 0; i--) {
						newPixels[outIndex++] = pixels[inIndex++];
					}
				}
			}
			newPixels.swap(pixels);
		}
		w = neww;
		h = newh;
	}
	void Draw(const Bitmap &bmp, int dx, int dy) {
		int sw = bmp.w, sh = bmp.h;
		int dw = w, dh = h;
		for (int x = 0; x < sw; x++)
			for (int y = 0; y < sh; y++)
				(*this)(x + dx, y + dy) = bmp(x, y);
	}
	void Trim(Bitmap &outBitmap, int &srcX, int &srcY) {
		int w = this->w, h = this->h;
		int minX;
		for (minX = 0; minX < w; minX++) {
			int y;
			for (y = 0; y < h; y++)
				if ((*this)(minX, y))
					break;
			if (y < h)
				break;
		}
		if (minX == w) {
			// empty
			outBitmap.Resize(0, 0);
			srcX = 0;
			srcY = 0;
			return;
		}
		int maxX;
		for (maxX = w - 1; maxX >= 0; maxX--) {
			int y;
			for (y = 0; y < h; y++)
				if ((*this)(maxX, y))
					break;
			if (y < h)
				break;
		}
		assert(maxX >= 0);
		int minY;
		for (minY = 0; minY < h; minY++) {
			int x;
			for (x = 0; x < w; x++)
				if ((*this)(x, minY))
					break;
			if (x < w)
				break;
		}
		assert(minY < h);
		int maxY;
		for (maxY = h - 1; maxY >= 0; maxY--) {
			int x;
			for (x = 0; x < w; x++)
				if ((*this)(x, maxY))
					break;
			if (x < w)
				break;
		}
		assert(maxY >= 0);
		outBitmap.Resize(maxX - minX + 1, maxY - minY + 1);

		int nw = maxX - minX + 1;
		int nh = maxY - minY + 1;
		for (int x = 0; x < nw; x++) {
			for (int y = 0; y < nh; y++) {
				outBitmap(x, y) = (*this)(x + minX, y + minY);
			}
		}

		srcX = minX;
		srcY = minY;
	}
};

template <typename IndexType> class AtlasBuilder {
public:
	struct Item {
		IndexType index;
		int x, y, w, h;
		Item() = default;
		Item(const IndexType &idx, int w, int h) : index(idx), w(w), h(h) {}
	};

private:
	std::unordered_map<IndexType, Item> items;

public:
	AtlasBuilder() {}

	~AtlasBuilder() {}
	AtlasBuilder(const AtlasBuilder &) = delete;
	AtlasBuilder &operator=(const AtlasBuilder &) = delete;

	bool TryPack(int binWidth, int binHeight) {
		int shelfTop = 0, shelfHeight = 0;
		int shelfRight = 0;
		for (auto &it : items) {
			Item &item = it.second;
			if (shelfRight + item.w > binWidth) {
				// new shelf
				shelfRight = 0;
				shelfTop += shelfHeight;
				shelfHeight = 0;
			}
			if (item.w > binWidth) {
				// impossible to pack
				return false;
			}
			shelfHeight = std::max(shelfHeight, item.h);
			item.x = shelfRight;
			item.y = shelfTop;
			shelfRight += item.w;
			if (shelfTop + shelfHeight > binHeight) {
				// overflow.
				return false;
			}
		}
		return true;
	}

	void AddItem(const IndexType &index, int w, int h) { items[index] = Item(index, w, h); }

	const Item &GetItem(const IndexType &index) const { return items.find(index)->second; }
};

class CharsetDecoder {
public:
	virtual ~CharsetDecoder() {}
	virtual char32_t Decode(const std::string &) = 0;
};

#include <iconv.h>

class JISDecoder : public CharsetDecoder {

	iconv_t cd;

public:
	JISDecoder() { cd = iconv_open("UTF-32LE", "JIS0208"); }
	~JISDecoder() { iconv_close(cd); }
	virtual char32_t Decode(const std::string &str) {
		int index = std::stoi(str.substr(2), 0, 16);

		// non-standard JIS chars (called "機種依存文字")
		// iconv won't convert them...
		switch (index) {
			case 0x2d21: return L'①';
			case 0x2d22: return L'②';
			case 0x2d23: return L'③';
			case 0x2d24: return L'④';
			case 0x2d25: return L'⑤';
			case 0x2d26: return L'⑥';
			case 0x2d27: return L'⑦';
			case 0x2d28: return L'⑧';
			case 0x2d29: return L'⑨';
			case 0x2d2a: return L'⑩';
			case 0x2d2b: return L'⑪';
			case 0x2d2c: return L'⑫';
			case 0x2d2d: return L'⑬';
			case 0x2d2e: return L'⑭';
			case 0x2d2f: return L'⑮';
			case 0x2d30: return L'⑯';
			case 0x2d31: return L'⑰';
			case 0x2d32: return L'⑱';
			case 0x2d33: return L'⑲';
			case 0x2d34: return L'⑳';
			case 0x2d35: return L'Ⅰ';
			case 0x2d36: return L'Ⅱ';
			case 0x2d37: return L'Ⅲ';
			case 0x2d38: return L'Ⅳ';
			case 0x2d39: return L'Ⅴ';
			case 0x2d3a: return L'Ⅵ';
			case 0x2d3b: return L'Ⅶ';
			case 0x2d3c: return L'Ⅷ';
			case 0x2d3d: return L'Ⅸ';
			case 0x2d3e: return L'Ⅹ';
			case 0x2d3f:
				break; // no code
			case 0x2d40: return L'㍉';
			case 0x2d41: return L'㌔';
			case 0x2d42: return L'㌢';
			case 0x2d43: return L'㍍';
			case 0x2d44: return L'㌘';
			case 0x2d45: return L'㌧';
			case 0x2d46: return L'㌃';
			case 0x2d47: return L'㌶';
			case 0x2d48: return L'㍑';
			case 0x2d49: return L'㍗';
			case 0x2d4a: return L'㌍';
			case 0x2d4b: return L'㌦';
			case 0x2d4c: return L'㌣';
			case 0x2d4d: return L'㌫';
			case 0x2d4e: return L'㍊';
			case 0x2d4f: return L'㌻';
			case 0x2d50: return L'㎜';
			case 0x2d51: return L'㎝';
			case 0x2d52: return L'㎞';
			case 0x2d53: return L'㎎';
			case 0x2d54: return L'㎏';
			case 0x2d55: return L'㏄';
			case 0x2d56: return L'㎡';
			case 0x2d57:
				break; // no code
			case 0x2d58:
				break; // no code
			case 0x2d59:
				break; // no code
			case 0x2d5a:
				break; // no code
			case 0x2d5b:
				break; // no code
			case 0x2d5c:
				break; // no code
			case 0x2d5d:
				break; // no code
			case 0x2d5e:
				break; // no code
			case 0x2d5f: return L'㍻';
			case 0x2d60: return L'〝';
			case 0x2d61: return L'〟';
			case 0x2d62: return L'№';
			case 0x2d63: return L'㏍';
			case 0x2d64: return L'℡';
			case 0x2d65: return L'㊤';
			case 0x2d66: return L'㊥';
			case 0x2d67: return L'㊦';
			case 0x2d68: return L'㊧';
			case 0x2d69: return L'㊨';
			case 0x2d6a: return L'㈱';
			case 0x2d6b: return L'㈲';
			case 0x2d6c: return L'㈹';
			case 0x2d6d: return L'㍾';
			case 0x2d6e: return L'㍽';
			case 0x2d6f: return L'㍼';
			case 0x2d70: return L'≒';
			case 0x2d71: return L'≡';
			case 0x2d72: return L'∫';
			case 0x2d73: return L'∮';
			case 0x2d74: return L'∑';
			case 0x2d75: return L'√';
			case 0x2d76: return L'⊥';
			case 0x2d77: return L'∠';
			case 0x2d78: return L'∟';
			case 0x2d79: return L'⊿';
			case 0x2d7a: return L'∵';
			case 0x2d7b: return L'∩';
			case 0x2d7c: return L'∪';
			case 0x2d7d: break;
			case 0x2d7e: break;
		}

		char inBuf[2] = {(char)(index + 0x00), (char)((index >> 8) + 0x00)};
		std::swap(inBuf[0], inBuf[1]);
		size_t inBufLen = 2;
		unsigned char outBuf[8];
		size_t outBufLeft = 8;
		char *inBufPtr = inBuf;
		char *outBufPtr = reinterpret_cast<char *>(outBuf);
		iconv(cd, &inBufPtr, &inBufLen, &outBufPtr, &outBufLeft);
		if (outBufLeft == 8) {
			std::cerr << "failed to convert " << str << " (" << index << ") to Unicode (ignored)"
			          << std::endl;
			return 0;
		}

		int idx = outBuf[0];
		idx += static_cast<int>(outBuf[1]) << 8;
		idx += static_cast<int>(outBuf[2]) << 16;
		idx += static_cast<int>(outBuf[3]) << 24;

		return static_cast<char32_t>(idx);
	}
};

static int DecodeHexDigit(char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	std::cerr << "unknown hex digit: " << c << std::endl;
	abort();
}

struct Glyph {
	char32_t index;
	std::shared_ptr<Bitmap> bitmap;
	int advance;
	int x, y;
};

void ApplyDoubleFilter(Bitmap &bmp) {
	int w = bmp.GetWidth();
	int h = bmp.GetHeight();
	Bitmap tmp = bmp;
	bmp.Resize(w * 2, h * 2);
	for (int x = 0; x < w; x++) {
		for (int y = 0; y < h; y++) {
			auto v = tmp(x, y);
			bmp(x * 2, y * 2) = v;
			bmp(x * 2 + 1, y * 2) = v;
			bmp(x * 2, y * 2 + 1) = v;
			bmp(x * 2 + 1, y * 2 + 1) = v;
		}
	}
}

void ApplySoftFilter(Bitmap &bmp) {
	bmp.Resize(bmp.GetWidth() + 1, bmp.GetHeight() + 1);
	for (int y = 0; y < bmp.GetHeight(); y++) {
		int last = 0;
		for (int x = 0; x < bmp.GetWidth(); x++) {
			int vl = bmp(x, y);
			int l = vl;
			// vl += last >> 1;
			vl = (vl + last) >> 1;
			if (vl > 255)
				vl = 255;
			last = l;
			bmp(x, y) = vl;
		}
	}
	for (int x = 0; x < bmp.GetWidth(); x++) {
		int last = 0;
		for (int y = 0; y < bmp.GetHeight(); y++) {
			int vl = bmp(x, y);
			int l = vl;
			// vl += last >> 1;
			vl = (vl + last) >> 1;
			if (vl > 255)
				vl = 255;
			last = l;
			bmp(x, y) = vl;
		}
	}
	for (int y = 0; y < bmp.GetHeight(); y++) {
		for (int x = 0; x < bmp.GetWidth(); x++) {
			int vl = bmp(x, y);
			vl += vl;
			if (vl > 255)
				vl = 255;
			bmp(x, y) = vl;
		}
	}
}

void ApplyDilationFilter(Bitmap &bmp) {
	bmp.Resize(bmp.GetWidth() + 1, bmp.GetHeight() + 1);
	for (int y = 0; y < bmp.GetHeight(); y++) {
		int last = 0;
		for (int x = 0; x < bmp.GetWidth(); x++) {
			int vl = bmp(x, y);
			int l = vl;
			// vl += last >> 1;
			vl = (vl + last);
			if (vl > 255)
				vl = 255;
			last = l;
			bmp(x, y) = vl;
		}
	}
	for (int x = 0; x < bmp.GetWidth(); x++) {
		int last = 0;
		for (int y = 0; y < bmp.GetHeight(); y++) {
			int vl = bmp(x, y);
			int l = vl;
			// vl += last >> 1;
			vl = (vl + last);
			if (vl > 255)
				vl = 255;
			last = l;
			bmp(x, y) = vl;
		}
	}
}
// inversed morphological thinning filter.
// based on: http://yvt.jp/contour/
void ApplyThickenFilter(Bitmap &bmp) {
	int w = bmp.GetWidth();
	int h = bmp.GetHeight();
	Bitmap outBmp(w, h);
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			if (bmp(x, y) || x == 0 || y == 0 || x == w - 1 || y == h - 1) {
				outBmp(x, y) = bmp(x, y);
			} else {
				if (bmp(x - 1, y) == 0 && bmp(x - 1, y - 1) == 0 && bmp(x - 1, y + 1) == 0 &&
				    bmp(x + 1, y) && bmp(x + 1, y - 1) && bmp(x + 1, y + 1)) {
					outBmp(x, y) = 255;
				} else if (bmp(x + 1, y) == 0 && bmp(x + 1, y - 1) == 0 && bmp(x + 1, y + 1) == 0 &&
				           bmp(x - 1, y) && bmp(x - 1, y - 1) && bmp(x - 1, y + 1)) {
					outBmp(x, y) = 255;
				} else {
					outBmp(x, y) = bmp(x, y);
				}
			}
		}
	}
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			if (outBmp(x, y) || x == 0 || y == 0 || x == w - 1 || y == h - 1) {
				bmp(x, y) = outBmp(x, y);
			} else {
				if (outBmp(x - 1, y - 1) == 0 && outBmp(x, y - 1) == 0 &&
				    outBmp(x + 1, y - 1) == 0 && outBmp(x - 1, y + 1) && outBmp(x, y + 1) &&
				    outBmp(x + 1, y + 1)) {
					bmp(x, y) = 255;
				} else if (outBmp(x - 1, y + 1) == 0 && outBmp(x, y + 1) == 0 &&
				           outBmp(x + 1, y + 1) == 0 && outBmp(x - 1, y - 1) && outBmp(x, y - 1) &&
				           outBmp(x + 1, y - 1)) {
					bmp(x, y) = 255;
				} else {
					bmp(x, y) = outBmp(x, y);
				}
			}
		}
	}
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			if (bmp(x, y) || x == 0 || y == 0 || x == w - 1 || y == h - 1) {
				outBmp(x, y) = bmp(x, y);
			} else {
				if (bmp(x - 1, y) == 0 && bmp(x, y - 1) == 0 && bmp(x + 1, y) && bmp(x, y + 1) &&
				    bmp(x + 1, y + 1)) {
					outBmp(x, y) = 255;
				} else if (bmp(x + 1, y) == 0 && bmp(x, y + 1) == 0 && bmp(x - 1, y) &&
				           bmp(x, y - 1) && bmp(x - 1, y - 1)) {
					outBmp(x, y) = 255;
				} else {
					outBmp(x, y) = bmp(x, y);
				}
			}
		}
	}
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			if (outBmp(x, y) || x == 0 || y == 0 || x == w - 1 || y == h - 1) {
				bmp(x, y) = outBmp(x, y);
			} else {
				if (outBmp(x + 1, y) == 0 && outBmp(x, y - 1) == 0 && outBmp(x - 1, y) &&
				    outBmp(x, y + 1) && outBmp(x - 1, y + 1)) {
					bmp(x, y) = 255;
				} else if (outBmp(x - 1, y) == 0 && outBmp(x, y + 1) == 0 && outBmp(x + 1, y) &&
				           outBmp(x, y - 1) && outBmp(x + 1, y - 1)) {
					bmp(x, y) = 255;
				} else {
					bmp(x, y) = outBmp(x, y);
				}
			}
		}
	}
}

static std::vector<std::string> Split(const std::string &str, const std::string &sep) {
	std::vector<std::string> strs;
	size_t pos = 0;
	while (pos < str.size()) {
		size_t newPos = str.find(sep, pos);
		if (newPos == std::string::npos) {
			strs.push_back(str.substr(pos));
			break;
		}
		strs.push_back(str.substr(pos, newPos - pos));
		pos = newPos + sep.size();
	}
	return std::move(strs);
}

int main(int argc, char **argv) {

	if (argc <= 1) {
		std::cout << "BDF (JIS Code) to OpenSpades font converter" << std::endl;
		std::cout << "USAGE: BdfToOSFont OUTFILE [FILTERS...] < BDFFONT.bdf" << std::endl;
		return 0;
	}

	AtlasBuilder<char32_t> builder;
	std::unordered_map<char32_t, Glyph> glyphs;

	std::unique_ptr<CharsetDecoder> decoder(static_cast<CharsetDecoder *>(new JISDecoder));

	bool applySoftFilter = false;
	bool applyThickenFilter = false;
	bool applyDoubleFilter = false;
	bool applyDilationFilter = false;
	for (int i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "soft")) {
			applySoftFilter = true;
		} else if (!strcmp(argv[i], "thicken")) {
			applyThickenFilter = true;
		} else if (!strcmp(argv[i], "double")) {
			applyDoubleFilter = true;
		} else if (!strcmp(argv[i], "dilation")) {
			applyDilationFilter = true;
		}
	}

	char32_t currentChar = 0;
	int dwidth;
	int size = 16;
	int bbxX = 0, bbxY = 0, bbxW = 16, bbxH = 16;
	int scaling = applyDoubleFilter ? 2 : 1;

	std::string lineBuffer;
	while (!std::cin.eof()) {
		std::getline(std::cin, lineBuffer);

		auto firstPartIndex = lineBuffer.find(' ');
		if (firstPartIndex == std::string::npos)
			firstPartIndex = lineBuffer.size();

		auto cmd = lineBuffer.substr(0, firstPartIndex);

		if (cmd == "STARTCHAR") {
			auto hexCode = lineBuffer.substr(firstPartIndex + 1);
			if (hexCode.find("U+") == 0) {
				currentChar = std::stoi(hexCode.substr(2), nullptr, 16);
			} else {
				currentChar = decoder->Decode(hexCode);
			}
			// char buf[32];
			// std::cerr << hexCode << " -> UTF " << currentChar << std::endl;
		} else if (cmd == "DWIDTH") {
			dwidth = std::stoi(lineBuffer.substr(firstPartIndex + 1));
		} else if (cmd == "BBX") {
			auto parts = std::move(Split(lineBuffer.substr(firstPartIndex + 1), " "));
			if (parts.size() < 4) {
				std::cerr << "unexpected EOF while reading BBX" << std::endl;
				return 1;
			}
			bbxX = std::stoi(parts[2]);
			bbxY = std::stoi(parts[3]);
			bbxW = std::stoi(parts[0]);
			bbxH = std::stoi(parts[1]);
		} else if (cmd == "PIXEL_SIZE") {
			size = std::stoi(lineBuffer.substr(firstPartIndex + 1));
			if (applyDoubleFilter)
				size *= 2;
		} else if (cmd == "BITMAP") {
			std::shared_ptr<Bitmap> bmp(new Bitmap);
			Bitmap bmp2;
			while (true) {
				if (std::cin.eof()) {
					std::cerr << "unexpected EOF while reading BITMAP" << std::endl;
					return 1;
				}
				std::getline(std::cin, lineBuffer);

				if (lineBuffer == "ENDCHAR") {
					break;
				}

				int len = lineBuffer.size();
				int y = bmp2.GetHeight();
				bmp2.Resize(std::max(bmp2.GetWidth(), len * 4), y + 1);
				int x = 0;
				for (char c : lineBuffer) {
					int hex = DecodeHexDigit(c);
					if (hex & 8)
						bmp2(x, y) = 255;
					if (hex & 4)
						bmp2(x + 1, y) = 255;
					if (hex & 2)
						bmp2(x + 2, y) = 255;
					if (hex & 1)
						bmp2(x + 3, y) = 255;
					x += 4;
				}
			}

			if (currentChar == 0) {
				// no char code.
				continue;
			}

			if (applyDoubleFilter) {
				ApplyDoubleFilter(bmp2);
				dwidth *= 2;
			}
			if (applyThickenFilter)
				ApplyThickenFilter(bmp2);
			if (applyDilationFilter)
				ApplyDilationFilter(bmp2);
			if (applySoftFilter)
				ApplySoftFilter(bmp2);

			int origHeight = bmp2.GetHeight();

			int srcX, srcY;
			bmp2.Trim(*bmp, srcX, srcY);

			srcX += bbxX * scaling;
			srcY += size - origHeight - bbxY;

			Glyph g;
			g.index = currentChar;
			g.bitmap = bmp;
			g.advance = dwidth;
			g.x = srcX;
			g.y = srcY;

			glyphs[currentChar] = g;

			if (bmp->GetWidth() == 0) {
				// empty
				continue;
			}
			builder.AddItem(currentChar, bmp->GetWidth() + 2, bmp->GetHeight() + 2);
		}
	}

	std::cerr << "packing" << std::endl;

	int binWidth = 1, binHeight = 1;
	while (!builder.TryPack(binWidth, binHeight)) {
		if (binWidth == binHeight) {
			binWidth <<= 1;
		} else {
			binHeight <<= 1;
		}
	}

	std::cerr << "Bin Size = " << binWidth << " x " << binHeight << std::endl;

	{
		Bitmap bin(binWidth, binHeight);

		for (auto &it : glyphs) {
			auto &item = it.second;
			if (item.bitmap->GetWidth() == 0)
				continue; // empty glyph

			const auto &info = builder.GetItem(item.index);
			// std::cerr <<info.x<<","<<info.y<<" for "<<
			// item.bitmap->GetWidth()<<"x"<<item.bitmap->GetHeight()<<std::endl;
			bin.Draw(*item.bitmap, info.x, info.y);
		}

		std::cerr << "rendered" << std::endl;

		std::vector<uint32_t> rgba;
		rgba.resize(binWidth * binHeight);

		for (int x = 0; x < binWidth; x++)
			for (int y = 0; y < binHeight; y++) {
				int alpha = bin(x, y);
				rgba[x + y * binWidth] = (alpha << 24) | 0xffffff;
			}

		std::cerr << "saving" << std::endl;

		tga_image img;
		init_tga_image(&img, reinterpret_cast<uint8_t *>(rgba.data()), binWidth, binHeight, 32);
		img.image_type = TGA_IMAGE_TYPE_BGR_RLE;
		(void)tga_swap_red_blue(&img);
		(void)tga_flip_vert(&img);
		img.image_descriptor ^= TGA_T_TO_B_BIT;

		tga_result result;
		std::string tifPath = argv[1];
		tifPath += ".tga";
		std::ofstream ofs(tifPath);
		result = tga_write_to_FILE(ofs, &img);
	}

	{
		std::cerr << "writing descriptor" << std::endl;
		std::string binPath = argv[1];
		binPath += ".ospfont";
		std::ofstream ofs(binPath);

		ofs.write("OpenSpadesFontFl", 16);

		uint32_t count = static_cast<uint32_t>(glyphs.size());
		static_assert(sizeof(count) == 4, "Oh no");

		// write num of glyphs
		ofs.write(reinterpret_cast<char *>(&count), 4);

		// write font size
		ofs.write(reinterpret_cast<char *>(&size), 4);

		struct GlyphInfo {
			uint32_t unicode;
			uint16_t x, y;
			uint8_t w, h;
			uint16_t advance;
			int16_t offX, offY;
		};

		std::vector<GlyphInfo> infos;

		for (auto &it : glyphs) {
			auto &item = it.second;

			if (item.bitmap->GetWidth() == 0) {
				// empty glyph
				GlyphInfo inf;

				inf.unicode = static_cast<uint32_t>(item.index);
				inf.x = static_cast<uint16_t>(0xffff);
				inf.y = static_cast<uint16_t>(0xffff);
				inf.w = 0;
				inf.h = 0;
				inf.offX = 0;
				inf.offY = 0;
				inf.advance = static_cast<uint16_t>(item.advance);

				infos.push_back(inf);
				continue;
			}

			const auto &info = builder.GetItem(item.index);

			GlyphInfo inf;

			inf.unicode = static_cast<uint32_t>(item.index);
			inf.x = static_cast<uint16_t>(info.x);
			inf.y = static_cast<uint16_t>(info.y);
			inf.w = static_cast<uint8_t>(item.bitmap->GetWidth());
			inf.h = static_cast<uint8_t>(item.bitmap->GetHeight());
			inf.offX = static_cast<int16_t>(item.x);
			inf.offY = static_cast<int16_t>(item.y);
			inf.advance = static_cast<uint16_t>(item.advance);

			infos.push_back(inf);
		}

		ofs.write(reinterpret_cast<char*>(infos.data()), sizeof(GlyphInfo) *
				  infos.size());
	}
	return 0;
}



