#include "tbuf.hpp"
#include <cstddef>
#include <cstring>
#include <iostream>
ThreadedBuf::ThreadedBuf(std::streambuf* sb) : org_buf(sb), t([this] { main_thread(); })
{
    setp(buf, buf + sizeof(buf));
}

ThreadedBuf::~ThreadedBuf()
{
    deinit = true;
    t.join();
}
void ThreadedBuf::main_thread()
{
    while (true) {
        std::this_thread::yield();
        bool out = false;
        while (true) {
            std::string line;
            {
                std::lock_guard lock(m);
                if (sync_data.size() == 0) {
                    break;
                }
                line = sync_data[0];
                sync_data.pop_front();
            }
            for (int i = 0; i < line.size(); i++) {
                org_buf->sputc(line[i]);
            }
            out = true;
        }
        if (out) {
            org_buf->pubsync();
        }
        if (deinit) {
            break;
        }
    }
}

int ThreadedBuf::sync()
{
    {
        std::lock_guard lock(m);
        sync_data.push_back(std::string(buf));
    }
    std::memset(buf, 0, sizeof(buf));
    setp(buf, buf + sizeof(buf));
    return 0;
}
