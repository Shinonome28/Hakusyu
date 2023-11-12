cmake_minimum_required(VERSION 3.11)
project(LearnSDL2)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/output)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)

set(image_source_ "${CMAKE_SOURCE_DIR}/images")
set(image_destination_ "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/images")
add_custom_target(
    copy_images_
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${image_source_} ${image_destination_} 
    COMMENT "Copying images..."
    VERBATIM
)

set(font_source_ "${CMAKE_SOURCE_DIR}/fonts")
set(font_destionation_ "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/fonts")
add_custom_target(
    copy_fonts_
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${font_source_} ${font_destionation_} 
    COMMENT "Copying fonts..."
    VERBATIM
)   

add_custom_target(copy_all_ COMMENT "Copying all files...")
add_dependencies(copy_all_ copy_images_ copy_fonts_)

add_subdirectory(src)