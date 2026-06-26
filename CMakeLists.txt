cmake_minimum_required(VERSION 3.30)

# Adoptar el nombre del proyecto a partir del nombre del directorio actual
get_filename_component(PROJECT_NAME_VAR "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
project(${PROJECT_NAME_VAR} LANGUAGES CXX)

# Establecer un directorio de instalación predeterminado al inicio para que todas las dependencias lo hereden si es necesario
set(CMAKE_INSTALL_PREFIX "C:/Dev/tesis/lsg-client/src/shared" CACHE PATH "Default install path" FORCE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Buscar dependencias (vcpkg las provee automáticamente)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Fuentes
set(SOURCES
        src/main.cpp
        src/ui.cpp
        src/network.cpp
        src/overlay.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE "$<NOT:$<CONFIG:Debug>>"
)

# Directorios de inclusión
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(${PROJECT_NAME} PRIVATE APP_NAME="${PROJECT_NAME}")

# Advertencias del compilador
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)
endif ()

# Vinculación de librerías
target_link_libraries(${PROJECT_NAME} PRIVATE
        SDL3::SDL3
        imgui::imgui
        httplib::httplib
        nlohmann_json::nlohmann_json
)

# Windows: Copiar DLLs automáticamente al directorio de salida de la compilación
if (WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMAND_EXPAND_LISTS
    )
endif ()

# Copiar assets si el directorio existe (Windows: copiar, Linux: enlace simbólico)
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

# Instalación
install(TARGETS ${PROJECT_NAME} DESTINATION .)

if (WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> DESTINATION . OPTIONAL)
endif ()
