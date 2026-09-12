#include <examples/example_opengl.h>

#include <generated/shaders/fullscreen.vert.h>
#include <generated/shaders/fullscreen.frag.h>
#include <generated/shaders/text.vert.h>
#include <generated/shaders/text.frag.h>

#include <lexkit/lexkit.h>
#include <lexkit/bidi.h>
#include <lexkit/perf.h>
#include <lexkit/unicode_data_16.h>

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

LkVertexDescriptor_Text vd[10240] = {0};

int main() {
  char * cstr = NULL;
  i32 len_cstr = 0;
  read_file("example_bidi.txt", &cstr, &len_cstr);
  int font_size = 72;

  LkArena* arena = lkArenaCreateFixed(Megabytes(64));

  LkContext ctx;
  lkCreateContext(&g_lk_unicode_data, NULL, &ctx);

  LkFont font;
  lkCreateFont(&ctx, "C:/Windows/Fonts/Arial.ttf", font_size, &font);
#if MEASURE_PERF
  int64_t ts_setup = timestamp();
#endif
  LkText text;
  lkCreateText(&ctx, &font, cstr, len_cstr, &text);
  i32 level_run_count = -1;
  LkLevelRun* level_runs = NULL;
  i32 para_count = -1;
  LkParagraph* paragraphs = NULL;
  LkGlyph** glyphs = NULL;
  i32* levels = NULL;
  BidiUnit* units = NULL;
  lkComputeBidiUnits(&ctx, arena, text.codepoints, text.codepoint_count, &units);
  lkSplitParagraphs(&ctx, arena, text.codepoints, text.codepoint_count, units, &para_count, &paragraphs);
  lkSplitBidiRuns(&ctx, arena, text.codepoints, text.codepoint_count, units, para_count, paragraphs, &levels, &level_run_count, &level_runs);
  lkShapeText(arena, &font, &text, level_run_count, level_runs, &glyphs);
#if MEASURE_PERF
  printf("Setup Time: %g ms\n", ((timestamp() - ts_setup) * 1000.0)/timestamp_res());
#endif

  HWND hwnd = create_window();

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RED,
      4096,
      4096,
      0,
      GL_RED,
      GL_UNSIGNED_BYTE,
      font.buffer);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  GLuint fb;
  glGenFramebuffers(1, &fb);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, fb);
  glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, GL_NONE);

  GLuint prog_fullscreen = create_prog(
      fullscreen_vert, fullscreen_vert_len,
      fullscreen_frag, fullscreen_frag_len);
  GLuint prog_text = create_prog(
      text_vert, text_vert_len,
      text_frag, text_frag_len);

  GLuint buffer_vertex_fullscreen;
  GLuint vao_fullscreen;
  float vertex_fullscreen[] = {
    1.0f, 1.0f,
    1.0f, -1.0f,
    -1.0f, 1.0f,
    -1.0f, -1.0f
  };

  glGenVertexArrays(1, &vao_fullscreen);
  glBindVertexArray(vao_fullscreen);
  glGenBuffers(1, &buffer_vertex_fullscreen);
  glBindBuffer(GL_ARRAY_BUFFER, buffer_vertex_fullscreen);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_fullscreen), vertex_fullscreen, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  GLuint buffer_vertex_text;
  GLuint vao_text;

  glGenVertexArrays(1, &vao_text);
  glBindVertexArray(vao_text);
  glGenBuffers(1, &buffer_vertex_text);
  glBindBuffer(GL_ARRAY_BUFFER, buffer_vertex_text);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(LkVertexDescriptor_Text), (void*)0);
  glVertexAttribDivisor(0, 1);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LkVertexDescriptor_Text), (void*)16);
  glVertexAttribDivisor(1, 1);
  glEnableVertexAttribArray(1);

  int64_t ts = timestamp_win64();
  int64_t ts_acc = 0;

  int quit = 0;
  while (!quit)
  {
    MSG msg;
    while(PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
      if(msg.message == WM_QUIT)
      {
        quit = 1;
        break;
      }
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }
    int64_t ts_new = timestamp_win64();
    int64_t dts = ts_new - ts;
    ts = ts_new;
    ts_acc += dts;
    if (ts_acc >= timestamp_win64_res() / 60)
    {
      RECT client_rect = {0};
      GetClientRect(hwnd, &client_rect);
      int w = client_rect.right - client_rect.left;
      int h = client_rect.bottom - client_rect.top;
      glViewport(0, 0, w, h);
      glClearColor((float) 0x21 / 0xFF, (float) 0x21 / 0xFF, (float) 0x21 / 0xFF, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      u64 frame_pos = lkArenaGetPos(arena);
      i32 vdc = 0;
      i32 line_count = -1;
      LkLine* lines = lkSplitLines(&ctx, arena, &font, &text, glyphs, para_count, paragraphs, w, h, &line_count);
      lkLayoutText(&ctx, arena, &font, &text, levels, glyphs, line_count, lines, units, w, h, 10240, vd, &vdc);
      lkArenaRestore(arena, frame_pos);

      glBindBuffer(GL_ARRAY_BUFFER, buffer_vertex_text);
      glBufferData(
          GL_ARRAY_BUFFER,
          sizeof(vd[0]) * vdc,
          vd,
          GL_STATIC_DRAW);

      glUseProgram(prog_text); 
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
      glBindVertexArray(vao_text);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, tex);
      glUniform1i(0, 0);
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, vdc);

      HDC hdc = GetDC(hwnd);
      SwapBuffers(hdc);
      ReleaseDC(hwnd, hdc);
    }
  }

  lkDestroyFont(&ctx, &font);

  return 0;
}

