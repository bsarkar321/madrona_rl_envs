# CMake generated Testfile for 
# Source directory: /content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang
# Build directory: /content/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(glslang-testsuite "bash" "runtests" "/content/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/localResults" "/content/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/StandAlone/glslangValidator" "/content/madrona_rl_envs/build/external/madrona/src/render/vk/external/glslang/StandAlone/spirv-remap")
set_tests_properties(glslang-testsuite PROPERTIES  WORKING_DIRECTORY "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/Test/" _BACKTRACE_TRIPLES "/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/CMakeLists.txt;367;add_test;/content/madrona_rl_envs/external/madrona/src/render/vk/external/glslang/CMakeLists.txt;0;")
subdirs("External")
subdirs("glslang")
subdirs("OGLCompilersDLL")
subdirs("StandAlone")
subdirs("SPIRV")
subdirs("hlsl")
subdirs("gtests")
