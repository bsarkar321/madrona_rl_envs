# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/content/madrona_rl_envs/external/madrona/external/madrona-toolchain/bundled-toolchain"
  "/content/madrona_rl_envs/build/_deps/madronabundledtoolchain-build"
  "/content/madrona_rl_envs/build/_deps/madronabundledtoolchain-subbuild/madronabundledtoolchain-populate-prefix"
  "/content/madrona_rl_envs/build/_deps/madronabundledtoolchain-subbuild/madronabundledtoolchain-populate-prefix/tmp"
  "/content/madrona_rl_envs/external/madrona/external/madrona-toolchain/download/timestamps"
  "/content/madrona_rl_envs/external/madrona/external/madrona-toolchain/download"
  "/content/madrona_rl_envs/external/madrona/external/madrona-toolchain/download/timestamps"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/content/madrona_rl_envs/external/madrona/external/madrona-toolchain/download/timestamps/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/content/madrona_rl_envs/external/madrona/external/madrona-toolchain/download/timestamps${cfgdir}") # cfgdir has leading slash
endif()
