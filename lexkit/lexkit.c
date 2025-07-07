#include <lexkit/lexkit.h>
#include <lexkit/break.h>
#include <lexkit/bidi.h>

#include <hb.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <assert.h>

#define GRAPHEME_BREAK_COUNT 16

void lkCreateFont(LkArena* arena, const char* cstr_path, i32 font_size, LkFont *o_font)
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
  o_font->buffer = APushArray(arena, u8, 4096 * 4096);
  o_font->glyphs = APushArray(arena, LkFontAtlasGlyph, 2000);

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

void lkCreateText(LkArena* arena, LkFont* font, const char* cstr, i32 len_cstr, LkText* o_text)
{
  hb_buffer_t *buf;
  buf = hb_buffer_create();
  hb_buffer_add_utf8(buf, cstr, -1, 0, -1);
  hb_buffer_guess_segment_properties(buf);

  o_text->codepoint_count = 0;
  hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &o_text->codepoint_count);

  o_text->codepoints = APushArray(arena, u32, o_text->codepoint_count);
  for (int i = 0; i < o_text->codepoint_count; i++)
    o_text->codepoints[i] = glyph_info[i].codepoint;
}

float MaxF32(float a, float b)
{
  return (a < b) ? b : a;
}

void lkSplitLines(
    LkArena* arena,
    LkUnicodeData* ud,
    LkFont* font,
    LkText* text,
    LkGlyph** glyphs,
    i32 para_count,
    LkParagraph* paras,
    i32 w,
    i32 h,
    i32* o_line_count,
    LkLine* o_lines)
{
  assert(o_line_count != NULL);
  assert(o_lines != NULL);
  assert(*o_line_count == -1);

  LkArena* scratch = arena->alt;
  u64 scratch_pos = scratch->pos;

  // TODO: When realloc lists are added remove this hardcoded line limit

  *o_line_count = 0;

  BreakerResult* breaks = lkGetBreaks(scratch, text, ud);

  float cursor_y = 0.0f;
  int vdc = 0;

  LkParagraph* para_focus = paras;
  for (i32 para_i = 0; para_i < para_count; para_i++)
  {
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
        LkGlyph* codepoint_glyph_p = glyphs[codepoint_i];
        while (codepoint_glyph_p != NULL)
        {
          float bitmap_left = font->glyphs[codepoint_glyph_p->glyph_index].bitmap_left;
          float bitmap_width = font->glyphs[codepoint_glyph_p->glyph_index].bitmap_width;
          word_advance += codepoint_glyph_p->x_advance;
          max_word_x_offset = MaxF32(codepoint_glyph_p->x_offset, max_word_x_offset);
          max_word_width = MaxF32(bitmap_width + bitmap_left, max_word_width);
          glyphs_per_codepoint++;
          codepoint_glyph_p = codepoint_glyph_p->next;
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
            LkGlyph* focus_glyph = glyphs[grapheme_breaks[i]];
            while (focus_glyph != NULL && focus_glyph->next != NULL)
              focus_glyph = focus_glyph->next;
            if (focus_glyph == NULL) continue;
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
        o_lines[(*o_line_count)++] = (LkLine){line_start_i, last_line_break_i, para_focus->para_level, cursor_x_before_last_line_break_i, false};
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
        LkGlyph* focus_glyph = glyphs[codepoint_j];
        while (focus_glyph != NULL && focus_glyph->next != NULL)
        {
          cursor_x += focus_glyph->x_advance;
          if (codepoint_j < last_line_break_i)
            cursor_x_before_last_line_break_i += focus_glyph->x_advance;
          focus_glyph = focus_glyph->next;
        }
        if (focus_glyph == NULL) continue;
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
          o_lines[(*o_line_count)++] = (LkLine){line_start_i, grapheme_i + 1, para_focus->para_level, cursor_x, true};
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
    o_lines[(*o_line_count)++] = (LkLine){line_start_i, line_end_i, para_focus->para_level, cursor_x, false};
    last_line_break_valid = false;
    para_focus = para_focus->next;
  }

  lkArenaRestore(scratch, scratch_pos);
}

void lkShapeText(
    LkArena* arena,
    LkFont* font,
    LkText* text,
    i32 lrun_count,
    LkLevelRun* lruns,
    LkGlyph*** o_glyphs)
{
  assert(o_glyphs != NULL);
  assert(*o_glyphs == NULL);
  *o_glyphs = APushArray(arena, LkGlyph*, text->codepoint_count);
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
    if (lrun.level % 2 == 1)
    {
      i32 gi = 0;
      for (i32 i = lrun.end_i; i >= lrun.start_i; i--)
      {
        LkGlyph** focus = &(*o_glyphs)[i];
        while(gi >= 0 && glyph_info[gi].cluster == i)
        {
          LkGlyph* new_glyph = APush(arena, LkGlyph);
          new_glyph->glyph_index = glyph_info[gi].codepoint;
          new_glyph->x_advance = glyph_pos[gi].x_advance;
          new_glyph->y_advance = glyph_pos[gi].y_advance;
          new_glyph->x_offset = glyph_pos[gi].x_offset;
          new_glyph->y_offset = glyph_pos[gi].y_offset;
          new_glyph->next = NULL;
          *focus = new_glyph;
          focus = &((*focus)->next);
          gi++;
        }
      }
      assert(gi >= glyph_count);
    }
    else {
      i32 gi = 0;
      for (i32 i = lrun.start_i; i <= lrun.end_i; i++)
      {
        LkGlyph** focus = &(*o_glyphs)[i];
        while(gi < glyph_count && glyph_info[gi].cluster == i)
        {
          LkGlyph* new_glyph = APush(arena, LkGlyph);
          new_glyph->glyph_index = glyph_info[gi].codepoint;
          new_glyph->x_advance = glyph_pos[gi].x_advance;
          new_glyph->y_advance = glyph_pos[gi].y_advance;
          new_glyph->x_offset = glyph_pos[gi].x_offset;
          new_glyph->y_offset = glyph_pos[gi].y_offset;
          new_glyph->next = NULL;
          *focus = new_glyph;
          focus = &((*focus)->next);
          gi++;
        }
      }
      assert(gi >= glyph_count);
    }
  }
}

static bool IsL1Class(BIDIC bidic)
{
  return (bidic == BIDIC_FSI &&
          bidic == BIDIC_LRI &&
          bidic == BIDIC_RLI &&
          bidic == BIDIC_PDI);
}

void lkLayoutText(
      LkArena* arena,
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
      i32* o_vd_count)
{
  LkArena* scratch = arena->alt;
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
    BidiUnit* units = APushArray(scratch, BidiUnit, line_codepoint_count);
    for (i32 codepoint_i = lines[line_i].start_i; codepoint_i < lines[line_i].end_i; codepoint_i++)
    {
      level_line[codepoint_i - lines[line_i].start_i] = levels[codepoint_i];
      units[codepoint_i - lines[line_i].start_i] = BidiUnitCreate(text->codepoints[codepoint_i], ud);
    }
    bool reset_fsi_lri_rli_pdi = true;
    for (i32 line_codepoint_i = line_codepoint_count - 1; line_codepoint_i >= 0; line_codepoint_i--)
    {
      if (IsL1Class(units[line_codepoint_i].bidic) && reset_fsi_lri_rli_pdi)
      {
        level_line[line_codepoint_i] = lines[line_i].para_level;
      }
      else if (units[line_codepoint_i].bidic == BIDIC_B &&
                units[line_codepoint_i].bidic == BIDIC_S)
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
    for (i32 codepoint_i = lines[line_i].start_i; codepoint_i < lines[line_i].end_i; codepoint_i++)
    {
      codepoint_orders[codepoint_i - lines[line_i].start_i] = codepoint_i;
    }
    for (i32 level_i = g_bidi_max_depth + 2; level_i > 0; level_i--)
    {
      i32 focus = 0;
      while (focus < line_codepoint_count)
      {
        if (level_line[focus] < level_i)
        {
          focus++;
          continue;
        }
        assert(level_line[focus] == level_i);
        i32 block_start_i = focus;
        i32 block_end_i = focus;
        while (focus < line_codepoint_count && level_line[focus] == level_i)
        {
          block_end_i = focus;
          focus++;
        }
        i32 rfocus = block_start_i;
		
		// Reverse block of contiguous glyphs at the same level
		
        while (block_end_i + block_start_i - rfocus > rfocus)
        {
          // Swap

          i32 a = codepoint_orders[rfocus];
          i32 b = codepoint_orders[block_end_i + block_start_i - rfocus];
          codepoint_orders[rfocus] = b;
          codepoint_orders[block_end_i + block_start_i - rfocus] = a;
          rfocus++;
        }
        for (i32 block_i = block_start_i; block_i <= block_end_i; block_i++)
        {
          level_line[block_i] = level_i - 1;
        }
      }
    }

    // NOTE: Not implementing L3, L4. Are they required or implicitly handled by HarfBuzz??

    for (i32 codepoint_order_i = 0; codepoint_order_i < line_codepoint_count; codepoint_order_i++)
    {
      i32 codepoint_i = codepoint_orders[codepoint_order_i];
      LkGlyph* focus_glyph = glyphs[codepoint_i];
      while (focus_glyph != NULL)
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
        o_vd[vdc++] = v;
        cursor_x += (lines[line_i].hyphen_end &&
                      codepoint_i + 1 >= lines[line_i].end_i &&
                      focus_glyph->next == NULL) ?
                      aglyph.x_advance_hyphen :
                      focus_glyph->x_advance;
        focus_glyph = focus_glyph->next;
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
      o_vd[vdc++] = v;
    }
    cursor_x = 0.0f;
    cursor_y += font->line_gap;
    lkArenaRestore(scratch, scratch_line_pos);
  }
  *o_vd_count = vdc;
}
