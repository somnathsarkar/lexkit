cmake_minimum_required(VERSION 3.20)
project(lexkit)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(LEXKIT_HARFBUZZ_DIR "" CACHE PATH "HarfBuzz root")
set(LEXKIT_FREETYPE_DIR "" CACHE PATH "FreeType root")
if(NOT EXISTS ${LEXKIT_HARFBUZZ_DIR}/src/hb.h)
  message(FATAL_ERROR "HarfBuzz not found at '${LEXKIT_HARFBUZZ_DIR}'. Configure with -DLEXKIT_HARFBUZZ_DIR=<Path to HarfBuzz root>")
endif()
if(NOT EXISTS ${LEXKIT_FREETYPE_DIR}/include/ft2build.h)
  message(FATAL_ERROR "FreeType not found at '${LEXKIT_FREETYPE_DIR}'. Configure with -DLEXKIT_FREETYPE_DIR=<Path to FreeType root>")
endif()

add_library(lexkit)
target_sources(
  lexkit PRIVATE
  lexkit/alloc.c
  lexkit/lexkit.c
  lexkit/break.c
  lexkit/bidi.c
  lexkit/sizes.c
  lexkit/perf.c
  lexkit/twostep.c
  lexkit/job.c
)
target_include_directories(lexkit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${LEXKIT_HARFBUZZ_DIR}/src ${LEXKIT_FREETYPE_DIR}/include)
target_link_directories(lexkit PUBLIC ${LEXKIT_HARFBUZZ_DIR}/build/$<CONFIG> ${LEXKIT_FREETYPE_DIR}/build/$<CONFIG>)
target_link_libraries(lexkit PUBLIC harfbuzz.lib debug freetyped.lib optimized freetype.lib)
target_compile_options(lexkit PUBLIC /arch:AVX2)

add_executable(example01)
target_sources(
  example01 PRIVATE
  examples/example_opengl.c
  examples/example01.c
  extern/glad/gl.c
  extern/glad/wgl.c)
target_include_directories(example01 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(example01 PRIVATE Opengl32.lib lexkit)

function(target_shaders tgt policy)
  foreach(shd ${ARGN})
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/generated/shaders/spv/${shd}.spv
      COMMAND glslangvalidator -G100 ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shd} -o ${CMAKE_CURRENT_SOURCE_DIR}/generated/shaders/spv/${shd}.spv
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shd}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      VERBATIM
    )
    string(REPLACE "." "_" spv_name ${shd})
    message(${spv_name})
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/generated/shaders/${shd}.h
      COMMAND xxd -i -n ${spv_name} ${CMAKE_CURRENT_SOURCE_DIR}/generated/shaders/spv/${shd}.spv ${CMAKE_CURRENT_SOURCE_DIR}/generated/shaders/${shd}.h
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/generated/shaders/spv/${shd}.spv
      VERBATIM
    )
    target_sources(${tgt} ${policy} ${CMAKE_CURRENT_SOURCE_DIR}/generated/shaders/${shd}.h)
  endforeach()
endfunction(target_shaders)
target_shaders(example01 PRIVATE fullscreen.frag fullscreen.vert text.frag text.vert)

add_executable(example02)
target_sources(
  example02 PRIVATE
  examples/example_opengl.c
  examples/example02.c
  extern/glad/gl.c
  extern/glad/wgl.c)
target_include_directories(example02 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(example02 PRIVATE Opengl32.lib lexkit)
target_shaders(example02 PRIVATE fullscreen.frag fullscreen.vert text.frag text.vert)

add_executable(example03)
target_sources(
  example03 PRIVATE
  examples/example_opengl.c
  examples/example03.c
  extern/glad/gl.c
  extern/glad/wgl.c)
target_include_directories(example03 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(example03 PRIVATE Opengl32.lib lexkit)
target_shaders(example03 PRIVATE fullscreen.frag fullscreen.vert text.frag text.vert)
