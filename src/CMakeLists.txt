# for each "src/x.cpp", generate target "x"
# file(GLOB_RECURSE all_srcs CONFIGURE_DEPENDS *.cpp)

# 使用 file(GLOB) 仅获取 src 目录下的 .cpp 文件
file(GLOB all_srcs CONFIGURE_DEPENDS *.cpp)

foreach(v ${all_srcs})
    string(REGEX MATCH "src/.*" relative_path ${v})
    # message(${relative_path})
    string(REGEX REPLACE "src/" "" target_name ${relative_path})
    string(REGEX REPLACE ".cpp" "" target_name ${target_name})

    add_executable(${target_name} ${v})
endforeach()

find_package(OpenSSL REQUIRED)

if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

# 在你的目标定义中链接 OpenSSL
target_link_libraries(server ${OPENSSL_LIBRARIES})

add_library(WebCrawler SHARED WebCrawler/WebCrawler.cpp)
target_link_libraries(WebCrawler ${OPENSSL_LIBRARIES})