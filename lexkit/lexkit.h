#ifndef __LEXKIT__
#define __LEXKIT__

#include <lexkit/types.h>
#include <lexkit/break.h>
#include <lexkit/bidi.h>

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

struct LkText
{
  u32 codepoint_count;
  u32* codepoints;
};

typedef struct LkText LkText;

struct LkGlyph
{
  u32 glyph_index;
  i32 x_advance;
  i32 y_advance;
  i32 x_offset;
  i32 y_offset;

  struct LkGlyph* next;
};

typedef struct LkGlyph LkGlyph;

typedef struct
{
  i32 start_i;
  i32 end_i;
  i32 para_level;
  float cursor_x;
  bool hyphen_end;
} LkLine;

void lkCreateFont(const char* cstr_path, i32 font_size, LkFont* o_font);
void lkCreateText(LkFont* font, const char* cstr, i32 len_cstr, LkText* o_text);
void lkShapeText(
    LkFont* font,
    LkText* text,
    i32 lrun_count,
    LevelRun* lruns,
    LkGlyph*** o_glyphs);
void lkSplitLines(
    LkUnicodeData* ud,
    LkFont* font,
    LkText* text,
    LkGlyph** glyphs,
    i32 para_count,
    Paragraph* paras,
    i32 w,
    i32 h,
    i32* o_line_count,
    LkLine* o_lines);
void lkLayoutText(
      LkUnicodeData* ud,
      LkFont* font,
      LkText* text,
      i32* levels,
      LkGlyph** glyphs,
      i32 line_count,
      LkLine* lines,
      i32 w,
      i32 h,
      u64 max_vd,
      LkVertexDescriptor_Text* o_vd,
      i32* o_vd_count);
#endif
