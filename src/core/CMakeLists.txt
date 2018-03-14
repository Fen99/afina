# build service
set(SOURCE_FILES
    ThreadPool.cpp
)

add_library(ThreadPool ${SOURCE_FILES})
target_link_libraries(ThreadPool ${CMAKE_THREAD_LIBS_INIT})
