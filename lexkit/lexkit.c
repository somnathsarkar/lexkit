#include <lexkit/lexkit.h>
#include <lexkit/break.h>

#include <hb.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <assert.h>

#define GRAPHEME_BREAK_COUNT 16

void lkCreateFont(const char* cstr_path, i32 font_size, LkFont *o_font)
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
  o_font->buffer = calloc(4096 * 4096, sizeof(u8));
  o_font->glyphs = calloc(2000, sizeof(LkFontAtlasGlyph));

  for(int i = 0; i < 2000; i++)
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

  for (int i = 0; i < 10000; i++)
  {
    // Hyphen advances

    const char* hyphen_pair_cstr = "-";
    hb_buffer_t *hyphen_buf;
    hb_buffer_t *hyphen_pair_buf;
    hyphen_pair_buf = hb_buffer_create();
    hb_codepoint_t codepoint_i = i;
    hb_buffer_add_codepoints(hyphen_pair_buf, &codepoint_i, 1, 0, 1);
    hb_buffer_add_utf8(hyphen_pair_buf, hyphen_pair_cstr, -1, 0, -1);
    hb_buffer_guess_segment_properties(hyphen_pair_buf);
    unsigned int hyphen_pair_glyph_count;
    hb_glyph_info_t* hyphen_pair_glyph_info = hb_buffer_get_glyph_infos(hyphen_pair_buf, &hyphen_pair_glyph_count);
    hb_shape(font, hyphen_pair_buf, NULL, 0);
    hyphen_pair_glyph_info = hb_buffer_get_glyph_infos(hyphen_pair_buf, &hyphen_pair_glyph_count);
    hb_codepoint_t glyph_index_i = hyphen_pair_glyph_info[0].codepoint;
    if (glyph_index_i >= 0 && glyph_index_i < 2000)
    {
      hb_glyph_position_t* hyphen_pair_glyph_pos = hb_buffer_get_glyph_positions(hyphen_pair_buf, &hyphen_pair_glyph_count);
      o_font->glyphs[glyph_index_i].x_advance_hyphen = hyphen_pair_glyph_pos[0].x_advance;
      o_font->glyphs[glyph_index_i].x_offset_hyphen = hyphen_pair_glyph_pos[1].x_offset;
      o_font->glyphs[glyph_index_i].y_offset_hyphen = hyphen_pair_glyph_pos[1].y_offset;
      o_font->glyphs[glyph_index_i].canuse_hyphen = true;
    }
  }

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
}

void lkCreateText(LkFont* font, const char* cstr, i32 len_cstr, LkText* o_text)
{
  hb_buffer_t *buf;
  buf = hb_buffer_create();
  hb_buffer_add_utf8(buf, cstr, -1, 0, -1);
  hb_buffer_guess_segment_properties(buf);

  o_text->codepoint_count = 0;
  o_text->glyph_info = hb_buffer_get_glyph_infos(buf, &o_text->codepoint_count);

  o_text->codepoints = (u32*) malloc(sizeof(u32) * o_text->codepoint_count);
  for (int i = 0; i < o_text->codepoint_count; i++)
    o_text->codepoints[i] = ((hb_glyph_info_t*)(o_text->glyph_info))[i].codepoint;

  hb_shape(font->font, buf, NULL, 0);
  o_text->glyph_info = hb_buffer_get_glyph_infos(buf, &o_text->glyph_count);
  
  // TODO: Number of codepoints does not need to match glyphs. Remove this assert when break behavior is changed
  //	so that glyph index does not need to match codepoint index.
  
  assert(o_text->codepoint_count == o_text->glyph_count);
  o_text->glyph_pos = hb_buffer_get_glyph_positions(buf, &o_text->glyph_count);
}

void lkLayoutText(
      LkUnicodeData* ud,
      LkFont* font,
      LkText* text,
      i32 w,
      i32 h,
      u64 max_vd,
      LkVertexDescriptor_Text* o_vd,
      i32* o_vd_count)
{
  Breaker brk = {0};
  BreakerCreate(text->codepoints, text->codepoint_count, &brk);

  float cursor_x = 0.0f;
  float cursor_y = 0.0f;
  int next_break = -1;
  bool parse_failure = false;
  int gi = 0;
  bool can_line_break = false;
  bool can_line_break_before_word = false;
  int vdc = 0;
  
  // BB: gi uses glyph indices, but brk uses codepoint indices. The hb_glyph_info_t struct has a cluster_id
  //	member after shaping that maps shaped glyph to the codepoint. That member should be checked here
  //	and the assert that compares glyph_count to codepoint_count should be removed.
  
  while (gi < text->glyph_count)
  {
    float word_advance = 0.0f;
    int word_start_i = gi;
    int word_end_i = gi;
    bool can_line_break_before_next_word = false;
    float grapheme_advances[GRAPHEME_BREAK_COUNT];
    int grapheme_breaks[GRAPHEME_BREAK_COUNT];
    int grapheme_break_count = 0;
    while (gi < text->glyph_count)
    {
      BreakerResult res = BreakerAdvance(&brk, ud);
      word_advance += ((hb_glyph_position_t*)(text->glyph_pos))[gi].x_advance;

      if (res.gbrk == GBRK_BRK && grapheme_break_count < GRAPHEME_BREAK_COUNT)
      {
        grapheme_breaks[grapheme_break_count] = gi;
        grapheme_advances[grapheme_break_count] = word_advance;
        grapheme_break_count++;
      }

      gi++;
      word_end_i = gi;
      if (res.wbrk == WBRK_BRK)
      {
        if (res.lbrk != LBRK_PRO)
          can_line_break_before_next_word = true;
        break;
      }
    }

    bool exceeds_line = (cursor_x + word_advance) / 64.0 >= w;

    // Try grapheme break

    int grapheme_break_i = -1;

    if (exceeds_line)
    {
      for (int i = 0; i < grapheme_break_count; i++)
      {
        if ((cursor_x + grapheme_advances[i] + font->hyphen_advance_x) / 64.0 < w)
        {
          const hb_glyph_info_t* focus_glyph_info = &((hb_glyph_info_t*)(text->glyph_info))[grapheme_breaks[i]];
          LkFontAtlasGlyph aglyph = font->glyphs[focus_glyph_info->codepoint];
          if (!aglyph.canuse_hyphen) continue;
          grapheme_break_i = grapheme_breaks[i];
        }
      }
    }

    // Try word break

    if (grapheme_break_i == -1 && can_line_break && exceeds_line && can_line_break_before_word)
    {
      cursor_x = 0.0f;
      cursor_y += font->line_gap;
      can_line_break = false;
    }
    else
      can_line_break = true;

    can_line_break_before_word = can_line_break_before_next_word;

    for (int i = word_start_i; i < word_end_i; i++)
    {
      const hb_glyph_info_t* focus_glyph_info = &((hb_glyph_info_t*)(text->glyph_info))[i];
      const hb_glyph_position_t* focus_glyph_pos = &((hb_glyph_position_t*)(text->glyph_pos))[i];
      LkFontAtlasGlyph aglyph = font->glyphs[focus_glyph_info->codepoint];
      LkVertexDescriptor_Text v = {
        ((cursor_x + focus_glyph_pos->x_offset) / 64.0f + aglyph.bitmap_left) / w,
        ((cursor_y + focus_glyph_pos->y_offset + font->ascent) / 64.0f - aglyph.bitmap_top) / h,
        ((float) aglyph.bitmap_width) / w,
        ((float) aglyph.bitmap_rows) / h,
        aglyph.u_min,
        aglyph.v_min,
        aglyph.u_max,
        aglyph.v_max
      };
      o_vd[vdc++] = v;
      cursor_x += (grapheme_break_i == i) ?
                    aglyph.x_advance_hyphen :
                    focus_glyph_pos->x_advance;
      if (grapheme_break_i == i)
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
        o_vd[vdc++] = v;
        cursor_x = 0.0f;
        cursor_y += font->line_gap;
        can_line_break = false;
      }
    }
  }
  *o_vd_count = vdc;
}
