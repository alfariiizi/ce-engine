cmake_minimum_required( VERSION 3.16 )

project( externalDepend )

add_library( externalDependLib
    ${CMAKE_CURRENT_SOURCE_DIR}/SimpleCppBenchmark/SimpleBenchmark.cpp
)

target_include_directories( externalDependLib 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/SimpleCppBenchmark
        ${CMAKE_CURRENT_SOURCE_DIR}/external/VulkanMemoryAllocator-Hpp/include
)