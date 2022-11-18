set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB SHADERS ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag
        ${SHADER_DIR}/*.comp ${SHADER_DIR}/*.geom ${SHADER_DIR}/*.tesc
        ${SHADER_DIR}/*.tese ${SHADER_DIR}/*.mesh ${SHADER_DIR}/*.task
        ${SHADER_DIR}/*.rgen ${SHADER_DIR}/*.rchit ${SHADER_DIR}/*.rmiss)

find_package(Vulkan COMPONENTS glslc REQUIRED)
set(SHADERS_OUT ${CMAKE_BINARY_DIR}/shaders)
target_compile_definitions(Renderer PUBLIC SHADER_DIR="${SHADERS_OUT}")
foreach (SHADER IN LISTS SHADERS)
    get_filename_component(FILENAME ${SHADER} NAME)
    add_custom_command(OUTPUT ${SHADERS_OUT}/${FILENAME}.spv
            COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${SHADER} -o ${SHADERS_OUT}/${FILENAME}.spv
            DEPENDS ${SHADER}
            COMMENT "Compiling ${FILENAME}")
    list(APPEND SPV_SHADERS ${SHADERS_OUT}/${FILENAME}.spv)
endForeach ()

add_custom_target(Shaders ALL DEPENDS ${SPV_SHADERS})
add_dependencies(Renderer Shaders)