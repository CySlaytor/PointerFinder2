cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED CMAKE_PREFIX_PATH AND DEFINED ENV{QTDIR})
    set(CMAKE_PREFIX_PATH "$ENV{QTDIR}")
else()
    set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64") # Local fallback path
endif()

project(PointerFinder2 VERSION 2.0.0 LANGUAGES CXX)

# Enable folders in the Visual Studio Solution Explorer. This automatically 
# moves utility projects (ALL_BUILD, ZERO_CHECK) into a subfolder.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Discover Qt dependencies
find_package(Qt6 COMPONENTS Core Widgets Concurrent Svg Network REQUIRED)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Collect all source and header files recursively
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.h"
)

# Collect resources so they are also visible in the IDE Solution Explorer
file(GLOB_RECURSE RESOURCE_ICONS "resources/icons/*.svg")
set(RESOURCES 
    resources/resources.qrc
    ${RESOURCE_ICONS}
)

if(WIN32)
    set(WIN32_RESOURCES resources/resources.rc)
    list(APPEND RESOURCES ${WIN32_RESOURCES})
endif()

# Organize source files mapping physical directories (e.g., UI, Core, Models)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${SOURCES})

# Organize resources under a "Resources" prefix matching their actual folder layout
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/resources" PREFIX "Resources" FILES ${RESOURCES})

if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${RESOURCES})

    # Inject compile definitions dynamically into CL.exe and RC.exe
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        VERSION_STRING="${PROJECT_VERSION}"
        RC_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        RC_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        RC_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        RC_VERSION_BUILD=0
        RC_VERSION_STRING="${PROJECT_VERSION}.0"
    )

    # Enable Level 4 warnings and silence external Qt library static analysis false-positives
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W4 /wd26495 /wd26478 /wd26813)
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE 
        Qt6::Core 
        Qt6::Widgets 
        Qt6::Concurrent
        Qt6::Svg
        Qt6::Network
        Psapi.lib
        winmm.lib 
    )

    # Generate the qt.conf file next to the binary at generation time.
    # This instructs the executable to search for all subfolders inside 'plugins/'.
    file(GENERATE 
        OUTPUT "$<TARGET_FILE_DIR:${PROJECT_NAME}>/qt.conf"
        CONTENT "[Paths]\nPlugins = plugins\n"
    )

    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${CMAKE_PREFIX_PATH}/bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            # 1. Run windeployqt with customized deployment and routing flags
            COMMAND ${WINDEPLOYQT_EXECUTABLE} 
                --no-compiler-runtime 
                --no-opengl-sw 
                --no-translations
                --plugindir "plugins"
                "$<TARGET_FILE:${PROJECT_NAME}>"

            # 2. Delete the unnecessary DirectX/shader compiler DLLs to optimize space
            COMMAND ${CMAKE_COMMAND} -E rm -f 
                "dxcompiler.dll"
                "dxil.dll"
                "D3Dcompiler_47.dll"
            
            # Force the working directory of this post-build step to match the output folder
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Optimizing and deploying Qt dependencies into a clean directory layout..."
        )
    endif()

else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})
    
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        VERSION_STRING="${PROJECT_VERSION}"
    )

    # Enable standard warnings on Unix/Clang toolchains
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)

    target_link_libraries(${PROJECT_NAME} PRIVATE 
        Qt6::Core 
        Qt6::Widgets 
        Qt6::Concurrent
        Qt6::Svg
        Qt6::Network
    )
endif()