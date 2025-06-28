#include <windows.h>

#include <extern/glad/gl.h>
#include <extern/glad/wgl.h>

#include <generated/shaders/fullscreen.vert.h>
#include <generated/shaders/fullscreen.frag.h>
#include <generated/shaders/text.vert.h>
#include <generated/shaders/text.frag.h>

#include <lexkit/lexkit.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

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

LkLine lines[512] = {0};
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

  LkArena* arena = lkArenaCreate(Megabytes(64));
  LkUnicodeData ud = {0};
  bool ud_success = lkTryLoadUnicodeDataFromSpec(
      arena,
      "C:/Code/lexkit/lexkit/LineBreakProperty.txt",
      "C:/Code/lexkit/lexkit/WordBreakProperty.txt",
      "C:/Code/lexkit/lexkit/GraphemeBreakProperty.txt",
      "C:/Code/lexkit/lexkit/UnicodeData.txt",
      "C:/Code/lexkit/lexkit/EastAsianWidth.txt",
      "C:/Code/lexkit/lexkit/DerivedCoreProperties.txt",
      "C:/Code/lexkit/lexkit/emoji-data.txt",
      "C:/Code/lexkit/lexkit/DerivedBidiClass.txt",
      "C:/Code/lexkit/lexkit/BidiBrackets.txt",
      &ud);

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

  LkFont font;
  lkCreateFont(arena, "C:/Dev/Fonts/Hack/Hack Regular Nerd Font Complete.ttf", font_size, &font);
  LkText text;
  lkCreateText(arena, &font, cstr, len_cstr, &text);
  i32 level_run_count = -1;
  LevelRun* level_runs = NULL;
  i32 para_count = -1;
  Paragraph* paragraphs = NULL;
  LkGlyph** glyphs = NULL;
  i32* levels = NULL;
  lkSplitParagraphs(arena, text.codepoints, text.codepoint_count, &ud, &para_count, &paragraphs);
  lkSplitBidiRuns(arena, text.codepoints, text.codepoint_count, &ud, para_count, paragraphs, &levels, &level_run_count, &level_runs);
  lkShapeText(arena, &font, &text, level_run_count, level_runs, &glyphs);

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
      RECT client_rect = {0};
      GetClientRect(hwnd, &client_rect);
      int w = client_rect.right - client_rect.left;
      int h = client_rect.bottom - client_rect.top;
      glViewport(0, 0, w, h);
      glClearColor((float) 0x21 / 0xFF, (float) 0x21 / 0xFF, (float) 0x21 / 0xFF, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      i32 vdc = 0;
      i32 line_count = -1;
      lkSplitLines(arena, &ud, &font, &text, glyphs, para_count, paragraphs, w, h, &line_count, lines);
      lkLayoutText(arena, &ud, &font, &text, levels, glyphs, line_count, lines, w, h, 10240, vd, &vdc);

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
  return 0;
}

