# CMake generated Testfile for 
# Source directory: /iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/external/madrona/src/render/vk/external/glslang
# Build directory: /iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(glslang-testsuite "bash" "runtests" "/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/localResults" "/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/StandAlone/glslangValidator" "/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/StandAlone/spirv-remap")
set_tests_properties(glslang-testsuite PROPERTIES  WORKING_DIRECTORY "/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/Test/" _BACKTRACE_TRIPLES "/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/CMakeLists.txt;367;add_test;/iliad/u/bidiptas/madrona_prebuilt/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/CMakeLists.txt;0;")
subdirs("External")
subdirs("glslang")
subdirs("OGLCompilersDLL")
subdirs("StandAlone")
subdirs("SPIRV")
subdirs("hlsl")
subdirs("gtests")
