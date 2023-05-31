# threaded-streambuf
coutとcerrの出力を別スレッドにすることで高速化します

## usage
```c++
#include "tbuf.hpp"
#include <iostream>
int main()
{
    ThreadedBuf cout_b(std::cout.rdbuf());
    std::cout.rdbuf(&cout_b);
    std::cout << "hello, world!" << std::endl;

    // デフォルトのstreambufに戻すには
    // std::cout.rdbuf(cout_b.org_buf);
}

```

## speed test

コンソールへの出力は3〜5倍速くなるが、リダイレクトすると2倍くらい遅くなる。なぜ


ptimer → https://github.com/na-trium-144/processing-timer

```c++
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

    ThreadedBuf cout_b(std::cout.rdbuf());
    std::cout.rdbuf(&cout_b);

    // threadedbuf
    auto t2 = ptimer::ptimer([] {
        for (int i = 0; i < 10000; i++) {
            std::cout << "hello, world!" << std::endl;
        }
    });
    std::cerr << t1 << std::endl;
    std::cerr << t2 << std::endl;

}
```

```
$ ./a.out
hello, world!
hello, world!
...
hello, world!
hello, world!
12049540
2707956

$ ./a.out >/dev/null
3341786
4032742
```

