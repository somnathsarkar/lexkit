#ifndef __LEXKIT__
#define __LEXKIT__

#include <lexkit/alloc.h>
#include <lexkit/types.h>
#include <lexkit/break.h>
#include <lexkit/bidi.h>
#include <lexkit/sizes.h>

struct LkContext
{
  LkUnicodeData* ud;
};

void lkCreateContext(LkUnicodeData* ud, LkContext* o_ctx);

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
  bool ignore;        // Whether to skip past this glyph, ie LF. All are true/false for any codepoint. 

  struct LkGlyph* next;
};

typedef struct LkGlyph LkGlyph;

struct LkLine
{
  i32 start_i;
  i32 end_i;
  i32 para_level;
  float cursor_x;
  bool hyphen_end;
};

typedef struct LkLine LkLine;

void lkCreateFont(LkArena* arena, const char* cstr_path, i32 font_size, LkFont* o_font);
void lkCreateText(LkArena* arena, LkFont* font, const char* cstr, i32 len_cstr, LkText* o_text);
void lkShapeText(
    LkArena* arena,
    LkFont* font,
    LkText* text,
    i32 lrun_count,
    LkLevelRun* lruns,
    LkGlyph*** o_glyphs);
LkLine* lkSplitLines(
    LkContext* ctx,
    LkArena* arena,
    LkFont* font,
    LkText* text,
    LkGlyph** glyphs,
    i32 para_count,
    LkParagraph* paras,
    i32 w,
    i32 h,
    i32* o_line_count);
void lkLayoutText(
      LkContext* ctx,
      LkArena* arena,
      LkFont* font,
      LkText* text,
      i32* levels,
      LkGlyph** glyphs,
      i32 line_count,
      LkLine* lines,
      const BidiUnit* units,
      i32 w,
      i32 h,
      u64 max_vd,
      LkVertexDescriptor_Text* o_vd,
      i32* o_vd_count);
#endif
