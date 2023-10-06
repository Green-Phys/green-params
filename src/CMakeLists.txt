project(params)

Include(FetchContent)

FetchContent_Declare(
        magic_enum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum
        GIT_TAG v0.9.3 # or a later release
)

FetchContent_MakeAvailable(magic_enum)

add_library(params INTERFACE)
include_directories(.)
target_include_directories(params INTERFACE .)
target_link_libraries(params INTERFACE magic_enum::magic_enum)
