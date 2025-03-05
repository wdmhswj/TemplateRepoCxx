# for each "test/x.cpp", generate target "x"
file(GLOB_RECURSE all_tests *.cpp)
foreach(v ${all_tests})
    string(REGEX MATCH ".*0\\.cpp$" is_ignore_file ${v})
    message(INFO ${is_ignore_file})
    if(NOT is_ignore_file)
        string(REGEX MATCH "test/.*" relative_path ${v})
        # message(${relative_path})
        string(REGEX REPLACE "test/" "" target_name ${relative_path})
        string(REGEX REPLACE ".cpp" "" target_name ${target_name})

        add_executable(${target_name} ${v})
    endif()
    
endforeach()

target_link_libraries(test_WebCrawler PRIVATE WebCrawler)
target_link_libraries(test_database PRIVATE sqlite3)
target_link_libraries(test_read_db PRIVATE sqlite3)