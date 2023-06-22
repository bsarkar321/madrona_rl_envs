# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.25.2)

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(madronabundledtoolchain-populate NONE)



include(ExternalProject)
ExternalProject_Add(madronabundledtoolchain-populate
                     "UPDATE_DISCONNECTED" "False" "DOWNLOAD_EXTRACT_TIMESTAMP" "TRUE" "URL" "https://github.com/shacklettbp/madrona-toolchain/releases/download/8875061/madrona-toolchain-8875061-linux.tar.xz" "URL_HASH" "SHA256=610abe620455b14827a16ecbb56084efb82c2a4c5013c0b70df7c99e759b095e" "DOWNLOAD_DIR" "/content/madrona_rl_envs/external/madrona/external/madrona-toolchain/download" "DOWNLOAD_NAME" "cur.tar" "STAMP_DIR" "/content/madrona_rl_envs/external/madrona/external/madrona-toolchain/download/timestamps"
                    SOURCE_DIR          "/content/madrona_rl_envs/external/madrona/external/madrona-toolchain/bundled-toolchain"
                    BINARY_DIR          "/content/madrona_rl_envs/build/_deps/madronabundledtoolchain-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
                    USES_TERMINAL_PATCH     YES
)


