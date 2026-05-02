cmake_minimum_required(VERSION 3.10)
project(ESBMCGeneratedTests CXX)

enable_testing()

option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-O0 -g --coverage)
  add_link_options(--coverage)
endif()

# ASan is essential here: the finding is a stack-buffer out-of-bounds read.
# ASan detects and reports it with a precise trace before the assert is reached.
add_compile_options(-fno-exceptions -fno-rtti -O0 -g -fsanitize=address)
add_link_options(-fsanitize=address)

add_executable(test_case_1 gpio_oob.cpp test_case_1.cpp)
add_test(NAME test_case_1 COMMAND test_case_1)
target_compile_options(test_case_1 PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/esbmc_verifier.h)
