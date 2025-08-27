if (WIN32)
    set(PROTOGEN_COMPILE_DEFINITIONS PROTOGEN_API=__declspec\(dllimport\) CACHE STRING "protobuf compile definitions")
elseif(UNIX)
    set(PROTOGEN_COMPILE_DEFINITIONS PROTOGEN_API=__attribute__\(\(visibility\(\"hidden\"\)\)\) CACHE STRING "protobuf compile definitions")
endif()
set(MILLION_PROTOGEN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(MILLION_PROTOGEN_LIB_TARGET million_protogen)

add_library(${MILLION_PROTOGEN_LIB_TARGET} SHARED)

target_sources(${MILLION_PROTOGEN_LIB_TARGET} PRIVATE 
    ${MILLION_PROTO_GENERATED_HEADERS} 
    ${MILLION_PROTO_GENERATED_SOURCES}
)

target_compile_definitions(${MILLION_PROTOGEN_LIB_TARGET} PUBLIC ${PROTOGEN_COMPILE_DEFINITIONS})
target_include_directories(${MILLION_PROTOGEN_LIB_TARGET} PUBLIC ${MILLION_PROTOGEN_INCLUDE_DIR})
target_include_directories(${MILLION_PROTOGEN_LIB_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (WIN32)
    target_compile_definitions(${MILLION_PROTOGEN_LIB_TARGET} PRIVATE PROTOGEN_API=__declspec\(dllexport\))
elseif(UNIX)
    target_compile_definitions(${MILLION_PROTOGEN_LIB_TARGET} PRIVATE PROTOGEN_API=__attribute__\(\(visibility\(\"default\"\)\)\))
endif()

# 添加对proto生成目标的依赖
add_dependencies(${MILLION_PROTOGEN_LIB_TARGET} million_proto_generates)

target_link_libraries(${MILLION_PROTOGEN_LIB_TARGET} PUBLIC million::third_party::protobuf)

add_library(million::protogen ALIAS ${MILLION_PROTOGEN_LIB_TARGET})