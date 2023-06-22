# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/content/madrona_rl_envs/build/_deps/madronaprecompileddeps-src"
  "/content/madrona_rl_envs/build/_deps/madronaprecompileddeps-build"
  "/content/madrona_rl_envs/build/_deps/madronaprecompileddeps-subbuild/madronaprecompileddeps-populate-prefix"
  "/content/madrona_rl_envs/build/_deps/madronaprecompileddeps-subbuild/madronaprecompileddeps-populate-prefix/tmp"
  "/content/madrona_rl_envs/build/_deps/madronaprecompileddeps-subbuild/madronaprecompileddeps-populate-prefix/src/madronaprecompileddeps-populate-stamp"
  "/content/madrona_rl_envs/external/madrona/external/madrona-deps/download"
  "/content/madrona_rl_envs/build/_deps/madronaprecompileddeps-subbuild/madronaprecompileddeps-populate-prefix/src/madronaprecompileddeps-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/content/madrona_rl_envs/build/_deps/madronaprecompileddeps-subbuild/madronaprecompileddeps-populate-prefix/src/madronaprecompileddeps-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/content/madrona_rl_envs/build/_deps/madronaprecompileddeps-subbuild/madronaprecompileddeps-populate-prefix/src/madronaprecompileddeps-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
