#pragma once
#include <streambuf>
#include <deque>
#include <mutex>
// #include <condition_variable>
#include <thread>
#include <string>

class ThreadedBuf : public std::streambuf
{
public:
    static constexpr int buf_size = 1024;

    ThreadedBuf(std::streambuf* sb);
    ~ThreadedBuf();

    std::streambuf* org_buf;

private:
    void main_thread();
    // virtual int overflow(int c);
    virtual int sync();
    char buf[buf_size];
    std::mutex m;
    // std::condition_variable cv;
    std::thread t;
    std::deque<std::string> sync_data;
    bool deinit = false;
};
