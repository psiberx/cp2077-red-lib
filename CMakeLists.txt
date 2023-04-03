cmake_minimum_required(VERSION 3.21)

project(RedLib VERSION 1.0.0 LANGUAGES CXX)

add_library(RedLib INTERFACE)
target_include_directories(RedLib INTERFACE include vendor)
