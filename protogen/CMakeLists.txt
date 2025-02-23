if (WIN32)
    set(PROTOGEN_COMPILE_DEFINITIONS PROTOGEN_API=__declspec\(dllimport\) CACHE STRING "protobuf compile definitions")
elseif(UNIX)
    set(PROTOGEN_COMPILE_DEFINITIONS PROTOGEN_API=__attribute__\(\(visibility\(\"hidden\"\)\)\) CACHE STRING "protobuf compile definitions")
endif()
set(PROTOGEN_HEADER_DIR 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    CACHE STRING "protobuf header dir")
set(PROTOGEN_LIB protogen CACHE STRING "protobuf lib")

if(WIN32)
    file(GLOB_RECURSE PRIVATE_HEADERS 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.hpp)
    source_group("Private Header Files" FILES ${PUBLIC_HEADERS})
    file(GLOB_RECURSE PUBLIC_HEADERS 
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.hpp)
    source_group("Public Header Files" FILES ${PUBLIC_HEADERS})
endif()
file(GLOB SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cc)
add_library(${PROTOGEN_LIB} SHARED ${SRC} ${PRIVATE_HEADERS} ${PUBLIC_HEADERS})

target_compile_definitions(${PROTOGEN_LIB} PUBLIC ${PROTOGEN_COMPILE_DEFINITIONS})
target_include_directories(${PROTOGEN_LIB} PUBLIC ${PROTOGEN_HEADER_DIR})
target_include_directories(${PROTOGEN_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (WIN32)
    target_compile_definitions(${PROTOGEN_LIB} PRIVATE PROTOGEN_API=__declspec\(dllexport\))
elseif(UNIX)
    target_compile_definitions(${PROTOGEN_LIB} PRIVATE PROTOGEN_API=__attribute__\(\(visibility\(\"default\"\)\)\))
 endif()

target_compile_definitions(${PROTOGEN_LIB} PRIVATE PROTOBUF_USE_DLLS)
target_include_directories(${PROTOGEN_LIB} PRIVATE ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(${PROTOGEN_LIB} PRIVATE ${PROTOBUF_LIB})