# SDL2 下载
- 官网地址：Simple DirectMedia Layer - Homepage  
- 源码下载地址（SDL version 2.0.14）：https://www.libsdl.org/release/SDL2-2.0.14.tar.gz  

## 编译、安装
```
    tar -zvxf SDL2-2.0.14.tar.gz
    cd SDL2-2.0.14/
    ./configure --prefix=$PWD/_instal
    make && make install
    cd _install/
```
- 这是系统提示的缺少的软件包:
```
    sudo apt install libsdl2-dev 
```

## 设置环境变量

- 在SDL2-2.0.14/_install目录下创建一个.sh文件，比如```sdl.sh```，并在sdl.sh文件中写入以下内容：
- (注)在SDL2-2.0.14目录下的Makfile中加入以下的，并且export前面的不是空格，是tab键
- 下面代码中的```richard/Downloads```需要改为自己的安装路径
```
#!/bin/bash

        export PATH=$PATH:/home/richard/Downloads/SDL2/SDL2-2.0.14/_install/lib
        export PATH=$PATH:/home/richard/Downloads/SDL2/SDL2-2.0.14/_install/bin
        export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/home/richard/Downloads/SDL2/SDL2-2.0.14/_install/lib/pkgconfig"   
```
- 然后执行以下命令：  
```
        chmod +x sdl.sh
        ./sdl.sh
        source /path/to/sdl.sh
```
- 这样每次启动终端时，脚本自动运行，并将环境变量设置到终端会话中


## Makefile创建软链接的命令
- 创建一个Makefile文件，并将以下内容写入文件中,然后执行命令```make```
```
.PHONY:all link unlink mk_cmake_dir link_bin link_pc link_m4 link_cmake
 
SRCDIRS         = $(PWD)
DSTDIRS         = /usr
MKDIR           = mkdir -p
RM                      = rm -rf
 
FILE_BIN        = sdl2-config
FILE_M4         = sdl2.m4
FILE_PC         = sdl2.pc
DIR_CMAKE       = lib/cmake/SDL2
 
 
all:link link_bin link_pc link_m4 link_cmake
 
mk_cmake_dir:
        sudo $(MKDIR) /usr/lib/cmake

link:
        @echo ">>>>make links for the files of include && lib. <<<<"
        sudo ln -s $(SRCDIRS)/include/SDL2/ $(DSTDIRS)/include/SDL2
        sudo ln -s $(SRCDIRS)/lib/libSDL2-2.0.so.0.14.0 $(DSTDIRS)/lib/libSDL2-2.0.so.0
        sudo ln -s $(SRCDIRS)/lib/libSDL2-2.0.so.0.14.0 $(DSTDIRS)/lib/libSDL2-2.0.so.0.14.0
        sudo ln -s $(SRCDIRS)/lib/libSDL2.a $(DSTDIRS)/lib/libSDL2.a
        sudo ln -s $(SRCDIRS)/lib/libSDL2.la $(DSTDIRS)/lib/libSDL2.la
        sudo ln -s $(SRCDIRS)/lib/libSDL2main.a $(DSTDIRS)/lib/libSDL2main.a
        sudo ln -s $(SRCDIRS)/lib/libSDL2main.la $(DSTDIRS)/lib/libSDL2main.la
        sudo ln -s $(SRCDIRS)/lib/libSDL2-2.0.so.0.14.0 $(DSTDIRS)/lib/libSDL2.so
        sudo ln -s $(SRCDIRS)/lib/libSDL2_test.a $(DSTDIRS)/lib/libSDL2_test.a
        sudo ln -s $(SRCDIRS)/lib/libSDL2_test.la $(DSTDIRS)/lib/libSDL2_test.la
 
link_bin:
        @echo ">>>>make links for the files of bin. <<<<"
        sudo ln -s $(SRCDIRS)/bin/sdl2-config $(DSTDIRS)/bin/sdl2-config
 
link_pc:
        @echo ">>>>make links for the files of pc. <<<<"
        sudo ln -s $(SRCDIRS)/lib/pkgconfig/sdl2.pc $(DSTDIRS)/lib/pkgconfig/sdl2.pc
 
link_m4:
        @echo ">>>>make links for the files of m4. <<<<"
        sudo ln -s $(SRCDIRS)/share/aclocal/sdl2.m4 $(DSTDIRS)/share/aclocal/sdl2.m4
 
link_cmake: mk_cmake_dir
        @echo ">>>>make links for the files of cmake. <<<<"
        sudo ln -s $(SRCDIRS)/lib/cmake/SDL2 $(DSTDIRS)/lib/cmake/SDL2
 
unlink:
        sudo $(RM) $(DSTDIRS)/include/SDL2
        sudo $(RM) $(DSTDIRS)/lib/libSDL2*
        sudo $(RM) $(DSTDIRS)/bin/sdl2-config
        sudo $(RM) $(DSTDIRS)/lib/pkgconfig/sdl2.pc
        sudo $(RM) $(DSTDIRS)/share/aclocal/sdl2.m4
        sudo $(RM) $(DSTDIRS)/lib/cmake/SDL2
```
- 请注意上面的代码文件中的是tab键而不是空格，否则会报错
- 如果提示链接已经存在，先执行make unlink，然后执行make



## SDL测试
- 与代码文件同一个目录下创建一个Makefile，把下面的代码加入进去，需要把INCDIRS和LIBDIRS后面的路径改为你当前的工作路径，测试代码文件为hello_sdl.c
- 使用```make```命令可以直接生成可执行的二进制文件，执行这个二进制文件时需要在有图形化界面中执行，否则会报错，注意(以下该测试仅用于测试环境是否搭建成功)
```
CC      = gcc
C_SRC   = ${wildcard *.c}
BIN = ${patsubst %.c, %, $(C_SRC)}
 
INCDIRS :=
LIBDIRS :=
 
INCDIRS += /home/richard/Downloads/SDL2/SDL2-2.0.14/_install/include/SDL2
LIBDIRS += /home/richard/Downloads/SDL2/SDL2-2.0.14/_install/lib
 
CFLAGS  = -g -Wall -I$(INCDIRS) -L$(LIBDIRS) -lSDL2 -lpthread -lm -ldl
 
all:$(BIN)
 
$(BIN):%:%.c
        $(CC) -o $@ $^ $(CFLAGS)
 
clean:
        $(RM) a.out $(BIN)
 
.PHONY: all clean
```

