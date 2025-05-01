#ifndef __LEXKIT__
#define __LEXKIT__

#include <lexkit/types.h>

typedef struct
{
  int codepoint;
  int bitmap_left;
  int bitmap_top;
  int bitmap_rows;
  int bitmap_width;
  float u_min;
  float v_min;
  float u_max;
  float v_max;
  int x_advance_hyphen;
  int x_offset_hyphen;
  int y_offset_hyphen;
} LkFontAtlasGlyph;

typedef struct {
  float x;
  float y;
  float w;
  float h;
  float u_min;
  float v_min;
  float u_max;
  float v_max;
} LkVertexDescriptor_Text;

#endif
