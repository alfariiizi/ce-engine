cmake_minimum_required( VERSION 3.16 )

include_guard( GLOBAL )

project( ce-engine )

set( CMAKE_CXX_STANDARD 17 )

# set( external-path ${CMAKE_SOURCE_DIR}/external ) # it'll be used on child cmake to access external path

add_definitions( -DSHADER_PATH="${CMAKE_SOURCE_DIR}/shaders" )
# add_definitions( -DVERTEX_SHADER_PATH="${CMAKE_SOURCE_DIR}/shaders/spv/vertex" )
# add_definitions( -DFRAGMENT_SHADER_PATH="${CMAKE_SOURCE_DIR}/shaders/spv/fragment" )
# add_definitions( -DASSETS_PATH="${CMAKE_SOURCE_DIR}/assets" )

add_executable( main-exec
    src/main.cpp
)

add_subdirectory( src )

target_include_directories( engineSystem
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/external/VulkanMemoryAllocator-Hpp/include
)

target_link_libraries( main-exec
    PUBLIC
       engineSystem
)
