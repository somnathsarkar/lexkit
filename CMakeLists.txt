cmake_minimum_required(VERSION 3.20)
project(lexkit)
add_library(lexkit)
target_sources(
  lexkit PRIVATE
  lexkit/lexkit.c
  lexkit/break.c
)
target_include_directories(lexkit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} C:/Dev/harfbuzz-10.2.0/src C:/Dev/freetype-2.13.3/include)
target_link_directories(lexkit PUBLIC C:/Dev/harfbuzz-10.2.0/build/Debug C:/Dev/freetype-2.13.3/build/Debug)
target_link_libraries(lexkit PUBLIC harfbuzz.lib freetyped.lib)

add_executable(example01)
target_sources(
  example01 PRIVATE
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
  examples/example02.c
  extern/glad/gl.c
  extern/glad/wgl.c)
target_include_directories(example02 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(example02 PRIVATE Opengl32.lib lexkit)
target_shaders(example02 PRIVATE fullscreen.frag fullscreen.vert text.frag text.vert)
