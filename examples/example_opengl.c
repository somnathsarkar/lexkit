#include <examples/example_opengl.h>

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

int64_t timestamp_win64()
{
  LARGE_INTEGER res = {0};
  QueryPerformanceCounter(&res);
  return res.QuadPart;
}

int64_t timestamp_win64_res()
{
  static int64_t res = -1;
  if (res == -1)
  {
    LARGE_INTEGER qpf = {0};
    QueryPerformanceFrequency(&qpf);
    res = qpf.QuadPart;
  }
  return res;
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

HWND create_window()
{
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
  return hwnd;
}
