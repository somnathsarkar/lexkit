#ifndef __LEXKIT__
#define __LEXKIT__

#include <lexkit/alloc.h>
#include <lexkit/types.h>
#include <lexkit/break.h>
#include <lexkit/bidi.h>
#include <lexkit/sizes.h>

typedef struct LkJobQueue LkJobQueue;

struct LkContext
{
  const LkUnicodeData* ud;
  LkAllocator* alloc;
  LkArena* scratch;
  LkJobQueue* queue;
  LkArena** worker_scratch;
};

void lkCreateContext(const LkUnicodeData* ud, int num_workers, LkAllocator* alloc, LkContext* o_ctx);
void lkDestroyContext(LkContext* ctx);

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
  i32 glyph_count;
  i32 hyphen_glyph_i;
  i32 hyphen_advance_x;
  i32 hyphen_offset_x;
  i32 hyphen_offset_y;
  void* font; // hb_font_t*
  float ascent;
  float descent;
  float line_gap;
} LkFont;

struct LkGlyph
{
  u32 glyph_index;
  i32 x_advance;
  i32 y_advance;
  i32 x_offset;
  i32 y_offset;
  bool ignore;        // Whether to skip past this glyph, ie LF. All are true/false for any codepoint. 
};

typedef struct LkGlyph LkGlyph;

struct LkText
{
  u32 codepoint_count;
  u32* codepoints;
  BidiUnit* units;
  i32 para_count;
  LkParagraph* paragraphs;
  i32* levels;
  i32 level_run_count;
  LkLevelRun* level_runs;
  LkGlyph* glyphs;
  u32* glyph_start;
  u32 glyph_count;
  BreakerResult* breaks;
  LkArena* arena;
};

typedef struct LkText LkText;

struct LkLine
{
  i32 start_i;
  i32 end_i;
  i32 para_level;
  float cursor_x;
  bool hyphen_end;
};

typedef struct LkLine LkLine;

void lkCreateFont(LkContext* ctx, const char* cstr_path, i32 font_size, LkFont* o_font);
void lkDestroyFont(LkContext* ctx, LkFont* font);
void lkCreateText(LkContext* ctx, LkFont* font, const char* cstr, i32 len_cstr, LkText* o_text);
void lkDestroyText(LkContext* ctx, LkText* text);
void lkLayoutText(
    LkContext* ctx,
    LkFont* font,
    LkText* text,
    i32 w,
    i32 h,
    i32 scroll_y,
    u64 max_vd,
    LkVertexDescriptor_Text* o_vd,
    i32* o_vd_count);
void lkShapeText(
    LkContext* ctx,
    LkArena* arena,
    LkFont* font,
    LkText* text,
    i32 lrun_count,
    LkLevelRun* lruns,
    LkGlyph** o_glyphs,
    u32** o_glyph_start,
    u32* o_glyph_count);
LkLine* lkSplitLines(
    LkContext* ctx,
    LkArena* arena,
    LkFont* font,
    LkText* text,
    const LkGlyph* glyphs,
    const u32* glyph_start,
    i32 para_count,
    LkParagraph* paras,
    i32 w,
    i32 h,
    i32* o_line_count);
#endif
