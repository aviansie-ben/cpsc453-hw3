# Advanced OpenGL Application CMake 2.0 (w/ compile-time shader embedding)
# Written by Benjamin Thomas <ben@benthomas.ca>

CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT("HW3" CXX)

SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
SET(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/inc")
SET(STB_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/stb")

FIND_PACKAGE(PkgConfig REQUIRED)

PKG_SEARCH_MODULE(OPENGL REQUIRED gl)
PKG_SEARCH_MODULE(GLFW3 REQUIRED glfw3)
PKG_SEARCH_MODULE(GLM REQUIRED glm)
PKG_SEARCH_MODULE(FREETYPE2 REQUIRED freetype2)
PKG_SEARCH_MODULE(FONTCONFIG REQUIRED fontconfig)
FIND_PACKAGE(Boost 1.40 COMPONENTS filesystem system REQUIRED)

FUNCTION(GLSL_GENERATE_CXX INPUT VAR_NAME)
    SET(INPUT ${CMAKE_SOURCE_DIR}/shaders/${INPUT})
    SET(OUTPUT ${CMAKE_BINARY_DIR}/shader_gen/${INPUT}.cpp)

    SET(GLSL_OUTPUTS ${GLSL_OUTPUTS} ${OUTPUT} PARENT_SCOPE)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${OUTPUT}
        COMMAND ${CMAKE_SOURCE_DIR}/gen_glsl_cpp.sh "${INPUT}" "${OUTPUT}" "${VAR_NAME}" "shaderimpl.hpp"
        DEPENDS ${INPUT}
                ${CMAKE_SOURCE_DIR}/gen_glsl_cpp.sh
        VERBATIM
    )
ENDFUNCTION()

FILE(GLOB_RECURSE CXX_SOURCES src/*.cpp)

GLSL_GENERATE_CXX(vertex_simple.glsl "hw3::shaders::impl::vertex_simple")
GLSL_GENERATE_CXX(vertex_textured.glsl "hw3::shaders::impl::vertex_textured")
GLSL_GENERATE_CXX(vertex_textured_normal.glsl "hw3::shaders::impl::vertex_textured_normal")
GLSL_GENERATE_CXX(geometry_point.glsl "hw3::shaders::impl::geometry_point")
GLSL_GENERATE_CXX(fragment_font.glsl "hw3::shaders::impl::fragment_font")
GLSL_GENERATE_CXX(fragment_fixed.glsl "hw3::shaders::impl::fragment_fixed")
GLSL_GENERATE_CXX(fragment_normal.glsl "hw3::shaders::impl::fragment_normal")
GLSL_GENERATE_CXX(fragment_phong.glsl "hw3::shaders::impl::fragment_phong")
GLSL_GENERATE_CXX(fragment_textured.glsl "hw3::shaders::impl::fragment_textured")

ADD_EXECUTABLE(hw3 ${CXX_SOURCES} ${GLSL_OUTPUTS})
TARGET_LINK_LIBRARIES(hw3 ${OPENGL_LIBRARIES} ${GLFW3_LIBRARIES} ${GLM_LIBRARIES} ${FREETYPE2_LIBRARIES} ${FONTCONFIG_LIBRARIES} ${Boost_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(hw3 PUBLIC ${INCLUDE_DIR} ${OPENGL_INCLUDE_DIRS} ${GLFW3_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS} ${STB_INCLUDE_DIRS} ${FREETYPE2_INCLUDE_DIRS} ${FONTCONFIG_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
TARGET_COMPILE_OPTIONS(hw3 PUBLIC ${OPENGL_CFLAGS_OTHER} ${GLFW3_CFLAGS_OTHER} ${GLM_CFLAGS_OTHER} ${FREETYPE2_CFLAGS_OTHER} ${FONTCONFIG_CFLAGS_OTHER})
