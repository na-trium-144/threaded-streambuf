# threaded-streambuf
* coutとcerrの出力を別スレッドにすることで高速化します
* また、複数のスレッドから書き込んでも出力は行ごと(sync()ごと)に分けられます

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

## thread safe
同じstreambufを出力先にするThreadedBufを複数作成すると、ThreadedBuf→出力先streambufへの出力が排他制御されます
```c++
#include "tbuf.hpp"
#include <iostream>
#include <thread>
#include <string>

int main()
{
    auto* cout_buf = std::cout.rdbuf();
    ThreadedBuf s1(cout_buf), s2(cout_buf), s3(cout_buf);
    std::ostream os1(&s1), os2(&s2), os3(&s3);

    auto loop = [](std::ostream* os, const std::string& str){
        for(int i = 0; i < 100; i++){
            *os << str << std::endl;
        }
    };
    std::thread t1(loop, &os1, "aaaaaa"), t2(loop, &os2, "bbbbbb"), t3(loop, &os3, "cccccc");
    t1.join();
    t2.join();
    t3.join();
}
```

```
$ a.out
aaaaaa
aaaaaa
aaaaaa
aaaaaa
cccccc
bbbbbb
bbbbbb
bbbbbb
bbbbbb
cccccc
...
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

