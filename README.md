# lexkit

Text Layout Library

## Build Instructions

We use the CMake build system. Currently, we support building on Windows 64-bit systems only.

1. Clone and build [HarfBuzz](https://github.com/harfbuzz/harfbuzz). Configure `LEXKIT_HARFBUZZ_DIR` to HarfBuzz root directory.
2. Clone and build [FreeType](https://freetype.org). Configure `LEXKIT_FREETYPE_DIR` to FreeType root directory.

The examples use OpenGL for rendering. Building shaders requires:

1. `glslangValidator`, from the Vulkan SDK, or [standalone](https://github.com/KhronosGroup/glslang/releases).
2. `xxd`, typically included with [Git for Windows](https://gitforwindows.org/)

## License Information

This work is made available under the [MIT License](LICENSE.txt)
