cmake_minimum_required(VERSION 3.30)

# Adopt the project name from the current directory name
get_filename_component(PROJECT_NAME_VAR "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
project(${PROJECT_NAME_VAR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ── spdlog (logging library) ───────────────────────────────────────────────
# Uncomment to enable:
# FetchContent_Declare(
#     spdlog
#     GIT_REPOSITORY https://github.com/gabime/spdlog.git
#     GIT_TAG        v1.15.2
#     GIT_SHALLOW    TRUE
# )
# FetchContent_MakeAvailable(spdlog)

# ── nlohmann/json (JSON library) ───────────────────────────────────────────
# Uncomment to enable:
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# ── cpp-httplib (HTTP client/server) ────────────────────────────────────────
# Uncomment to enable:
FetchContent_Declare(
        cpp-httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG v0.48.0
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(cpp-httplib)

# ── Box2D (2D physics) ─────────────────────────────────────────────────────
# FetchContent_Declare(
#         box2d
#         GIT_REPOSITORY https://github.com/erincatto/box2d.git
#         GIT_TAG v3.1.1
#         GIT_SHALLOW TRUE
# )
# set(BOX2D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
# set(BOX2D_SAMPLES OFF CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(box2d)

# ── Dear ImGui ─────────────────────────────────────────────────────────────
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.9b
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

# ── SDL3 ────────────────────────────────────────────────────────────────────
set(SDL3_VERSION "3.4.10")
#set(SDL3_IMAGE_VERSION "3.4.4")
#set(SDL3_MIXER_VERSION "3.2.4")
#set(SDL3_TTF_VERSION "3.2.2")

find_package(SDL3 QUIET)
#find_package(SDL3_image QUIET)
#find_package(SDL3_mixer QUIET)
#find_package(SDL3_ttf QUIET)

if (NOT SDL3_FOUND)
    if (WIN32)
        message(STATUS "SDL3 not found. Fetching prebuilt VC binaries...")

        FetchContent_Declare(
                sdl3_prebuilt
                URL "https://github.com/libsdl-org/SDL/releases/download/release-${SDL3_VERSION}/SDL3-devel-${SDL3_VERSION}-VC.zip"
        )
        FetchContent_MakeAvailable(sdl3_prebuilt)

        list(APPEND CMAKE_PREFIX_PATH "${sdl3_prebuilt_SOURCE_DIR}")

        # FetchContent_Declare(
        #         sdl3_image_prebuilt
        #         URL "https://github.com/libsdl-org/SDL_image/releases/download/release-${SDL3_IMAGE_VERSION}/SDL3_image-devel-${SDL3_IMAGE_VERSION}-VC.zip"
        # )
        # FetchContent_MakeAvailable(sdl3_image_prebuilt)
        #
        # FetchContent_Declare(
        #         sdl3_mixer_prebuilt
        #         URL "https://github.com/libsdl-org/SDL_mixer/releases/download/release-${SDL3_MIXER_VERSION}/SDL3_mixer-devel-${SDL3_MIXER_VERSION}-VC.zip"
        # )
        # FetchContent_MakeAvailable(sdl3_mixer_prebuilt)
        #
        # FetchContent_Declare(
        #         sdl3_ttf_prebuilt
        #         URL "https://github.com/libsdl-org/SDL_ttf/releases/download/release-${SDL3_TTF_VERSION}/SDL3_ttf-devel-${SDL3_TTF_VERSION}-VC.zip"
        # )
        # FetchContent_MakeAvailable(sdl3_ttf_prebuilt)
    else ()
        message(FATAL_ERROR "SDL3 not found as system package. "
                "Install it manually (e.g. 'apt install libsdl3-dev' on Debian/Ubuntu, "
                "'brew install sdl3' on macOS, "
                "or use your distro's package manager)."
        )
    endif ()
endif ()

find_package(SDL3 REQUIRED)
#find_package(SDL3_image REQUIRED)
#find_package(SDL3_mixer REQUIRED)
#find_package(SDL3_ttf REQUIRED)

# ── ImGui static library ───────────────────────────────────────────────────
add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC SDL3::SDL3)

# ── Sources ─────────────────────────────────────────────────────────────────
set(SOURCES
        src/main.cpp
        src/ui.cpp
        src/app.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE "$<NOT:$<CONFIG:Debug>>"
)

# ── Include directories ────────────────────────────────────────────────────
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(${PROJECT_NAME} PRIVATE APP_NAME="${PROJECT_NAME}")

# ── Compiler Warnings ───────────────────────────────────────────────────────
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE
        SDL3::SDL3
        # SDL3_image::SDL3_image
        # SDL3_mixer::SDL3_mixer
        # SDL3_ttf::SDL3_ttf
        imgui
        # box2d
        httplib::httplib
        # spdlog::spdlog_header_only
        nlohmann_json::nlohmann_json
)

# Windows: Copy DLLs automatically to the build output directory
if (WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMAND_EXPAND_LISTS
    )
endif ()

# Copy assets if directory exists (Windows: copy, Linux: symlink)
if (EXISTS "${CMAKE_SOURCE_DIR}/assets")
    if (WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/assets"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
        )
    else ()
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
                COMMAND ${CMAKE_COMMAND} -E create_symlink
                "${CMAKE_SOURCE_DIR}/assets"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
        )
    endif ()
endif ()
