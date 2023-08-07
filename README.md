# threaded-streambuf
* coutとcerrの出力を別スレッドにすることで高速化します
* また、複数のスレッドから書き込んでも出力は行ごと(sync()ごと)に分けられます
    * stream自体はスレッドセーフではないですが別スレッドで出力先に書き込む処理がスレッドセーフ

## usage
```c++
// コンストラクタで渡したtargetに別スレッドでデータを送るstreambuf
class ThreadedBuf : public std::streambuf{
    explicit ThreadedBuf(std::ostream *target);
    explicit ThreadedBuf(std::ostream &target);
    explicit ThreadedBuf(std::streambuf *sb);
}

// コンストラクタで渡したtargetに別スレッドでデータを送るostream
class ThreadedStream : public std::ostream{
    explicit ThreadedStream(std::ostream *target);
    explicit ThreadedStream(std::ostream &target);
    explicit ThreadedStream(std::streambuf *sb);
}
```

## example
### coutのrdbufを置き換える例
```c++
#include "tbuf.hpp"
#include <iostream>
int main()
{
    auto org = std::cout.rdbuf();
    
    ThreadedBuf cout_tb(std::cout.rdbuf());
    std::cout.rdbuf(&cout_tb);
    std::cout << "hello, world!" << std::endl;

    // デフォルトのstreambufに戻す
    std::cout.rdbuf(org);
}
```
* 上の例で `ThreadedBuf cout_tb(std::cout);`とすると、 std::cout -> cout_tb -> std::cout とデータが送られて無限ループしてしまう

### 新しいstreamを作る例
* 複数のThreadedStreamやThreadedBufに同じ出力先を指定すると、内部でそこに書き込む処理をするスレッドが共有され排他制御される
* よって以下のように別々のスレッドで書き込んでも出力が混ざらない
```c++
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

