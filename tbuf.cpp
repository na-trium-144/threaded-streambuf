#include "tbuf.hpp"
#include <cstddef>
#include <cstring>

ThreadedBuf::ThreadedBuf(std::streambuf* sb) : org_buf(sb)
{
    setp(buf, buf + sizeof(buf));
    if (!writer.count(org_buf)) {
        writer.emplace(org_buf, std::make_shared<Writer>(org_buf));
    } else {
        std::lock_guard lock(writer[org_buf]->m);
        writer[org_buf]->ref++;
    }
}
ThreadedBuf::~ThreadedBuf()
{
    int ref;
    {
        std::lock_guard lock(writer[org_buf]->m);
        ref = --writer[org_buf]->ref;
    }
    if (ref <= 0) {
        writer.erase(org_buf);
    }
}
ThreadedBuf::Writer::Writer(std::streambuf* sb) : org_buf(sb), t([this] { main_thread(); }), ref(1)
{
}
ThreadedBuf::Writer::~Writer()
{
    deinit = true;
    t.join();
}
void ThreadedBuf::Writer::main_thread()
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
            for (std::size_t i = 0; i < line.size(); i++) {
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
    int s = writer[org_buf]->sync(buf);
    std::memset(buf, 0, sizeof(buf));
    setp(buf, buf + sizeof(buf));
    return s;
}
int ThreadedBuf::Writer::sync(char* buf)
{
    std::lock_guard lock(m);
    sync_data.push_back(std::string(buf));
    return 0;
}
