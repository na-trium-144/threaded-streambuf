#include "tbuf.hpp"
#include <iostream>
#include <thread>
#include <string>

int main() {
    auto loop = [](const std::string &str) {
        ThreadedStream os(std::cout);
        for (int i = 0; i < 100; i++) {
            os << str << std::endl;
        }
    };
    std::thread t1(loop, "aaaaaa"), t2(loop, "bbbbbb"), t3(loop, "cccccc");
    t1.join();
    t2.join();
    t3.join();
}