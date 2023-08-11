#include "tbuf.hpp"
#include <iostream>
int main()
{
    auto org = std::cout.rdbuf();

    ThreadedBuf cout_tb(std::cout.rdbuf());
    std::cout.rdbuf(&cout_tb);
    std::cout << "hello, world!" << std::endl;

    // デフォルトのstreambufに戻すには
    std::cout.rdbuf(org);
}
