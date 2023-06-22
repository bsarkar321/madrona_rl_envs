# Install script for directory: /content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "RelWithDebInfo")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/content/madrona_rl_envs/external/madrona/external/madrona-toolchain/bundled-toolchain/toolchain/bin/llvm-objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/content/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/SPIRV/libSPVRemapper.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/content/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/SPIRV/libSPIRV.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake" TYPE FILE FILES "/content/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/SPIRV/SPVRemapperTargets.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake" TYPE FILE FILES "/content/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/SPIRV/SPIRVTargets.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/glslang/SPIRV" TYPE FILE FILES
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/bitutils.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/spirv.hpp"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/GLSL.std.450.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/GLSL.ext.EXT.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/GLSL.ext.KHR.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/GlslangToSpv.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/hex_float.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/Logger.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/SpvBuilder.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/spvIR.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/doc.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/SpvTools.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/disassemble.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/GLSL.ext.AMD.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/GLSL.ext.NV.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/GLSL.ext.ARM.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/NonSemanticDebugPrintf.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/NonSemanticShaderDebugInfo100.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/SPVRemapper.h"
    "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/SPIRV/doc.h"
    )
endif()

