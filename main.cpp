#include <iostream>

int test_VideoRecoder();
int test_VideoCapture();

int main()
{
    //test_VideoCapture();
    test_VideoRecoder();
}

#include "rtsp_video_record/video_recoder.h"
int test_VideoRecoder()
{
    VideoRecoder recode("rtsp://username:password*.*.*.*:554/Streaming/Channels/1", "test.mp4");
    recode.Start();
    std::this_thread::sleep_for(std::chrono::seconds(15));
    if (!recode.Stop())
    {
        std::cout << "recode failed" << std::endl;
    }
    return 0;
}

#include "rtsp_video_capture/video_capture.h"
int test_VideoCapture()
{
    VideoCapture cap("rtsp://username:password*.*.*.*:554/Streaming/Channels/1");
    if (!cap.isOpened())
    {
        std::cout << "Open capture failed";
        return 0;
    }

    int count = 1000;
    while (count-- > 0)
    {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty())
            break;
        std::cout << frame.cols << "  " << frame.rows << std::endl;
    }
    return true;
}