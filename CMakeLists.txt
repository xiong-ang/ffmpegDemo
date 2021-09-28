cmake_minimum_required(VERSION 3.0.0)

set(VCPKG_ROOT "D:/4_Tools/x64-windows/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)
project(rtspTest VERSION 0.1.0)

add_executable(${PROJECT_NAME} 
    ./rtsp_video_capture/video_capture.h
    ./rtsp_video_capture/video_capture.cpp
    ./rtsp_video_record/video_recoder.h
    ./rtsp_video_record/video_recoder.cpp
    main.cpp
)

# ffmpeg
find_package(FFMPEG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE  ${FFMPEG_LIBRARIES})

# opencv
find_package(OpenCV CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE opencv_core opencv_imgproc opencv_imgcodecs opencv_dnn opencv_cudaarithm opencv_cudawarping opencv_cudaimgproc opencv_highgui opencv_videoio)