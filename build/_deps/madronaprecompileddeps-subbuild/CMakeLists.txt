# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.26.4)

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(madronaprecompileddeps-populate NONE)



include(ExternalProject)
ExternalProject_Add(madronaprecompileddeps-populate
                     "UPDATE_DISCONNECTED" "False" "DOWNLOAD_EXTRACT_TIMESTAMP" "TRUE" "URL" "https://github.com/shacklettbp/madrona-deps/releases/download/ce1b3dc/madrona-deps-ce1b3dc-linux.tar.xz" "URL_HASH" "SHA256=5e77b1fe83e432cfdea8b5fad3de3c653b4fcd2478cdcebc14576d2cc49e1937" "DOWNLOAD_DIR" "/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/external/madrona/external/madrona-deps/download" "DOWNLOAD_NAME" "cur.tar"
                    SOURCE_DIR          "/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/_deps/madronaprecompileddeps-src"
                    BINARY_DIR          "/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/_deps/madronaprecompileddeps-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
                    USES_TERMINAL_PATCH     YES
)


