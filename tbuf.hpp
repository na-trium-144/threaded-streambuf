#pragma once
#include <streambuf>
#include <deque>
#include <mutex>
// #include <condition_variable>
#include <thread>
#include <string>
#include <memory>
#include <unordered_map>

class ThreadedBuf : public std::streambuf
{
public:
    static constexpr int buf_size = 1024;

    ThreadedBuf(std::streambuf* sb);
    ~ThreadedBuf();

    std::streambuf* org_buf;

    // 書き込み先のsbごとに1つスレッドを立て、管理する
    struct Writer {
        Writer(std::streambuf* sb);
        ~Writer();
        void main_thread();
        std::streambuf* org_buf;
        std::mutex m;
        std::thread t;
        std::deque<std::string> sync_data;
        int sync(char* buf);  // sync_dataに追加する
        bool deinit = false;
        int ref;
    };

private:
    inline static std::unordered_map<std::streambuf*, std::shared_ptr<Writer>> writer;

    // virtual int overflow(int c);
    virtual int sync();
    char buf[buf_size];
};
