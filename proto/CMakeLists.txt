# 确保找到protobuf
find_package(Protobuf REQUIRED)

# 设置输出目录
set(PROTO_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/million_protogen")

# 设置导出宏
set(PROTOGEN_EXPORT_MACRO "MILLION_PROTOGEN_API")

# 收集所有proto文件
file(GLOB_RECURSE PROTO_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/**/*.proto"
)

# 创建输出目录
file(MAKE_DIRECTORY "${PROTO_OUTPUT_DIR}")

# 生成的C++文件列表
set(GENERATED_HEADERS)
set(GENERATED_SOURCES)

# 为每个proto文件生成对应的C++文件
foreach(proto_file ${PROTO_FILES})
    # 获取相对路径和文件名
    file(RELATIVE_PATH proto_rel_path "${CMAKE_CURRENT_SOURCE_DIR}" "${proto_file}")
    get_filename_component(proto_dir "${proto_rel_path}" DIRECTORY)
    get_filename_component(proto_name "${proto_file}" NAME_WE)
    
    # 设置生成的文件路径
    set(generated_header "${PROTO_OUTPUT_DIR}/${proto_dir}/${proto_name}.pb.h")
    set(generated_source "${PROTO_OUTPUT_DIR}/${proto_dir}/${proto_name}.pb.cc")
    
    # 添加到生成文件列表
    list(APPEND GENERATED_HEADERS "${generated_header}")
    list(APPEND GENERATED_SOURCES "${generated_source}")
    
    # 创建必要的目录
    file(MAKE_DIRECTORY "${PROTO_OUTPUT_DIR}/${proto_dir}")
    
    # 添加自定义命令来生成C++文件
    add_custom_command(
        OUTPUT "${generated_header}" "${generated_source}"
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            --cpp_out=dllexport_decl=${PROTOGEN_EXPORT_MACRO}:${PROTO_OUTPUT_DIR}
            --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
            ${proto_rel_path}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
            "${PROTO_OUTPUT_DIR}/${proto_dir}/${proto_name}.pb.h"
            "${generated_header}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
            "${PROTO_OUTPUT_DIR}/${proto_dir}/${proto_name}.pb.cc"
            "${generated_source}"
        DEPENDS "${proto_file}"
        COMMENT "Generating protobuf files for ${proto_rel_path}"
        VERBATIM
    )
endforeach()

# 创建自定义目标
add_custom_target(million_proto_generates ALL
    DEPENDS ${GENERATED_HEADERS} ${GENERATED_SOURCES}
    COMMENT "generate protobuf C++ files"
    VERBATIM
)

# 添加对 protoc 的依赖
add_dependencies(million_proto_generates protobuf::protoc)

# 导出生成的文件列表供其他目标使用
set(MILLION_PROTO_GENERATED_HEADERS ${GENERATED_HEADERS})
set(MILLION_PROTO_GENERATED_SOURCES ${GENERATED_SOURCES})

# 设置生成文件的属性
# set_source_files_properties(${MILLION_PROTO_GENERATED_HEADERS} ${MILLION_PROTO_GENERATED_SOURCES}
#     PROPERTIES GENERATED TRUE
# )

# 显示生成的文件信息
message(STATUS "Protobuf configuration:")
message(STATUS "  Output directory: ${PROTO_OUTPUT_DIR}")
message(STATUS "  Protoc executable: ${Protobuf_PROTOC_EXECUTABLE}")
message(STATUS "  Found ${PROTO_FILES_COUNT} proto files")



if (WIN32)
    set(PROTOGEN_COMPILE_DEFINITIONS MILLION_PROTOGEN_API=__declspec\(dllimport\) CACHE STRING "protobuf compile definitions")
elseif(UNIX)
    set(PROTOGEN_COMPILE_DEFINITIONS MILLION_PROTOGEN_API=__attribute__\(\(visibility\(\"hidden\"\)\)\) CACHE STRING "protobuf compile definitions")
endif()


# 设置生成文件的属性
set_source_files_properties(${MILLION_PROTO_GENERATED_HEADERS} ${MILLION_PROTO_GENERATED_SOURCES}
    PROPERTIES GENERATED TRUE
)

set(MILLION_PROTOGEN_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/million_protogen)
set(MILLION_PROTOGEN_LIB_TARGET million_protogen)

# message("${MILLION_PROTO_GENERATED_HEADERS}")
# message("${MILLION_PROTO_GENERATED_SOURCES}")
    

add_library(${MILLION_PROTOGEN_LIB_TARGET} SHARED
    ${MILLION_PROTO_GENERATED_HEADERS} 
    ${MILLION_PROTO_GENERATED_SOURCES}
)

target_compile_definitions(${MILLION_PROTOGEN_LIB_TARGET} PUBLIC ${PROTOGEN_COMPILE_DEFINITIONS})
target_include_directories(${MILLION_PROTOGEN_LIB_TARGET} PUBLIC ${MILLION_PROTOGEN_INCLUDE_DIR})
target_include_directories(${MILLION_PROTOGEN_LIB_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (WIN32)
    target_compile_definitions(${MILLION_PROTOGEN_LIB_TARGET} PRIVATE MILLION_PROTOGEN_API=__declspec\(dllexport\))
elseif(UNIX)
    target_compile_definitions(${MILLION_PROTOGEN_LIB_TARGET} PRIVATE MILLION_PROTOGEN_API=__attribute__\(\(visibility\(\"default\"\)\)\))
endif()

# 添加对proto生成目标的依赖
add_dependencies(${MILLION_PROTOGEN_LIB_TARGET} million_proto_generates)

target_link_libraries(${MILLION_PROTOGEN_LIB_TARGET} PUBLIC million::third_party::protobuf)

add_library(million::protogen ALIAS ${MILLION_PROTOGEN_LIB_TARGET})