# Install script for directory: /iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/external/madrona/src/render/vk/external/glslang

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
  set(CMAKE_OBJDUMP "/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/external/madrona/external/madrona-toolchain/bundled-toolchain/toolchain/bin/llvm-objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/External/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/glslang/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/OGLCompilersDLL/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/StandAlone/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/SPIRV/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/hlsl/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/gtests/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/glslang/glslang-targets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/glslang/glslang-targets.cmake"
         "/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/CMakeFiles/Export/43b0efddc38c54c95da93ebc2a9ba55e/glslang-targets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/glslang/glslang-targets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/glslang/glslang-targets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/glslang" TYPE FILE FILES "/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/CMakeFiles/Export/43b0efddc38c54c95da93ebc2a9ba55e/glslang-targets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/glslang" TYPE FILE FILES "/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/CMakeFiles/Export/43b0efddc38c54c95da93ebc2a9ba55e/glslang-targets-relwithdebinfo.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/glslang" TYPE FILE FILES
    "/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/glslang-config.cmake"
    "/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/glslang-config-version.cmake"
    )
endif()

