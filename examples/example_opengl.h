#include <windows.h>

#include <extern/glad/gl.h>
#include <extern/glad/wgl.h>

#include <lexkit/sizes.h>

LRESULT wndproc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
i64 timestamp();
void* glad_load_func(const char* name);
GLuint create_prog(
    const unsigned char* vs, const unsigned int vs_len,
    const unsigned char* fs, const unsigned int fs_len);
HWND create_window();
