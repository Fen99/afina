# build service
set(SOURCE_FILES          
    core/FIFO.cpp
    FIFOServer.cpp
   )

add_library(FIFO ${SOURCE_FILES})
target_link_libraries(FIFO ${CMAKE_THREAD_LIBS_INIT})

