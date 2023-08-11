#include "tbuf.hpp"
#include <iostream>
#include "ptimer.hpp"

int main()
{
    // default streambuf
    auto t1 = ptimer::ptimer([] {
        for (int i = 0; i < 10000; i++) {
            std::cout << "hello, world!" << std::endl;
        }
    });

    auto org = std::cout.rdbuf();
    long t2;
    {
        ThreadedBuf cout_b(std::cout.rdbuf());
        std::cout.rdbuf(&cout_b);

        // threadedbuf
        t2 = ptimer::ptimer([] {
            for (int i = 0; i < 10000; i++) {
                std::cout << "hello, world!" << std::endl;
            }
        });

        // flush cout_b
    }
    std::cout.rdbuf(org);

    std::cerr << t1 << std::endl;
    std::cerr << t2 << std::endl;

}