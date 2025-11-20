cmake_minimum_required(VERSION 3.20)

project(addNMult)
enable_language(C)
enable_language(CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig at: ${LLVM_DIR}")

add_executable(addnmult
  main.cpp
  Lexer.cpp
  Parser.cpp
  CodeGen.cpp
  SemanticAnalyzer.cpp
)

target_include_directories(addnmult PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(addnmult PRIVATE ${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(LLVM_LIBS core support)
target_link_libraries(addnmult PRIVATE ${LLVM_LIBS})