cmake_minimum_required(VERSION 3.26)
project(velka_uloha)

set(CMAKE_CXX_STANDARD 20)

add_executable(velka_uloha main.cpp
        expressionBuilderAST.h
        all_in_one.cpp
        tests.h
)
