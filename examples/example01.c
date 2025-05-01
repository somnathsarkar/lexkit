#include <windows.h>

#include <extern/glad/gl.h>
#include <extern/glad/wgl.h>
#include <hb.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <generated/shaders/fullscreen.vert.h>
#include <generated/shaders/fullscreen.frag.h>
#include <generated/shaders/text.vert.h>
#include <generated/shaders/text.frag.h>

#include <lexkit/lexkit.h>
#include <lexkit/break.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define GRAPHEME_BREAK_COUNT 16

LRESULT wndproc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
  switch (umsg)
  {
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProcA(hwnd, umsg, wparam, lparam);
}

int64_t timestamp() {
  LARGE_INTEGER res = {0};
  QueryPerformanceCounter(&res);
  return res.QuadPart;
}

void* glad_load_func(const char* name)
{
  void *p = (void*)wglGetProcAddress(name);
  if  (p == 0 ||
      (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) ||
      (p == (void*)-1))
  {
    HMODULE module = LoadLibraryA("opengl32.dll");
    p = (void *)GetProcAddress(module, name);
  }

  return p;
}

unsigned char buffer[2048 * 2048] = { 0 };

LkVertexDescriptor_Text vd[10240] = {0};

GLuint create_prog(
    const unsigned char* vs, const unsigned int vs_len,
    const unsigned char* fs, const unsigned int fs_len)
{
  GLuint prog = glCreateProgram();

  GLuint vert = glCreateShader(GL_VERTEX_SHADER);
  glShaderBinary(1, &vert, GL_SHADER_BINARY_FORMAT_SPIR_V, vs, vs_len);
  glSpecializeShader(vert, "main", 0, 0, 0);
  glAttachShader(prog, vert);

  GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderBinary(1, &frag, GL_SHADER_BINARY_FORMAT_SPIR_V, fs, fs_len);
  glSpecializeShader(frag, "main", 0, 0, 0);
  glAttachShader(prog, frag);

  glLinkProgram(prog);
  return prog;
}

int main() {
  LARGE_INTEGER qpf = {0};
  QueryPerformanceFrequency(&qpf);
  int64_t timestamp_res = qpf.QuadPart;

  const char* cstr = \
    "Call Me Ishmael. Some years ago—never mind how long precisely—having "
    "little or no money in my purse, and nothing particular to interest me on shore, "
    "I thought I would sail about a little and see the watery part of the world. "
    "It is a way I have of driving off the spleen, and regulating the circulation. "
    "Whenever I find myself growing grim about the mouth; whenever it is a damp, "
    "drizzly November in my soul; whenever I find myself involuntarily pausing before "
    "coffin warehouses, and bringing up the rear of every funeral I meet; and especially "
    "whenever my hypos get such an upper hand of me, that it requires a strong moral "
    "principle to prevent me from deliberately stepping into the street, and "
    "methodically knocking people’s hats off—then, I account it high time to get "
    "to sea as soon as I can.";
  int len_cstr = strnlen(cstr, 10000);
  int font_size = 72;

  LkUnicodeData ud = {0};
  bool ud_success = UnicodeDataTryLoadFromSpec(
      "C:/Code/lexkit/lexkit/LineBreakProperty.txt",
      "C:/Code/lexkit/lexkit/WordBreakProperty.txt",
      "C:/Code/lexkit/lexkit/GraphemeBreakProperty.txt",
      "C:/Code/lexkit/lexkit/UnicodeData.txt",
      "C:/Code/lexkit/lexkit/EastAsianWidth.txt",
      "C:/Code/lexkit/lexkit/DerivedCoreProperties.txt",
      "C:/Code/lexkit/lexkit/emoji-data.txt",
      &ud);

  FT_Library library;
  FT_Face ftface;
  FT_Init_FreeType(&library);
  FT_New_Face(library, "C:/Dev/Fonts/Hack/Hack Regular Nerd Font Complete.ttf", 0, &ftface);
  FT_Set_Char_Size(ftface, font_size * 64, font_size * 64, 0, 0);

  hb_buffer_t *buf;
  buf = hb_buffer_create();
  hb_buffer_add_utf8(buf, cstr, -1, 0, -1);
  hb_buffer_guess_segment_properties(buf);
  hb_blob_t *blob = hb_blob_create_from_file("C:/Dev/Fonts/Hack/Hack Regular Nerd Font Complete.ttf");
  hb_face_t *face = hb_face_create(blob, 0);
  hb_font_t *font = hb_font_create(face);
  hb_font_set_scale(font, font_size * 64, font_size * 64);

  unsigned int glyph_count;
  hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);

  u32* codepoints = (u32*) malloc(sizeof(u32) * glyph_count);
  for (int i = 0; i < glyph_count; i++)
    codepoints[i] = glyph_info[i].codepoint;

  hb_shape(font, buf, NULL, 0);
  glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
  hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

  // TODO: Atlas
  int cursor_x = 2;
  int cursor_y = 2;
  int cursor_y_max = 0;
  LkFontAtlasGlyph glyphs[1000] = {0};

  int hyphen_glyph_i;
  int hyphen_advance_x;
  int hyphen_offset_x;
  int hyphen_offset_y;
  
  const char* hyphen_cstr = "‐";
  hb_buffer_t *hyphen_buf;
  hyphen_buf = hb_buffer_create();
  hb_buffer_add_utf8(hyphen_buf, hyphen_cstr, -1, 0, -1);
  hb_buffer_guess_segment_properties(hyphen_buf);
  unsigned int hyphen_glyph_count;
  hb_glyph_info_t* hyphen_glyph_info = hb_buffer_get_glyph_infos(hyphen_buf, &hyphen_glyph_count);
  hb_shape(font, hyphen_buf, NULL, 0);
  hyphen_glyph_info = hb_buffer_get_glyph_infos(hyphen_buf, &hyphen_glyph_count);
  hb_glyph_position_t* hyphen_glyph_pos = hb_buffer_get_glyph_positions(hyphen_buf, &hyphen_glyph_count);
  FT_Load_Char(ftface, 0x2010, FT_LOAD_DEFAULT);
  hyphen_glyph_i = ftface->glyph->glyph_index;
  hyphen_advance_x = hyphen_glyph_pos[0].x_advance;
  hyphen_offset_x = hyphen_glyph_pos[0].x_offset;
  hyphen_offset_y = hyphen_glyph_pos[0].y_offset;

  for(int i = 0; i < 1000; i++)
  {
    glyphs[i].codepoint = i;
    FT_Load_Glyph(ftface, i, FT_LOAD_DEFAULT);
    FT_Render_Glyph(ftface->glyph, FT_RENDER_MODE_NORMAL);
    if (cursor_x + ftface->glyph->bitmap.width >= 2048)
    {
      cursor_y = cursor_y_max + 2;
      cursor_x = 2;
    }
    glyphs[i].bitmap_left   = ftface->glyph->bitmap_left;
    glyphs[i].bitmap_top    = ftface->glyph->bitmap_top;
    glyphs[i].bitmap_rows   = ftface->glyph->bitmap.rows;
    glyphs[i].bitmap_width  = ftface->glyph->bitmap.width;
    glyphs[i].u_min = (cursor_x) / 2048.0f;
    glyphs[i].v_min = (cursor_y) / 2048.0f;
    glyphs[i].u_max = (cursor_x + ftface->glyph->bitmap.width) / 2048.0f;
    glyphs[i].v_max = (cursor_y + ftface->glyph->bitmap.rows) / 2048.0f;
    int cursor_x_local = cursor_x;
    int cursor_y_local = cursor_y;
    int cursor_x_max = cursor_x_local;
    for (int j = 0; j < ftface->glyph->bitmap.rows; j++)
    {
      for (int k = 0; k < ftface->glyph->bitmap.width; k++)
      {
        int pix = j * ftface->glyph->bitmap.pitch + k;
        buffer[cursor_y_local * 2048 + cursor_x_local] = ftface->glyph->bitmap.buffer[pix];
        cursor_x_local++;
        cursor_x_max = (cursor_x_local > cursor_x_max) ? cursor_x_local : cursor_x_max;
        cursor_y_max = (cursor_y_local > cursor_y_max) ? cursor_y_local : cursor_y_max;
      }
      cursor_x_local = cursor_x;
      cursor_y_local++;
    }
    cursor_x = cursor_x_max;
    cursor_x += 2;

    // Hyphen advances

    const char* hyphen_pair_cstr = "‐";
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
    hb_glyph_position_t* hyphen_pair_glyph_pos = hb_buffer_get_glyph_positions(hyphen_pair_buf, &hyphen_pair_glyph_count);
    glyphs[i].x_advance_hyphen = hyphen_pair_glyph_pos[0].x_advance;
    glyphs[i].x_offset_hyphen = hyphen_pair_glyph_pos[1].x_offset;
    glyphs[i].y_offset_hyphen = hyphen_pair_glyph_pos[1].y_offset;
  }

  WNDCLASSA cls = {0};
  cls.style = 0;
  cls.lpfnWndProc = wndproc;
  cls.cbClsExtra = 0;
  cls.cbWndExtra = 0;
  cls.hInstance = GetModuleHandle(NULL);
  cls.hIcon = NULL;
  cls.hCursor = NULL;
  cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  cls.lpszMenuName = NULL;
  cls.lpszClassName = "lexkit";
  RegisterClassA(&cls);

  HWND hwnd = CreateWindowA(
      "lexkit",
      "lexkit",
      WS_OVERLAPPEDWINDOW,
      0,
      0,
      800,
      800,
      NULL,
      NULL,
      GetModuleHandle(NULL),
      0);

  HDC hdc = GetDC(hwnd);
  DWORD pixel_format_flags =  PFD_SUPPORT_OPENGL | PFD_SUPPORT_COMPOSITION |
                              PFD_GENERIC_ACCELERATED | PFD_DRAW_TO_WINDOW | 
                              PFD_DOUBLEBUFFER;
  PIXELFORMATDESCRIPTOR pfd = {0};
  pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  pfd.nVersion = 1;
  pfd.dwFlags = pixel_format_flags;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 32;
  pfd.cRedBits = 0;
  pfd.cRedShift = 0;
  pfd.cGreenBits = 0;
  pfd.cGreenShift = 0;
  pfd.cBlueBits = 0;
  pfd.cBlueShift = 0;
  pfd.cAlphaBits = 0;
  pfd.cAlphaShift = 0;
  pfd.cAccumBits = 0;
  pfd.cAccumRedBits = 0;
  pfd.cAccumGreenBits = 0;
  pfd.cAccumBlueBits = 0;
  pfd.cAccumAlphaBits = 0;
  pfd.cDepthBits = 24;
  pfd.cStencilBits = 8;
  pfd.cAuxBuffers = 0;
  pfd.iLayerType = PFD_MAIN_PLANE;
  pfd.bReserved = 0;
  pfd.dwLayerMask = 0;
  pfd.dwVisibleMask = 0;
  pfd.dwDamageMask = 0;
  int pixel_format = ChoosePixelFormat(hdc, &pfd);
  SetPixelFormat(hdc, pixel_format, &pfd);

  HGLRC hglrc_dummy = wglCreateContext(hdc);
  wglMakeCurrent(hdc, hglrc_dummy);
  int success = gladLoadWGL(hdc, (GLADloadfunc)glad_load_func);
  success = gladLoadGL((GLADloadfunc)glad_load_func);
  wglMakeCurrent(hdc, NULL);

  int attrib_list[] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    WGL_CONTEXT_MINOR_VERSION_ARB, 6,
    0
  };

  HGLRC hglrc = wglCreateContextAttribsARB(hdc, 0, attrib_list);
  wglMakeCurrent(hdc, hglrc);
  wglDeleteContext(hglrc_dummy);
  ReleaseDC(hwnd, hdc);
  ShowWindow(hwnd, SW_SHOW);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RED,
      2048,
      2048,
      0,
      GL_RED,
      GL_UNSIGNED_BYTE,
      buffer);
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

  int64_t ts = timestamp();
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
    int64_t ts_new = timestamp();
    int64_t dts = ts_new - ts;
    ts = ts_new;
    ts_acc += dts;
    if (ts_acc >= timestamp_res / 60)
    {
      Breaker brk = {0};
      BreakerCreate(codepoints, glyph_count, &brk);

      RECT client_rect = {0};
      GetClientRect(hwnd, &client_rect);
      int w = client_rect.right - client_rect.left;
      int h = client_rect.bottom - client_rect.top;
      glViewport(0, 0, w, h);
      glClearColor((float) 0x21 / 0xFF, (float) 0x21 / 0xFF, (float) 0x21 / 0xFF, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      float cursor_x = 0.0f;
      float cursor_y = 0.0f;
      float ascent = ftface->size->metrics.ascender;
      float descent = ftface->size->metrics.descender;
      float line_gap = ftface->size->metrics.height;
      int next_break = -1;
      bool parse_failure = false;
      int gi = 0;
      bool can_line_break = false;
      bool can_line_break_before_word = false;
      int vdc = 0;
      while (gi < glyph_count)
      {
        float word_advance = 0.0f;
        int word_start_i = gi;
        int word_end_i = gi;
        bool can_line_break_before_next_word = false;
        float grapheme_advances[GRAPHEME_BREAK_COUNT];
        int grapheme_breaks[GRAPHEME_BREAK_COUNT];
        int grapheme_break_count = 0;
        while (gi < glyph_count)
        {
          BreakerResult res = BreakerAdvance(&brk, &ud);
          word_advance += glyph_pos[gi].x_advance;

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
            if ((cursor_x + grapheme_advances[i] + hyphen_advance_x) / 64.0 < w)
            {
              grapheme_break_i = grapheme_breaks[i];
            }
          }
        }

        // Try word break

        if (grapheme_break_i == -1 && can_line_break && exceeds_line && can_line_break_before_word)
        {
          cursor_x = 0.0f;
          cursor_y += line_gap;
          can_line_break = false;
        }
        else
          can_line_break = true;

        can_line_break_before_word = can_line_break_before_next_word;

        for (int i = word_start_i; i < word_end_i; i++)
        {
          LkFontAtlasGlyph aglyph = glyphs[glyph_info[i].codepoint];
          LkVertexDescriptor_Text v = {
            ((cursor_x + glyph_pos[i].x_offset) / 64.0f + aglyph.bitmap_left) / w,
            ((cursor_y + glyph_pos[i].y_offset + ascent) / 64.0f - aglyph.bitmap_top) / h,
            ((float) aglyph.bitmap_width) / w,
            ((float) aglyph.bitmap_rows) / h,
            aglyph.u_min,
            aglyph.v_min,
            aglyph.u_max,
            aglyph.v_max
          };
          vd[vdc++] = v;
          cursor_x += (grapheme_break_i == i) ?
                        aglyph.x_advance_hyphen :
                        glyph_pos[i].x_advance;
          if (grapheme_break_i == i)
          {
            LkFontAtlasGlyph aglyph_hyphen = glyphs[hyphen_glyph_i];
            LkVertexDescriptor_Text v = {
              ((cursor_x + aglyph.x_offset_hyphen) / 64.0f + aglyph_hyphen.bitmap_left) / w,
              ((cursor_y + aglyph.y_offset_hyphen + ascent) / 64.0f - aglyph_hyphen.bitmap_top) / h,
              ((float) aglyph_hyphen.bitmap_width) / w,
              ((float) aglyph_hyphen.bitmap_rows) / h,
              aglyph_hyphen.u_min,
              aglyph_hyphen.v_min,
              aglyph_hyphen.u_max,
              aglyph_hyphen.v_max
            };
            vd[vdc++] = v;
            cursor_x = 0.0f;
            cursor_y += line_gap;
            can_line_break = false;
          }
        }
      }

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
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, len_cstr);

      HDC hdc = GetDC(hwnd);
      SwapBuffers(hdc);
      ReleaseDC(hwnd, hdc);
    }
  }
  return 0;
}

