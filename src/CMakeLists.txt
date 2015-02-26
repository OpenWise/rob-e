include_directories (${PROJECT_SOURCE_DIR}/include)

add_library( hiredis SHARED IMPORTED )
set_property (TARGET hiredis PROPERTY IMPORTED_LOCATION /usr/local/lib/libhiredis.so)

add_executable (robe robe.cpp uipc.cpp jsoncpp.cpp)
target_link_libraries (robe mraa hiredis event ${CMAKE_THREAD_LIBS_INIT})
