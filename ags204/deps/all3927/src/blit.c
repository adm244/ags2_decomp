/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Blitting functions.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/aintern.h"



/* blit_from_256:
 *  Expand 256 color images onto a truecolor destination.
 */
static void blit_from_256(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   #ifdef ALLEGRO_COLOR8

   int *dest_palette_color;
   unsigned long s, d;
   unsigned char *ss;
   int x, y, c;

   /* lookup table avoids repeated color format conversions */
   dest_palette_color = _palette_expansion_table(bitmap_color_depth(dest));

   /* worker macro */
   #define EXPAND_BLIT(bits, dsize)                                          \
   {                                                                         \
      if (is_memory_bitmap(src)) {                                           \
	 /* fast version when reading from memory bitmap */                  \
	 bmp_select(dest);                                                   \
									     \
	 for (y=0; y<h; y++) {                                               \
	    ss = src->line[s_y+y] + s_x;                                     \
	    d = bmp_write_line(dest, d_y+y) + d_x*dsize;                     \
									     \
	    for (x=0; x<w; x++) {                                            \
	       bmp_write##bits(d, dest_palette_color[*ss]);                  \
	       ss++;                                                         \
	       d += dsize;                                                   \
	    }                                                                \
	 }                                                                   \
									     \
	 bmp_unwrite_line(dest);                                             \
      }                                                                      \
      else {                                                                 \
	 /* slower version when reading from the screen */                   \
	 for (y=0; y<h; y++) {                                               \
	    s = bmp_read_line(src, s_y+y) + s_x;                             \
	    d = bmp_write_line(dest, d_y+y) + d_x*dsize;                     \
									     \
	    for (x=0; x<w; x++) {                                            \
	       bmp_select(src);                                              \
	       c = bmp_read8(s);                                             \
									     \
	       bmp_select(dest);                                             \
	       bmp_write##bits(d, dest_palette_color[c]);                    \
									     \
	       s++;                                                          \
	       d += dsize;                                                   \
	    }                                                                \
	 }                                                                   \
									     \
	 bmp_unwrite_line(src);                                              \
	 bmp_unwrite_line(dest);                                             \
      }                                                                      \
   }

   /* expand the above macro for each possible output depth */
   switch (bitmap_color_depth(dest)) {

   #ifdef ALLEGRO_COLOR16
      case 15:
      case 16:
	 EXPAND_BLIT(16, sizeof(short));
	 break;
   #endif

   #ifdef ALLEGRO_COLOR24
      case 24:
	 EXPAND_BLIT(24, 3);
	 break;
   #endif

   #ifdef ALLEGRO_COLOR32
      case 32:
	 EXPAND_BLIT(32, sizeof(long));
	 break;
   #endif

   }

   #endif
}



/* worker macro for converting between two color formats */
#define CONVERT_BLIT(sbits, ssize, dbits, dsize)                             \
{                                                                            \
   for (y=0; y<h; y++) {                                                     \
      s = bmp_read_line(src, s_y+y) + s_x*ssize;                             \
      d = bmp_write_line(dest, d_y+y) + d_x*dsize;                           \
									     \
      for (x=0; x<w; x++) {                                                  \
	 bmp_select(src);                                                    \
	 c = bmp_read##sbits(s);                                             \
									     \
	 r = getr##sbits(c);                                                 \
	 g = getg##sbits(c);                                                 \
	 b = getb##sbits(c);                                                 \
									     \
	 bmp_select(dest);                                                   \
	 bmp_write##dbits(d, makecol##dbits(r, g, b));                       \
									     \
	 s += ssize;                                                         \
	 d += dsize;                                                         \
      }                                                                      \
   }                                                                         \
									     \
   bmp_unwrite_line(src);                                                    \
   bmp_unwrite_line(dest);                                                   \
}



/* worker macro for converting formats plus option dithering */
#define CONVERT_DITHER_BLIT(sbits, ssize, dbits, dsize)                      \
{                                                                            \
   for (y=0; y<h; y++) {                                                     \
      s = bmp_read_line(src, s_y+y) + s_x*ssize;                             \
      d = bmp_write_line(dest, d_y+y) + d_x*dsize;                           \
									     \
      if (_color_conv & COLORCONV_DITHER) {                                  \
	 for (x=0; x<w; x++) {                                               \
	    bmp_select(src);                                                 \
	    c = bmp_read##sbits(s);                                          \
									     \
	    r = getr##sbits(c);                                              \
	    g = getg##sbits(c);                                              \
	    b = getb##sbits(c);                                              \
									     \
	    bmp_select(dest);                                                \
	    bmp_write##dbits(d, makecol##dbits##_dither(r, g, b, x, y));     \
									     \
	    s += ssize;                                                      \
	    d += dsize;                                                      \
	 }                                                                   \
      }                                                                      \
      else {                                                                 \
	 for (x=0; x<w; x++) {                                               \
	    bmp_select(src);                                                 \
	    c = bmp_read##sbits(s);                                          \
									     \
	    r = getr##sbits(c);                                              \
	    g = getg##sbits(c);                                              \
	    b = getb##sbits(c);                                              \
									     \
	    bmp_select(dest);                                                \
	    bmp_write##dbits(d, makecol##dbits(r, g, b));                    \
									     \
	    s += ssize;                                                      \
	    d += dsize;                                                      \
	 }                                                                   \
      }                                                                      \
   }                                                                         \
									     \
   bmp_unwrite_line(src);                                                    \
   bmp_unwrite_line(dest);                                                   \
}



/* blit_from_15:
 *  Convert 15 bpp images onto some other destination format.
 */
static void blit_from_15(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   #ifdef ALLEGRO_COLOR16

   int x, y, c, r, g, b;
   unsigned long s, d;

   switch (bitmap_color_depth(dest)) {

      #ifdef ALLEGRO_COLOR8
	 case 8:
	    CONVERT_BLIT(15, sizeof(short), 8, 1);
	    break;
      #endif

      #ifdef ALLEGRO_COLOR16
	 case 16:
	    CONVERT_BLIT(15, sizeof(short), 16, sizeof(short));
	    break;
      #endif

      #ifdef ALLEGRO_COLOR24
	 case 24:
	    CONVERT_BLIT(15, sizeof(short), 24, 3);
	    break;
      #endif

      #ifdef ALLEGRO_COLOR32
	 case 32:
	    CONVERT_BLIT(15, sizeof(short), 32, sizeof(long));
	    break;
      #endif

   }

   #endif
}



/* blit_from_16:
 *  Convert 16 bpp images onto some other destination format.
 */
static void blit_from_16(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   #ifdef ALLEGRO_COLOR16

   int x, y, c, r, g, b;
   unsigned long s, d;

   switch (bitmap_color_depth(dest)) {

      #ifdef ALLEGRO_COLOR8
	 case 8:
	    CONVERT_BLIT(16, sizeof(short), 8, 1);
	    break;
      #endif

      #ifdef ALLEGRO_COLOR16
	 case 15:
	    CONVERT_BLIT(16, sizeof(short), 15, sizeof(short));
	    break;
      #endif

      #ifdef ALLEGRO_COLOR24
	 case 24:
	    CONVERT_BLIT(16, sizeof(short), 24, 3);
	    break;
      #endif

      #ifdef ALLEGRO_COLOR32
	 case 32:
	    CONVERT_BLIT(16, sizeof(short), 32, sizeof(long));
	    break;
      #endif

   }

   #endif 
}



/* blit_from_24:
 *  Convert 24 bpp images onto some other destination format.
 */
static void blit_from_24(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   #ifdef ALLEGRO_COLOR24

   int x, y, c, r, g, b;
   unsigned long s, d;

   switch (bitmap_color_depth(dest)) {

      #ifdef ALLEGRO_COLOR8
	 case 8:
	    CONVERT_BLIT(24, 3, 8, 1);
	    break;
      #endif

      #ifdef ALLEGRO_COLOR16
	 case 15:
	    CONVERT_DITHER_BLIT(24, 3, 15, sizeof(short));
	    break;

	 case 16:
	    CONVERT_DITHER_BLIT(24, 3, 16, sizeof(short));
	    break;
      #endif

      #ifdef ALLEGRO_COLOR32
	 case 32:
	    CONVERT_BLIT(24, 3, 32, sizeof(long));
	    break;
      #endif

   }

   #endif 
}



/* blit_from_32:
 *  Convert 32 bpp images onto some other destination format.
 */
static void blit_from_32(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   #ifdef ALLEGRO_COLOR32

   int x, y, c, r, g, b;
   unsigned long s, d;

   switch (bitmap_color_depth(dest)) {

      #ifdef ALLEGRO_COLOR8
	 case 8:
	    CONVERT_BLIT(32, sizeof(long), 8, 1);
	    break;
      #endif

      #ifdef ALLEGRO_COLOR16
	 case 15:
	    CONVERT_DITHER_BLIT(32, sizeof(long), 15, sizeof(short));
	    break;

	 case 16:
	    CONVERT_DITHER_BLIT(32, sizeof(long), 16, sizeof(short));
	    break;
      #endif

      #ifdef ALLEGRO_COLOR24
	 case 24:
	    CONVERT_BLIT(32, sizeof(long), 24, 3);
	    break;
      #endif

   }

   #endif 
}



/* blit_to_or_from_modex:
 *  Converts between truecolor and planar mode-X bitmaps. This function is 
 *  painfully slow, but I don't think it is something that people will need
 *  to do very often...
 */
static void blit_to_or_from_modex(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   #ifdef GFX_MODEX

   int x, y, c, r, g, b;
   int src_depth = bitmap_color_depth(src);
   int dest_depth = bitmap_color_depth(dest);

   for (y=0; y<h; y++) {
      for (x=0; x<w; x++) {
	 c = getpixel(src, s_x+x, s_y+y);
	 r = getr_depth(src_depth, c);
	 g = getg_depth(src_depth, c);
	 b = getb_depth(src_depth, c);
	 c = makecol_depth(dest_depth, r, g, b);
	 putpixel(dest, d_x+x, d_y+y, c);
      }
   }

   #endif
}



/* blit_between_formats:
 *  Blits an (already clipped) region between two bitmaps of different
 *  color depths, doing the appopriate format conversions.
 */
static void blit_between_formats(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   if ((is_planar_bitmap(src)) || (is_planar_bitmap(dest))) {
      blit_to_or_from_modex(src, dest, s_x, s_y, d_x, d_y, w, h);
   }
   else {
      switch (bitmap_color_depth(src)) {

	 case 8:
	    blit_from_256(src, dest, s_x, s_y, d_x, d_y, w, h);
	    break;

	 case 15:
	    blit_from_15(src, dest, s_x, s_y, d_x, d_y, w, h);
	    break;

	 case 16:
	    blit_from_16(src, dest, s_x, s_y, d_x, d_y, w, h);
	    break;

	 case 24:
	    blit_from_24(src, dest, s_x, s_y, d_x, d_y, w, h);
	    break;

	 case 32:
	    blit_from_32(src, dest, s_x, s_y, d_x, d_y, w, h);
	    break;
      }
   }
}



/* blit_to_self:
 *  Blits an (already clipped) region between two areas of the same bitmap,
 *  checking which way around to do the blit.
 */
static void blit_to_self(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   unsigned long sx, sy, dx, dy;
   BITMAP *tmp;

   if (dest->id & BMP_ID_NOBLIT) {
      /* with single-banked cards we have to use a temporary bitmap */
      tmp = create_bitmap(w, h);
      if (tmp) {
	 src->vtable->blit_to_memory(src, tmp, s_x, s_y, 0, 0, w, h);
	 dest->vtable->blit_from_memory(tmp, dest, 0, 0, d_x, d_y, w, h);
	 destroy_bitmap(tmp);
      }
   }
   else {
      /* check which way round to do the blit */
      sx = s_x + src->x_ofs;
      sy = s_y + src->y_ofs;

      dx = d_x + dest->x_ofs;
      dy = d_y + dest->y_ofs;

      if ((sx+w <= dx) || (dx+w <= sx) || (sy+h <= dy) || (dy+h <= sy))
	 dest->vtable->blit_to_self(src, dest, s_x, s_y, d_x, d_y, w, h);
      else if ((sy > dy) || ((sy == dy) && (sx > dx)))
	 dest->vtable->blit_to_self_forward(src, dest, s_x, s_y, d_x, d_y, w, h);
      else if ((sx != dx) || (sy != dy))
	 dest->vtable->blit_to_self_backward(src, dest, s_x, s_y, d_x, d_y, w, h);
   } 
}



/* helper for clipping a blit rectangle */
#define BLIT_CLIP()                                                          \
   /* check for ridiculous cases */                                          \
   if ((s_x >= src->w) || (s_y >= src->h) ||                                 \
       (d_x >= dest->cr) || (d_y >= dest->cb))                               \
      return;                                                                \
									     \
   /* clip src left */                                                       \
   if (s_x < 0) {                                                            \
      w += s_x;                                                              \
      d_x -= s_x;                                                            \
      s_x = 0;                                                               \
   }                                                                         \
									     \
   /* clip src top */                                                        \
   if (s_y < 0) {                                                            \
      h += s_y;                                                              \
      d_y -= s_y;                                                            \
      s_y = 0;                                                               \
   }                                                                         \
									     \
   /* clip src right */                                                      \
   if (s_x+w > src->w)                                                       \
      w = src->w - s_x;                                                      \
									     \
   /* clip src bottom */                                                     \
   if (s_y+h > src->h)                                                       \
      h = src->h - s_y;                                                      \
									     \
   /* clip dest left */                                                      \
   if (d_x < dest->cl) {                                                     \
      d_x -= dest->cl;                                                       \
      w += d_x;                                                              \
      s_x -= d_x;                                                            \
      d_x = dest->cl;                                                        \
   }                                                                         \
									     \
   /* clip dest top */                                                       \
   if (d_y < dest->ct) {                                                     \
      d_y -= dest->ct;                                                       \
      h += d_y;                                                              \
      s_y -= d_y;                                                            \
      d_y = dest->ct;                                                        \
   }                                                                         \
									     \
   /* clip dest right */                                                     \
   if (d_x+w > dest->cr)                                                     \
      w = dest->cr - d_x;                                                    \
									     \
   /* clip dest bottom */                                                    \
   if (d_y+h > dest->cb)                                                     \
      h = dest->cb - d_y;                                                    \
									     \
   /* bottle out if zero size */                                             \
   if ((w <= 0) || (h <= 0))                                                 \
      return;



/* void blit(BITMAP *src, BITMAP *dest, int s_x, s_y, int d_x, d_y, int w, h);
 *  Copies an area of the source bitmap to the destination bitmap. s_x and 
 *  s_y give the top left corner of the area of the source bitmap to copy, 
 *  and d_x and d_y give the position in the destination bitmap. w and h 
 *  give the size of the area to blit. This routine respects the clipping 
 *  rectangle of the destination bitmap, and will work correctly even when 
 *  the two memory areas overlap (ie. src and dest are the same). 
 */
void blit(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   BLIT_CLIP();

   if (src->vtable->color_depth != dest->vtable->color_depth) {
      /* need to do a color conversion */
      blit_between_formats(src, dest, s_x, s_y, d_x, d_y, w, h);
   }
   else if (is_same_bitmap(src, dest)) {
      /* special handling for overlapping regions */
      blit_to_self(src, dest, s_x, s_y, d_x, d_y, w, h);
   }
   else if (is_video_bitmap(dest)) {
      /* drawing onto video bitmaps */
      if (is_video_bitmap(src))
	 dest->vtable->blit_to_self(src, dest, s_x, s_y, d_x, d_y, w, h);
      else if (is_system_bitmap(src))
	 dest->vtable->blit_from_system(src, dest, s_x, s_y, d_x, d_y, w, h);
      else
	 dest->vtable->blit_from_memory(src, dest, s_x, s_y, d_x, d_y, w, h);
   }
   else if (is_system_bitmap(dest)) {
      /* drawing onto system bitmaps */
      if (is_video_bitmap(src))
	 src->vtable->blit_to_system(src, dest, s_x, s_y, d_x, d_y, w, h);
      else if (is_system_bitmap(src))
	 dest->vtable->blit_to_self(src, dest, s_x, s_y, d_x, d_y, w, h);
      else
	 dest->vtable->blit_from_memory(src, dest, s_x, s_y, d_x, d_y, w, h);
   }
   else {
      /* drawing onto memory bitmaps */
      if ((is_video_bitmap(src)) || (is_system_bitmap(src)))
	 src->vtable->blit_to_memory(src, dest, s_x, s_y, d_x, d_y, w, h);
      else
	 dest->vtable->blit_to_self(src, dest, s_x, s_y, d_x, d_y, w, h);
   }
}

END_OF_FUNCTION(blit);



/* void masked_blit(BITMAP *src, BITMAP *dest, int s_x, s_y, d_x, d_y, w, h);
 *  Version of blit() that skips zero pixels. The source must be a memory
 *  bitmap, and the source and dest regions must not overlap.
 */
void masked_blit(BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   ASSERT(src->vtable->color_depth == dest->vtable->color_depth);

   BLIT_CLIP();

   dest->vtable->masked_blit(src, dest, s_x, s_y, d_x, d_y, w, h);
}

