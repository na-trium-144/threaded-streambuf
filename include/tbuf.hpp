#pragma once
#include <streambuf>
#include <ostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include <memory>
#include <unordered_map>

class ThreadedBuf : public std::streambuf {
    static constexpr int buf_size = 1024;
    char buf[buf_size];
    // bufからあふれた分を入れる
    std::string overflow_buf;

    std::ostream *target;

    // 書き込み先のostreamごとに1つスレッドを立て、管理する
    struct Writer {
        Writer(std::ostream *target)
            : target(target), t([this] { main_thread(); }) {}
        ~Writer() {
            deinit.store(true);
            cond_push.notify_all();
            t.join();
        }
        // sync_dataを読んでorg_bufに流すスレッド
        void main_thread() {
            while (true) {
                std::string data;
                {
                    std::unique_lock<std::mutex> lock(m_push);
                    cond_push.wait(lock, [this] {
                        return !sync_data.empty() || deinit.load();
                    });
                    if (sync_data.empty()) {
                        break;
                    }
                    data = sync_data.front();
                    sync_data.pop();
                }
                {
                    std::lock_guard<std::mutex> lock(m_flush);
                    *target << data << std::flush;
                }
                cond_flush.notify_all();
            }
            cond_flush.notify_all();
        };
        std::ostream *target;
        std::atomic<bool> deinit = false; // デストラクタのフラグ
        std::queue<std::string> sync_data;
        std::mutex m_push, m_flush;
        std::condition_variable cond_push, cond_flush;
        std::thread t;
        // sync_dataに追加する
        void push(const std::string &buf) {
            {
                std::lock_guard<std::mutex> lock(m_push);
                sync_data.push(buf);
            }
            cond_push.notify_all();
        }
        // flushが完了するまで待つ
        void wait() {
            std::unique_lock<std::mutex> lock(m_flush);
            cond_flush.wait(lock, [this] {
                std::lock_guard<std::mutex> lock(m_push);
                return sync_data.empty();
            });
        }
    };
    // ostreamとそれに書き込むwriterの対応を管理する
    inline static std::unordered_map<std::ostream *, std::shared_ptr<Writer>>
        writer;

    // streambufとそれに書き込むostreamの対応を管理する
    inline static std::unordered_map<std::streambuf *,
                                     std::shared_ptr<std::ostream>>
        str;

    std::string get_str() {
        return std::string(buf, this->pptr() - this->pbase());
    }
    int overflow(int c) override {
        overflow_buf += get_str();
        this->setp(buf, buf + sizeof(buf));
        this->sputc(c);
        return 0;
    }
    int sync() override {
        if (!overflow_buf.empty()) {
            writer[target]->push(overflow_buf + get_str());
            overflow_buf = "";
        } else {
            writer[target]->push(get_str());
        }
        this->setp(buf, buf + sizeof(buf));
        return 0;
    }

    // streambufの初期化とwriterの初期化
    // 同じostreamに書き込むwriterがすでにあればそれを使う
    void init() {
        this->setp(buf, buf + sizeof(buf));
        overflow_buf = "";
        if (!writer.count(target)) {
            writer.emplace(target, std::make_shared<Writer>(target));
        }
    }

  public:
    // 書き込み先のostreamを指定
    explicit ThreadedBuf(std::ostream *target) : target(target) { init(); }
    // 書き込み先のostreamを指定
    explicit ThreadedBuf(std::ostream &target) : target(&target) { init(); }
    // 書き込み先のstreambufを指定
    // そのために、sbに書き込むostreamを作成
    // 同じsbに書き込むostreamがすでに作成されていたらそれを使用
    explicit ThreadedBuf(std::streambuf *sb) {
        if (!str.count(sb)) {
            str.emplace(sb, std::make_shared<std::ostream>(sb));
        }
        target = str[sb].get();
        init();
    }

    ~ThreadedBuf() {
        // すべて出力されるまでwriterを待機
        writer[target]->wait();
    }
};

class ThreadedStream : public std::ostream {
    ThreadedBuf tbuf;

  public:
    explicit ThreadedStream(std::ostream *target)
        : tbuf(target), std::ostream(&tbuf) {}
    explicit ThreadedStream(std::ostream &target)
        : tbuf(target), std::ostream(&tbuf) {}
    explicit ThreadedStream(std::streambuf *sb)
        : tbuf(sb), std::ostream(&tbuf) {}
};
