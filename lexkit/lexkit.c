#include <lexkit/lexkit.h>
#include <lexkit/break.h>
#include <lexkit/bidi.h>
#include <lexkit/perf.h>

#include <hb.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <assert.h>
#include <string.h>

#define UNICODE_REPLACEMENT_CHARACTER 0xFFFD
#define GRAPHEME_BREAK_COUNT 16

void lkCreateContext(const LkUnicodeData* ud, int num_workers, LkAllocator* alloc, LkContext* o_ctx)
{
  assert(o_ctx != NULL && ud != NULL);
  o_ctx->ud = ud;
  o_ctx->alloc = (alloc) ? alloc : lkAllocatorDefault();
  o_ctx->scratch = lkArenaCreateFrom(o_ctx->alloc);
  o_ctx->queue = lkJobQueueCreate(alloc, num_workers);
}

void lkDestroyContext(LkContext* ctx)
{
  lkJobQueueDestroy(ctx->queue);
  lkArenaDestroy(ctx->scratch);
  ctx->scratch = NULL;
}

void lkCreateFont(LkContext* ctx, const char* cstr_path, i32 font_size, LkFont *o_font)
{
  hb_blob_t *blob = hb_blob_create_from_file(cstr_path);
  hb_face_t *face = hb_face_create(blob, 0);
  hb_font_t *font = hb_font_create(face);
  hb_font_set_scale(font, font_size * 64, font_size * 64);

  FT_Library library;
  FT_Face ftface;
  FT_Init_FreeType(&library);
  FT_New_Face(library, cstr_path, 0, &ftface);
  FT_Set_Char_Size(ftface, font_size * 64, font_size * 64, 0, 0);

  int cursor_x = 2;
  int cursor_y = 2;
  int cursor_y_max = 0;
  o_font->glyph_count = (i32)ftface->num_glyphs;
  o_font->glyphs = AAllocArray(ctx->alloc, LkFontAtlasGlyph, o_font->glyph_count);
  o_font->buffer = AAllocArray(ctx->alloc, u8, 4096llu * 4096llu);
  assert(o_font->glyphs != NULL && o_font->buffer != NULL);

  i32 raster_count = (o_font->glyph_count < 2000) ? o_font->glyph_count : 2000;
  for(int i = 0; i < raster_count; i++)
  {
    o_font->glyphs[i].codepoint = i;
    FT_Load_Glyph(ftface, i, FT_LOAD_DEFAULT);
    FT_Render_Glyph(ftface->glyph, FT_RENDER_MODE_NORMAL);
    if (cursor_x + ftface->glyph->bitmap.width >= 4096)
    {
      cursor_y = cursor_y_max + 2;
      cursor_x = 2;
    }
    o_font->glyphs[i].bitmap_left   = ftface->glyph->bitmap_left;
    o_font->glyphs[i].bitmap_top    = ftface->glyph->bitmap_top;
    o_font->glyphs[i].bitmap_rows   = ftface->glyph->bitmap.rows;
    o_font->glyphs[i].bitmap_width  = ftface->glyph->bitmap.width;

    // TODO: Verify uv bounds against notepad. Without calloc in buffer, noise occurs at letter
    //  boundaries.

    o_font->glyphs[i].u_min = (cursor_x) / 4096.0f;
    o_font->glyphs[i].v_min = (cursor_y) / 4096.0f;
    o_font->glyphs[i].u_max = (cursor_x + ftface->glyph->bitmap.width) / 4096.0f;
    o_font->glyphs[i].v_max = (cursor_y + ftface->glyph->bitmap.rows) / 4096.0f;

    // TODO: Zero-initialize FontAtlasGlyph (or all other fields)

    o_font->glyphs[i].canuse_hyphen = false;

    int cursor_x_local = cursor_x;
    int cursor_y_local = cursor_y;
    int cursor_x_max = cursor_x_local;
    for (int j = 0; j < ftface->glyph->bitmap.rows; j++)
    {
      for (int k = 0; k < ftface->glyph->bitmap.width; k++)
      {
        int pix = j * ftface->glyph->bitmap.pitch + k;
        o_font->buffer[cursor_y_local * 4096 + cursor_x_local] = ftface->glyph->bitmap.buffer[pix];
        cursor_x_local++;
        cursor_x_max = (cursor_x_local > cursor_x_max) ? cursor_x_local : cursor_x_max;
        cursor_y_max = (cursor_y_local > cursor_y_max) ? cursor_y_local : cursor_y_max;
      }
      cursor_x_local = cursor_x;
      cursor_y_local++;
    }
    cursor_x = cursor_x_max;
    cursor_x += 2;
  }

  hb_buffer_t *hyphen_pair_buf = hb_buffer_create();
  for (int i = 0; i < 10000; i++)
  {
    // Hyphen advances

    const char* hyphen_pair_cstr = "-";
    hb_buffer_reset(hyphen_pair_buf);
    hb_codepoint_t codepoint_i = i;
    hb_buffer_add_codepoints(hyphen_pair_buf, &codepoint_i, 1, 0, 1);
    hb_buffer_add_utf8(hyphen_pair_buf, hyphen_pair_cstr, -1, 0, -1);
    hb_buffer_guess_segment_properties(hyphen_pair_buf);
    unsigned int hyphen_pair_glyph_count;
    hb_glyph_info_t* hyphen_pair_glyph_info = hb_buffer_get_glyph_infos(hyphen_pair_buf, &hyphen_pair_glyph_count);
    hb_shape(font, hyphen_pair_buf, NULL, 0);
    hyphen_pair_glyph_info = hb_buffer_get_glyph_infos(hyphen_pair_buf, &hyphen_pair_glyph_count);
    hb_codepoint_t glyph_index_i = hyphen_pair_glyph_info[0].codepoint;
    if (glyph_index_i >= 0 && glyph_index_i < raster_count)
    {
      hb_glyph_position_t* hyphen_pair_glyph_pos = hb_buffer_get_glyph_positions(hyphen_pair_buf, &hyphen_pair_glyph_count);
      o_font->glyphs[glyph_index_i].x_advance_hyphen = hyphen_pair_glyph_pos[0].x_advance;
      o_font->glyphs[glyph_index_i].x_offset_hyphen = hyphen_pair_glyph_pos[1].x_offset;
      o_font->glyphs[glyph_index_i].y_offset_hyphen = hyphen_pair_glyph_pos[1].y_offset;
      o_font->glyphs[glyph_index_i].canuse_hyphen = true;
    }
  }
  hb_buffer_destroy(hyphen_pair_buf);

  const char* hyphen_cstr = "-";
  hb_buffer_t *hyphen_buf;
  hyphen_buf = hb_buffer_create();
  hb_buffer_add_utf8(hyphen_buf, hyphen_cstr, -1, 0, -1);
  hb_buffer_guess_segment_properties(hyphen_buf);
  unsigned int hyphen_glyph_count;
  hb_glyph_info_t* hyphen_glyph_info = hb_buffer_get_glyph_infos(hyphen_buf, &hyphen_glyph_count);
  hb_shape(font, hyphen_buf, NULL, 0);
  hyphen_glyph_info = hb_buffer_get_glyph_infos(hyphen_buf, &hyphen_glyph_count);
  hb_glyph_position_t* hyphen_glyph_pos = hb_buffer_get_glyph_positions(hyphen_buf, &hyphen_glyph_count);
  FT_Load_Char(ftface, '-', FT_LOAD_DEFAULT);
  o_font->hyphen_glyph_i = ftface->glyph->glyph_index;
  o_font->hyphen_advance_x = hyphen_glyph_pos[0].x_advance;
  o_font->hyphen_offset_x = hyphen_glyph_pos[0].x_offset;
  o_font->hyphen_offset_y = hyphen_glyph_pos[0].y_offset;

  // TODO: Make LkFont opaque pointer, integrate stripped down harfbuzz

  o_font->font = font;
  o_font->ascent = ftface->size->metrics.ascender;
  o_font->descent = ftface->size->metrics.descender;
  o_font->line_gap = ftface->size->metrics.height;

  hb_buffer_destroy(hyphen_buf);
  hb_blob_destroy(blob);
  hb_face_destroy(face);
  FT_Done_Face(ftface);
  FT_Done_FreeType(library);
}

void lkDestroyFont(LkContext* ctx, LkFont* font)
{
  AFree(ctx->alloc, font->glyphs, LkFontAtlasGlyph, font->glyph_count);
  font->glyphs = NULL;
  font->glyph_count = 0;
  AFree(ctx->alloc, font->buffer, u8, 4096llu * 4096llu);
  font->buffer = NULL;
  hb_font_destroy((hb_font_t*)(font->font));
  font->font = NULL;
}

// "Flexible and Economical UTF-8 Decoder" by Bjoern Hoehrmann

#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

static const u8 utf8d[] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
  8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
  0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
  0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
  0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
  1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
  1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
  1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
};

static inline u32 Utf8DecodeStep(u32* state, u32* codep, u32 byte)
{
  u32 type = utf8d[byte];

  *codep = (*state != UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

  *state = utf8d[256 + *state*16 + type];
  return *state;
}

static u64 Utf8Decode(const u8* s, u64 n, u32* o_codepoints)
{
  u64 count = 0;
  u64 i = 0;
  u64 start = 0;
  u32 state = UTF8_ACCEPT;
  u32 cp = 0;
  while (i < n)
  {
    if (state == UTF8_ACCEPT)
    {
      // Fast path for ASCII characters

      while (i + 8 <= n)
      {
        u64 chunk;
        memcpy(&chunk, s + i, 8);
        if (chunk & 0x8080808080808080llu)
          break;
        if (o_codepoints)
        {
          for (i32 k = 0; k < 8; k++)
            o_codepoints[count + k] = s[i + k];
        }
        count += 8;
        i += 8;
      }
      if (i >= n)
        break;
      start = i;
    }

    // Lookup table that supports longer codepoints

    Utf8DecodeStep(&state, &cp, s[i]);
    i++;
    if (state != UTF8_ACCEPT)
    {
      // Replace invalid codepoints with the replacement character

      if (state != UTF8_REJECT && i < n)
        continue;
      cp = UNICODE_REPLACEMENT_CHARACTER;
      i = start + 1;
      state = UTF8_ACCEPT;
    }
    if (o_codepoints)
      o_codepoints[count] = cp;
    count++;
  }
  return count;
}

void lkCreateText(LkContext* ctx, LkFont* font, const char* cstr, i32 len_cstr, LkText* o_text)
{
#if MEASURE_PERF
  int64_t ts = timestamp();
#endif
  const u8* bytes = (const u8*)cstr;
  u64 len_bytes;
  if (len_cstr < 0)
  {
    len_bytes = strlen(cstr);
  }
  else
  {
    // Check if buffer is less than len_cstr

    const u8* nul = (const u8*)memchr(bytes, 0, (size_t)len_cstr);
    len_bytes = nul ? (u64)(nul - bytes) : (u64)len_cstr;
  }

  o_text->arena = lkArenaCreateFrom(ctx->alloc);
  o_text->codepoint_count = (u32)Utf8Decode(bytes, len_bytes, NULL);
  o_text->codepoints = APushArray(o_text->arena, u32, o_text->codepoint_count);
  Utf8Decode(bytes, len_bytes, o_text->codepoints);

#if MEASURE_PERF
  printf("Decode: %g ms\n", (timestamp() - ts) * 1000.0 / timestamp_res());
  ts = timestamp();
#endif
  o_text->units = NULL;
  lkComputeBidiUnits(ctx, o_text->arena, o_text->codepoints, o_text->codepoint_count, &o_text->units);
#if MEASURE_PERF
  printf("lkComputeBidiUnits: %g ms\n", (timestamp() - ts) * 1000.0 / timestamp_res());
  ts = timestamp();
#endif
  o_text->para_count = -1;
  o_text->paragraphs = NULL;
  lkSplitParagraphs(ctx, o_text->arena, o_text->codepoints, o_text->codepoint_count, o_text->units, &o_text->para_count, &o_text->paragraphs);
#if MEASURE_PERF
  printf("lkSplitParagraphs: %g ms\n", (timestamp() - ts) * 1000.0 / timestamp_res());
  ts = timestamp();
#endif
  o_text->levels = NULL;
  o_text->level_run_count = -1;
  o_text->level_runs = NULL;
  lkSplitBidiRuns(ctx, o_text->arena, o_text->codepoints, o_text->codepoint_count, o_text->units, o_text->para_count, o_text->paragraphs, &o_text->levels, &o_text->level_run_count, &o_text->level_runs);
#if MEASURE_PERF
  printf("lkSplitBidiRuns: %g ms\n", (timestamp() - ts) * 1000.0 / timestamp_res());
  ts = timestamp();
#endif
  o_text->glyphs = NULL;
  o_text->glyph_start = NULL;
  o_text->glyph_count = 0;
  lkShapeText(o_text->arena, font, o_text, o_text->level_run_count, o_text->level_runs, &o_text->glyphs, &o_text->glyph_start, &o_text->glyph_count);
#if MEASURE_PERF
  printf("lkShapeText: %g ms\n", (timestamp() - ts) * 1000.0 / timestamp_res());
  ts = timestamp();
#endif
  o_text->breaks = lkGetBreaks(ctx, o_text->arena, o_text);
#if MEASURE_PERF
  printf("lkGetBreaks: %g ms\n", (timestamp() - ts) * 1000.0 / timestamp_res());
#endif
}

void lkDestroyText(LkContext* ctx, LkText* text)
{
  lkArenaDestroy(text->arena);
  text->arena = NULL;
  text->codepoints = NULL;
  text->units = NULL;
  text->paragraphs = NULL;
  text->levels = NULL;
  text->level_runs = NULL;
  text->glyphs = NULL;
  text->glyph_start = NULL;
  text->glyph_count = 0;
  text->breaks = NULL;
  text->codepoint_count = 0;
  text->para_count = 0;
  text->level_run_count = 0;
}

float MaxF32(float a, float b)
{
  return (a < b) ? b : a;
}

static void PushLine(LkArena* arena, LkLine** io_lines, i32* io_line_count, LkLine line)
{
  LkLine* new_line = APush(arena, LkLine);
  if (*io_lines == NULL)
    *io_lines = new_line;
  assert(new_line == *io_lines + *io_line_count);
  *new_line = line;
  (*io_line_count)++;
}

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
    i32* o_line_count)
{
  assert(o_line_count != NULL);
  assert(*o_line_count == -1);

  LkLine* o_lines = NULL;

  *o_line_count = 0;

  const BreakerResult* breaks = text->breaks;

  float cursor_y = 0.0f;
  int vdc = 0;

  for (i32 para_i = 0; para_i < para_count; para_i++)
  {
    const LkParagraph* para_focus = &paras[para_i];
    float cursor_x = 0.0f;
    float cursor_x_before_last_line_break_i = 0.0f;
    int codepoint_i = para_focus->para_start_i;
    i32 line_start_i = codepoint_i;
    i32 line_end_i = codepoint_i;
    bool last_line_break_valid = false;
    i32 last_line_break_i = codepoint_i;
    bool can_line_break = false;
    bool can_line_break_before_word = false;
    bool must_line_break_before_word = false;

    while (codepoint_i <= para_focus->para_end_i)
    {
      float word_advance = 0.0f;
      float max_word_width = 0.0f;
      float max_word_x_offset = 0.0f;
      i32 word_start_i = codepoint_i;
      i32 word_end_i = codepoint_i;
      bool can_line_break_before_next_word = false;
      bool must_line_break_before_next_word = false;
      float grapheme_advances[GRAPHEME_BREAK_COUNT];
      i32 grapheme_breaks[GRAPHEME_BREAK_COUNT];
      int grapheme_break_count = 0;
      while (codepoint_i <= para_focus->para_end_i)
      {
        BreakerResult res = breaks[codepoint_i];

        i32 glyphs_per_codepoint = 0;
        const LkGlyph* codepoint_glyph_p = glyphs + glyph_start[codepoint_i];
        const LkGlyph* codepoint_glyph_end = glyphs + glyph_start[codepoint_i + 1];
        while (codepoint_glyph_p < codepoint_glyph_end && !codepoint_glyph_p->ignore)
        {
          float bitmap_left = font->glyphs[codepoint_glyph_p->glyph_index].bitmap_left;
          float bitmap_width = font->glyphs[codepoint_glyph_p->glyph_index].bitmap_width;
          word_advance += codepoint_glyph_p->x_advance;
          max_word_x_offset = MaxF32(codepoint_glyph_p->x_offset, max_word_x_offset);
          max_word_width = MaxF32(bitmap_width + bitmap_left, max_word_width);
          glyphs_per_codepoint++;
          codepoint_glyph_p++;
        }
        
        if (res.gbrk == GBRK_BRK &&
            grapheme_break_count < GRAPHEME_BREAK_COUNT &&
            glyphs_per_codepoint > 0)
        {
          grapheme_breaks[grapheme_break_count] = codepoint_i;
          grapheme_advances[grapheme_break_count] = word_advance;
          grapheme_break_count++;
        }

        codepoint_i++;
        word_end_i = codepoint_i;

        if (res.wbrk == WBRK_BRK)
        {
          if (res.lbrk != LBRK_PRO)
            can_line_break_before_next_word = true;
          if (res.lbrk == LBRK_MAN)
            must_line_break_before_next_word = true;
          break;
        }
      }

      bool exceeds_line = (cursor_x + word_advance + max_word_x_offset) / 64.0 + max_word_width >= w;

      // Try grapheme break

      bool found_grapheme_break = false;
      i32 grapheme_i = -1;

      if (exceeds_line)
      {
        for (int i = 0; i < grapheme_break_count; i++)
        {
          if (grapheme_breaks[i] + 1 < word_end_i &&
              (cursor_x + grapheme_advances[i] + font->hyphen_advance_x) / 64.0 < w)
          {
            if (glyph_start[grapheme_breaks[i]] == glyph_start[grapheme_breaks[i] + 1])
              continue;
            const LkGlyph* focus_glyph = glyphs + glyph_start[grapheme_breaks[i] + 1] - 1;
            LkFontAtlasGlyph aglyph = font->glyphs[focus_glyph->glyph_index];
            if (!aglyph.canuse_hyphen) continue;
            found_grapheme_break = true;
            grapheme_i = grapheme_breaks[i];
          }
        }
      }

      // Try word break

      if (must_line_break_before_word || (!found_grapheme_break &&
          exceeds_line && last_line_break_valid))
      {
        PushLine(arena, &o_lines, o_line_count, (LkLine){line_start_i, last_line_break_i, para_focus->para_level, cursor_x_before_last_line_break_i, false});
        last_line_break_valid = false;
        line_start_i = last_line_break_i;
        cursor_x = MaxF32(0.0f, cursor_x - cursor_x_before_last_line_break_i);
        cursor_x_before_last_line_break_i = cursor_x; 
        cursor_y += font->line_gap;
        can_line_break = false;
      }
      else
        can_line_break = true;
        
      line_end_i = word_end_i;
      can_line_break_before_word = can_line_break_before_next_word;
      must_line_break_before_word = must_line_break_before_next_word;

      if (can_line_break_before_next_word && can_line_break)
      {
        last_line_break_valid = true;
        last_line_break_i = line_end_i;
        cursor_x_before_last_line_break_i = cursor_x;
      }
      else if (can_line_break && found_grapheme_break)
      {
        last_line_break_valid = true;
        last_line_break_i = grapheme_i + 1;
        cursor_x_before_last_line_break_i = cursor_x;
      }

      for (i32 codepoint_j = word_start_i; codepoint_j < word_end_i; codepoint_j++)
      {
        const LkGlyph* focus_glyph = glyphs + glyph_start[codepoint_j];
        const LkGlyph* focus_glyph_end = glyphs + glyph_start[codepoint_j + 1];
        while (focus_glyph < focus_glyph_end && focus_glyph + 1 < focus_glyph_end && !focus_glyph->ignore)
        {
          cursor_x += focus_glyph->x_advance;
          if (codepoint_j < last_line_break_i)
            cursor_x_before_last_line_break_i += focus_glyph->x_advance;
          focus_glyph++;
        }
        if (focus_glyph >= focus_glyph_end) continue;
        if (focus_glyph->ignore) continue;
        LkFontAtlasGlyph aglyph = font->glyphs[focus_glyph->glyph_index];
        float cursor_x_advance = (found_grapheme_break &&
                                  grapheme_i == codepoint_j) ?
                                  aglyph.x_advance_hyphen :
                                  focus_glyph->x_advance;
        cursor_x += cursor_x_advance;
        if (codepoint_j < last_line_break_i)
          cursor_x_before_last_line_break_i += cursor_x_advance;
        if (found_grapheme_break && grapheme_i == codepoint_j)
        {
          PushLine(arena, &o_lines, o_line_count, (LkLine){line_start_i, grapheme_i + 1, para_focus->para_level, cursor_x, true});
          line_start_i = grapheme_i + 1;
          line_end_i = word_end_i;
          last_line_break_valid = false;
          cursor_x = 0.0f;
          cursor_x_before_last_line_break_i = 0.0f;
          cursor_y += font->line_gap;
          can_line_break = false;
        }
      }
    }
    PushLine(arena, &o_lines, o_line_count, (LkLine){line_start_i, line_end_i, para_focus->para_level, cursor_x, false});
    last_line_break_valid = false;
  }

  if (o_lines == NULL)
    o_lines = APushArray(arena, LkLine, 0);
  return o_lines;
}

bool IgnoreCodepointDuringShaping(u32 codepoint)
{
  return (codepoint == 10); // LF
}

void lkShapeText(
    LkArena* arena,
    LkFont* font,
    LkText* text,
    i32 lrun_count,
    LkLevelRun* lruns,
    LkGlyph** o_glyphs,
    u32** o_glyph_start,
    u32* o_glyph_count)
{
  assert(o_glyphs != NULL && o_glyph_start != NULL && o_glyph_count != NULL);
  assert(*o_glyphs == NULL && *o_glyph_start == NULL);

  u32* glyph_start = APushArray(arena, u32, (u64)text->codepoint_count + 1);
  LkGlyph* glyphs = NULL;
  u32 glyph_total = 0;
  i32 next_codepoint_i = 0;

  for (i32 lrun_i = 0; lrun_i < lrun_count; lrun_i++)
  {
    LkLevelRun lrun = lruns[lrun_i];

    hb_buffer_t *buf;
    buf = hb_buffer_create();
    hb_buffer_add_codepoints(
        buf,
        text->codepoints,
        text->codepoint_count,
        lrun.start_i,
        lrun.end_i - lrun.start_i + 1);
    hb_buffer_guess_segment_properties(buf);
    if (lrun.level % 2 == 1)
    {
      hb_buffer_set_direction(buf, HB_DIRECTION_RTL);
    }
    else
    {
      hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    }
    hb_shape(font->font, buf, NULL, 0);
    u32 glyph_count = 0;
    hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    LkGlyph* run_glyphs = APushArray(arena, LkGlyph, glyph_count);
    if (glyphs == NULL)
      glyphs = run_glyphs;
    assert(run_glyphs == glyphs + glyph_total);

    assert(lrun.start_i >= next_codepoint_i);
    for (i32 i = next_codepoint_i; i <= lrun.start_i; i++)
      glyph_start[i] = glyph_total;
    for (i32 i = lrun.start_i; i <= lrun.end_i; i++)
      glyph_start[i + 1] = 0;
    for (u32 gi = 0; gi < glyph_count; gi++)
      glyph_start[glyph_info[gi].cluster + 1]++;
    for (i32 i = lrun.start_i; i <= lrun.end_i; i++)
      glyph_start[i + 1] += glyph_start[i];

    u64 scratch_pos = lkArenaGetPos(arena->alt);
    u32* cluster_fill = APushArray(arena->alt, u32, (u64)(lrun.end_i - lrun.start_i + 1));
    for (i32 i = lrun.start_i; i <= lrun.end_i; i++)
      cluster_fill[i - lrun.start_i] = glyph_start[i];
    for (u32 gi = 0; gi < glyph_count; gi++)
    {
      u32 cluster = glyph_info[gi].cluster;
      LkGlyph* new_glyph = &glyphs[cluster_fill[cluster - lrun.start_i]++];
      new_glyph->glyph_index = glyph_info[gi].codepoint;
      new_glyph->x_advance = glyph_pos[gi].x_advance;
      new_glyph->y_advance = glyph_pos[gi].y_advance;
      new_glyph->x_offset = glyph_pos[gi].x_offset;
      new_glyph->y_offset = glyph_pos[gi].y_offset;
      new_glyph->ignore = IgnoreCodepointDuringShaping(text->codepoints[cluster]);
    }
    lkArenaRestore(arena->alt, scratch_pos);
    glyph_total += glyph_count;
    next_codepoint_i = lrun.end_i + 1;
    hb_buffer_destroy(buf);
  }

  for (u64 i = (u64)next_codepoint_i; i <= text->codepoint_count; i++)
    glyph_start[i] = glyph_total;

  *o_glyphs = glyphs;
  *o_glyph_start = glyph_start;
  *o_glyph_count = glyph_total;
}

static bool IsL1Class(BIDIC bidic)
{
  return (bidic == BIDIC_FSI &&
          bidic == BIDIC_LRI &&
          bidic == BIDIC_RLI &&
          bidic == BIDIC_PDI);
}

typedef struct
{
  bool is_leaf;
  i32 level;
  i32 start;
  i32 sz;
  i32 first_child;
  i32 last_child;
  i32 next_sibling;
  i32 prev_sibling;
} L2ReversalNode;

i32 BuildL2ReversalTree(
      i32* level_line,
      i32 line_codepoint_count,
      i32 level,
      i32 level_start,
      i32* o_first_child,
      i32* o_node_count,
      L2ReversalNode* o_tree)
{
  int node_sz = 0;
  int focus = level_start;
  i32 prev_sibling = -1;
  while (focus < line_codepoint_count && level_line[focus] >= level)
  {
    if (level_line[focus] == level)
    {
      int node_start = focus;
      while (focus < line_codepoint_count && level_line[focus] == level)
      {
        focus++;
      }
      if (*o_first_child == -1)
      {
        *o_first_child = *o_node_count;
      }
      if (prev_sibling != -1)
      {
        o_tree[prev_sibling].next_sibling = *o_node_count;
      }
      o_tree[(*o_node_count)] = (L2ReversalNode){ true, level, node_start, focus - node_start, -1, -1, -1, prev_sibling };
      prev_sibling = *o_node_count;
      (*o_node_count)++;
      node_sz += focus - node_start;
    }
    else
    {
      i32 first_child = -1;
      i32 new_focus = BuildL2ReversalTree(level_line, line_codepoint_count, level + 1, focus, &first_child, o_node_count, o_tree);
      assert(first_child < *o_node_count);
      i32 last_child = (*o_node_count) - 1;
      if (*o_first_child == -1)
      {
        *o_first_child = *o_node_count;
      }
      if (prev_sibling != -1)
      {
        o_tree[prev_sibling].next_sibling = *o_node_count;
      }
      o_tree[(*o_node_count)] = (L2ReversalNode){ false, level + 1, focus, new_focus - focus, first_child, last_child, -1, prev_sibling };
      prev_sibling = *o_node_count;
      (*o_node_count)++;
      node_sz += new_focus - focus;
      focus = new_focus;
    }
  }
  return focus;
}

void PerformL2Reversals(
  LkLine line,
  L2ReversalNode* tree,
  i32 focus,
  i32 focus_start,
  i32* o_codepoint_orders)
{
  L2ReversalNode fnode = tree[focus];
  i32 pos = focus_start;
  bool is_reversed = (tree[focus].level & 1) ? true : false;
  i32 child_iter = is_reversed ? fnode.last_child : fnode.first_child;
  while (child_iter != -1)
  {
    i32 child_start = tree[child_iter].start;
    i32 child_sz = tree[child_iter].sz;
    if (tree[child_iter].is_leaf)
    {
      if (is_reversed)
      {
        for (int i = 0; i < child_sz; i++)
          o_codepoint_orders[pos + i] = line.start_i + child_start + child_sz - 1 - i;
      }
      else
      {
        for (int i = 0; i < child_sz; i++)
          o_codepoint_orders[pos + i] = line.start_i + child_start + i;
      }
      pos += child_sz;
    }
    else
    {
      PerformL2Reversals(line, tree, child_iter, pos, o_codepoint_orders);
      pos += child_sz;
    }
    child_iter = is_reversed ? tree[child_iter].prev_sibling : tree[child_iter].next_sibling;
  }
}

void lkLayoutText(
      LkContext* ctx,
      LkFont* font,
      LkText* text,
      i32 w,
      i32 h,
      u64 max_vd,
      LkVertexDescriptor_Text* o_vd,
      i32* o_vd_count)
{
  u64 pos = lkArenaGetPos(ctx->scratch);
  i32 line_count = -1;
#if MEASURE_PERF
  static bool first_layout = false;
  int64_t ts_start = timestamp();
#endif
  LkLine* lines = lkSplitLines(ctx, ctx->scratch, font, text, text->glyphs, text->glyph_start, text->para_count, text->paragraphs, w, h, &line_count);
#if MEASURE_PERF
  int64_t ts_split = timestamp();
#endif
  i32* levels = text->levels;
  const LkGlyph* glyphs = text->glyphs;
  const u32* glyph_start = text->glyph_start;
  const BidiUnit* units = text->units;
  LkArena* scratch = ctx->scratch->alt;
  i32 vdc = 0;
  float cursor_x = 0.0;
  float cursor_y = 0.0;
  LkFontAtlasGlyph aglyph = {0};
  for (i32 line_i = 0; line_i < line_count; line_i++)
  {
    u64 scratch_line_pos = scratch->pos;
    i32 line_codepoint_count = lines[line_i].end_i - lines[line_i].start_i;

    // L1

    i32* level_line = APushArray(scratch, i32, line_codepoint_count);
    for (i32 codepoint_i = lines[line_i].start_i; codepoint_i < lines[line_i].end_i; codepoint_i++)
    {
      level_line[codepoint_i - lines[line_i].start_i] = levels[codepoint_i];
    }
    bool reset_fsi_lri_rli_pdi = true;
    for (i32 line_codepoint_i = line_codepoint_count - 1; line_codepoint_i >= 0; line_codepoint_i--)
    {
      if (IsL1Class(units[lines[line_i].start_i + line_codepoint_i].bidic) && reset_fsi_lri_rli_pdi)
      {
        level_line[line_codepoint_i] = lines[line_i].para_level;
      }
      else if (units[lines[line_i].start_i + line_codepoint_i].bidic == BIDIC_B &&
                units[lines[line_i].start_i + line_codepoint_i].bidic == BIDIC_S)
      {
        level_line[line_codepoint_i] = lines[line_i].para_level;
        reset_fsi_lri_rli_pdi = true;
      }
      else
      {
        reset_fsi_lri_rli_pdi = false;
      }
    }

    // L2

    i32* codepoint_orders = APushArray(scratch, i32, line_codepoint_count);
    L2ReversalNode* l2_nodes = APushArray(scratch, L2ReversalNode, line_codepoint_count);
    i32 l2_node_count = 1;
    l2_nodes[0] = (L2ReversalNode) { false, 0, 0, line_codepoint_count, -1, -1, -1, -1 };
    BuildL2ReversalTree(level_line, line_codepoint_count, 0, 0, &l2_nodes[0].first_child, &l2_node_count, l2_nodes);
    l2_nodes[0].last_child = l2_node_count - 1;
    PerformL2Reversals(lines[line_i], l2_nodes, 0, 0, codepoint_orders);

    // NOTE: Not implementing L3, L4. Are they required or implicitly handled by HarfBuzz??

    for (i32 codepoint_order_i = 0; codepoint_order_i < line_codepoint_count; codepoint_order_i++)
    {
      i32 codepoint_i = codepoint_orders[codepoint_order_i];
      const LkGlyph* focus_glyph = glyphs + glyph_start[codepoint_i];
      const LkGlyph* focus_glyph_end = glyphs + glyph_start[codepoint_i + 1];
      while (focus_glyph < focus_glyph_end && !focus_glyph->ignore)
      {
        aglyph = font->glyphs[focus_glyph->glyph_index];
        LkVertexDescriptor_Text v = {
          ((cursor_x + focus_glyph->x_offset) / 64.0f + aglyph.bitmap_left) / w,
          ((cursor_y + focus_glyph->y_offset + font->ascent) / 64.0f - aglyph.bitmap_top) / h,
          ((float) aglyph.bitmap_width) / w,
          ((float) aglyph.bitmap_rows) / h,
          aglyph.u_min,
          aglyph.v_min,
          aglyph.u_max,
          aglyph.v_max
        };

        // TODO: Rethink this. Need to stop outputting vds after we've exceeded height.
        //  Probably at the lkSplitLines level.

        if (vdc + 1 >= max_vd)
          goto end;
        o_vd[vdc++] = v;
        cursor_x += (lines[line_i].hyphen_end &&
                      codepoint_i + 1 >= lines[line_i].end_i &&
                      focus_glyph + 1 == focus_glyph_end) ?
                      aglyph.x_advance_hyphen :
                      focus_glyph->x_advance;
        focus_glyph++;
      }
    }
    if (lines[line_i].hyphen_end)
    {
      LkFontAtlasGlyph aglyph_hyphen = font->glyphs[font->hyphen_glyph_i];
      LkVertexDescriptor_Text v = {
        ((cursor_x + aglyph.x_offset_hyphen) / 64.0f + aglyph_hyphen.bitmap_left) / w,
        ((cursor_y + aglyph.y_offset_hyphen + font->ascent) / 64.0f - aglyph_hyphen.bitmap_top) / h,
        ((float) aglyph_hyphen.bitmap_width) / w,
        ((float) aglyph_hyphen.bitmap_rows) / h,
        aglyph_hyphen.u_min,
        aglyph_hyphen.v_min,
        aglyph_hyphen.u_max,
        aglyph_hyphen.v_max
      };
      if (vdc + 1 >= max_vd)
        goto end;
      o_vd[vdc++] = v;
    }
end:
    cursor_x = 0.0f;
    cursor_y += font->line_gap;
    lkArenaRestore(scratch, scratch_line_pos);
  }
  *o_vd_count = vdc;
#if MEASURE_PERF
  if (!first_layout)
  {
    printf("lkSplitLines: %g ms\nlkLayoutText: %g ms\n",
            (ts_split - ts_start) * 1000.0 / timestamp_res(),
            (timestamp() - ts_split) * 1000.0 / timestamp_res());
    first_layout = true;
  }
#endif
  lkArenaRestore(ctx->scratch, pos);
}
