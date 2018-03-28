# build service
set(SOURCE_FILES
    ThreadPool.cpp
    FileDescriptor.cpp
)

add_library(Core ${SOURCE_FILES})
target_link_libraries(Core ${CMAKE_THREAD_LIBS_INIT})
