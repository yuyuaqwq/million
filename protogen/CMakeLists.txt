if (WIN32)
    set(PROTOGEN_COMPILE_DEFINITIONS PROTOGEN_API=__declspec\(dllimport\) CACHE STRING "protobuf compile definitions")
elseif(UNIX)
    set(PROTOGEN_COMPILE_DEFINITIONS PROTOGEN_API=__attribute__\(\(visibility\(\"hidden\"\)\)\) CACHE STRING "protobuf compile definitions")
endif()
set(PROTOGEN_HEADER_DIR 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    CACHE STRING "protobuf header dir")
set(PROTOGEN_LIB protogen CACHE STRING "protobuf lib")

add_library(${PROTOGEN_LIB} SHARED)

target_sources(${PROTOGEN_LIB} PRIVATE 
    ${PROTOBUF_GENERATED_SOURCES} 
    ${PROTOBUF_GENERATED_HEADERS}
)

target_compile_definitions(${PROTOGEN_LIB} PUBLIC ${PROTOGEN_COMPILE_DEFINITIONS})
target_include_directories(${PROTOGEN_LIB} PUBLIC ${PROTOGEN_HEADER_DIR})
target_include_directories(${PROTOGEN_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (WIN32)
    target_compile_definitions(${PROTOGEN_LIB} PRIVATE PROTOGEN_API=__declspec\(dllexport\))
elseif(UNIX)
    target_compile_definitions(${PROTOGEN_LIB} PRIVATE PROTOGEN_API=__attribute__\(\(visibility\(\"default\"\)\)\))
endif()

# 添加对proto生成目标的依赖
add_dependencies(${PROTOGEN_LIB} million_proto_generates)

# 添加生成文件所在的include目录
target_include_directories(${PROTOGEN_LIB} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/../protogen/include)

target_link_libraries(${PROTOGEN_LIB} PUBLIC ${PROTOBUF_LIB})