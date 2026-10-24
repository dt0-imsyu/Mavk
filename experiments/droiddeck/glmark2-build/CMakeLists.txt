cmake_minimum_required(VERSION 3.20)
project(mavk_glmark2_comparison LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(source /mnt/e/Mavk/research/glmark2 CACHE PATH "Upstream checkout")
set(sdk /mnt/e/Mavk/artifacts/zink-ui-25.1.9-20261019/sdk CACHE PATH "Runtime headers and libraries")
file(READ ${source}/src/meson.build meson)
string(REGEX MATCH "common_sources = \\[[^]]*\\]" common "${meson}")
string(REGEX MATCHALL "'[^']+'" entries "${common}")
foreach(entry IN LISTS entries)
  string(REPLACE "'" "" entry "${entry}")
  list(APPEND sources ${source}/src/${entry})
endforeach()
file(READ ${source}/android/jni/Android.ndk.mk ndk)
string(REGEX MATCHALL "src/libjpeg-turbo/[a-zA-Z0-9_]+\\.c" jpeg "${ndk}")
list(REMOVE_DUPLICATES jpeg)
foreach(entry IN LISTS jpeg)
  list(APPEND sources ${source}/${entry})
endforeach()
file(GLOB png ${source}/src/libpng/*.c)
option(MAVK_NULL_NATIVE "Use upstream surfaceless/offscreen native state" OFF)
if(MAVK_NULL_NATIVE)
  set(native_source ${source}/src/native-state-null.cpp)
  set(native_define GLMARK2_USE_NULL)
else()
  set(native_source ${source}/src/native-state-x11.cpp)
  set(native_define GLMARK2_USE_X11)
endif()
add_executable(glmark2-es2 ${sources} ${png}
  ${source}/src/main.cpp ${native_source}
  ${source}/src/gl-state-egl.cpp ${source}/src/glad/src/egl.c ${source}/src/glad/src/gles2.c)
target_include_directories(glmark2-es2 PRIVATE ${source}/src ${source}/src/libmatrix
  ${source}/src/glad/include ${source}/src/libpng ${source}/src/libjpeg-turbo
  ${source}/src/scene-ideas ${source}/src/scene-terrain)
target_compile_definitions(glmark2-es2 PRIVATE GLMARK2_USE_GLESv2 ${native_define}
  GLMARK2_USE_EGL USE_EXCEPTIONS PNG_ARM_NEON_OPT=0
  GLMARK_VERSION="2023.01" GLMARK2_EXECUTABLE="glmark2-es2"
  GLMARK_DATA_PATH="/data/user/0/com.droiddeck.launcher.mavk/files/mavk-glmark2/data")
target_link_libraries(glmark2-es2 PRIVATE ${sdk}/usr/lib/libX11.so ${sdk}/usr/lib/libz.so m dl pthread)
