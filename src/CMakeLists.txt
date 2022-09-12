cmake_minimum_required( VERSION 3.16 )

project( engine )

add_library( engineSystem
    Engine.cpp
    Buffer.cpp
    # vk_init.cpp
    # vk_utils.cpp
)

target_link_libraries( engineSystem
    PUBLIC
        vulkan
        shaderc
        # dl
        # pthread
        # Xxf86vm
        # Xrandr
        # Xi
)

target_include_directories( engineSystem 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

