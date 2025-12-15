cmake_minimum_required(VERSION 3.15)

project(VK_MiniRender LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(Vulkan REQUIRED)
message(STATUS "Vulkan found: ${Vulkan_FOUND}")
find_program(GLSLC_EXECUTABLE glslc REQUIRED)
message(STATUS "glslc found: ${GLSLC_EXECUTABLE}")

set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shader)
set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR})
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
file(GLOB_RECURSE SHADER_SOURCES 
    "${SHADER_SOURCE_DIR}/*.vert"
    "${SHADER_SOURCE_DIR}/*.frag"
)
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_SPIRV ${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv)

    add_custom_command(
        OUTPUT ${SHADER_SPIRV}
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SHADER_SPIRV}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER_NAME} to SPIR-V"
    )

    list(APPEND SPIRV_FILES ${SHADER_SPIRV})
endforeach()

add_custom_target(
    Shaders ALL
    DEPENDS ${SPIRV_FILES}
)

link_directories(external/glfw/lib)

include_directories(external/stb)
include_directories(external/imgui)
include_directories(external/tinygltf)
include_directories(external/glfw/include)
include_directories(external/glm)
include_directories(external)
include_directories(VKM_Base)
include_directories(${Vulkan_INCLUDE_DIRS})

file(GLOB HEADER_FILES "*.hpp" "*.h")
file(GLOB SOURCE_FILES "*.cpp")

if (MSVC)
    add_compile_options(/std:c++latest)
endif()

add_subdirectory(VKM_Base)
add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME}  PRIVATE ${Vulkan_LIBRARIES} VKM_Base glfw3.lib)
add_dependencies(${PROJECT_NAME} Shaders)