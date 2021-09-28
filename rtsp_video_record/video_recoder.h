#pragma once
#include <string>
#include <memory>
#include <future>

class VideoRecoder
{
public:
    VideoRecoder(const std::string &rtsp_url, const std::string &file_name)
        : _rtsp_url(rtsp_url), _file_name(file_name), _recording(false)
    {
    }

    bool Start();
    bool Stop();

private:
    bool StartHandle();

    mutable bool _recording{false};
    std::future<bool> _handler_future{};

    std::string _rtsp_url{};
    std::string _file_name{};
};