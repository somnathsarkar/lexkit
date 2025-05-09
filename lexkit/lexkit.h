#ifndef __LEXKIT__
#define __LEXKIT__

#include <lexkit/types.h>
#include <lexkit/break.h>

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
  bool canuse_hyphen;
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

typedef struct
{
  u8 *buffer;
  LkFontAtlasGlyph *glyphs;
  i32 hyphen_glyph_i;
  i32 hyphen_advance_x;
  i32 hyphen_offset_x;
  i32 hyphen_offset_y;
  void* font; // hb_font_t*
  float ascent;
  float descent;
  float line_gap;
} LkFont;

typedef struct
{
  u32 codepoint_count;
  u32 glyph_count;
  u32* codepoints;
  void* glyph_info; // hb_glyph_info_t*
  void* glyph_pos; // hb_glyph_position_t*
} LkText;

void lkCreateFont(const char* cstr_path, i32 font_size, LkFont* o_font);
void lkCreateText(LkFont* font, const char* cstr, i32 len_cstr, LkText* o_text);
void lkLayoutText(
      LkUnicodeData* ud,
      LkFont* font,
      LkText* text,
      i32 w,
      i32 h,
      u64 max_vd,
      LkVertexDescriptor_Text* o_vd,
      i32* o_vd_count);
#endif
