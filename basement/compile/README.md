# 编译项目

### 示例
```
gcc -I/path/to/include -L/path/to/lib -o test test.c -libavutil -libavcodec -libavformat -lz -lm
```

* `gcc`是编译器 
* 带`-`的都属于gcc程序的运行参数
* `-I`后跟ffmpeg的inlcude目录
* `-L`后跟ffmpeg的lib目录
* `-o`后跟输出可执行文件的命名，如`-o test`，编译后的的可执行文件名字就是test
* `-lz` zlib压缩库 
* `-lm` math数学库
* `-libavutil -libavcodec -libavformat`链接ffmpeg的一些库