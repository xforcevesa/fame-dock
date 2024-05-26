add_executable(create_table_test create_table_test.cpp)
#add_library(test STATIC SOURCES)
target_link_libraries(create_table_test system gtest_main record storage )
