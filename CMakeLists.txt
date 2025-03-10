

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

project(testFFmpeg)

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")


set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) #相当于-fpic


set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find_package(Boost REQUIRED COMPONENTS thread chrono)
# if(Boost_FOUND)
#     include_directories(${Boost_INCLUDE_DIR})
# endif()

INCLUDE_DIRECTORIES(3rd/spdlog/)
INCLUDE_DIRECTORIES(3rd/spdlog/include)
INCLUDE_DIRECTORIES(/usr/local/mt_vaapi/include/libavformat)
INCLUDE_DIRECTORIES(/usr/local/mt_vaapi/include/libavcodec)
INCLUDE_DIRECTORIES(/usr/local/mt_vaapi/include/libavutil)
INCLUDE_DIRECTORIES(/usr/local/mt_vaapi/include/)
INCLUDE_DIRECTORIES(/usr/local/mt_vaapi/include)
INCLUDE_DIRECTORIES(/usr/local/mt_vaapi/)
INCLUDE_DIRECTORIES(/usr/local/mt_vaapi)

#INCLUDE_DIRECTORIES(/usr/include/spdlog)
INCLUDE_DIRECTORIES(/usr/include/X11)
INCLUDE_DIRECTORIES(/usr/include)

INCLUDE_DIRECTORIES(src/UI)
INCLUDE_DIRECTORIES(src)


link_directories(/usr/local/mt_vaapi/lib)
link_directories(/usr/local/mt_vaapi/lib/x86_64-linux-gnu)
#link_directories(/usr/lib/x86_64-linux-gnu)
#link_directories(/usr/local/lib/)

#add_subdirectory(3rd/spdlog)

add_executable(testFFmpeg main_d_e.cpp)
add_executable(testDecodeFFmpeg main.cpp)
add_executable(testEncodeFFmpeg main_encode.cpp)

target_link_libraries(testFFmpeg #PRIVATE
avcodec avformat avutil avdevice swscale avfilter swresample
avfilter swscale va-drm va-glx va-wayland va-x11 
dav1d
pthread
#${Boost_LIBRARIES}
#       -lspdlog      
# -lboost_system -lboost_thread -lboost_filesystem -lboost_iostreams -lboost_chrono

)

target_link_libraries(testDecodeFFmpeg #PRIVATE
avcodec avformat avutil avdevice swscale avfilter swresample
avfilter swscale va-drm va-glx va-wayland va-x11 
dav1d
pthread
#${Boost_LIBRARIES}
#       -lspdlog      
# -lboost_system -lboost_thread -lboost_filesystem -lboost_iostreams -lboost_chrono

)

target_link_libraries(testEncodeFFmpeg #PRIVATE
avcodec avformat avutil avdevice swscale avfilter swresample
avfilter swscale va-drm va-glx va-wayland va-x11 
dav1d
pthread
#${Boost_LIBRARIES}
#       -lspdlog      
# -lboost_system -lboost_thread -lboost_filesystem -lboost_iostreams -lboost_chrono

)
