
add_executable(Hakusyu game.cpp main.cpp)
target_link_libraries(Hakusyu PRIVATE SDL2::SDL2main SDL2::SDL2 SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf SDL2_mixer::SDL2_mixer)
target_include_directories(Hakusyu PRIVATE .)
add_dependencies(Hakusyu copy_all_)