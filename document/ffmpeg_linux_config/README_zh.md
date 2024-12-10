## 安装开发工具和依赖项：打开终端，并使用以下命令安装必要的开发工具和依赖项:

- 在CentOS7/8/9和Ubuntu 22.04中配置FFmpeg环境
- 安装编译FFmpeg时所需的工具  

- centos:  
    ```sudo yum install nasm```  
    ```sudo yum update```  
    ```sudo yum groupinstall "Development Tools"```  
    ```sudo yum install git yasm cmake libtool```  
- ubuntu:  
    ```
    sudo apt-get install nasm
    sudo apt-get update 
    sudo apt-get install build-essential
    sudo apt-get install git yasm cmake libtool
    ```

- 安装其他依赖项：FFmpeg还依赖于其他一些库和软件包  
centos:  
    ```sudo yum install zlib-devel bzip2-devel openssl-devel ncurses-devel sqlite-devel readline-devel tk-devel gdbm-devel db4-devel libpcap-devel xz-devel expat-devel```  
ubuntu:  
    ```sudo apt-get install zlib1g-dev libbz2-dev libssl-dev libncurses5-dev libsqlite3-dev libreadline-dev tk-dev libgdbm-dev libdb-dev libpcap-dev liblzma-dev libexpat1-dev```

- 下载FFmpeg源代码：进入您要存储FFmpeg源代码的目录，并使用以下命令克隆FFmpeg的Git仓库  
    ```
    git clone https://github.com/FFmpeg/FFmpeg.git
    cd FFmpeg
    git checkout n5.1.4
    ./configure --enable-gpl --enable-libx264 --enable-sdl --enable-shared
    make -j8
    sudo make install
    ```


- 测试是否配置成功
```
    richard@richard-MS-7A54:$ ffmpeg -version
    ffmpeg version n5.1.4 Copyright (c) 2000-2023 the FFmpeg developers
    built with gcc 11 (Ubuntu 11.4.0-1ubuntu1~22.04)
    configuration: --enable-gpl --enable-libx264 --enable-sdl --enable-shared
    libavutil      57. 28.100 / 57. 28.100
    libavcodec     59. 37.100 / 59. 37.100
    libavformat    59. 27.100 / 59. 27.100
    libavdevice    59.  7.100 / 59.  7.100
    libavfilter     8. 44.100 /  8. 44.100
    libswscale      6.  7.100 /  6.  7.100
    libswresample   4.  7.100 /  4.  7.100
    libpostproc    56.  6.100 / 56.  6.100
```
- 以上信息表示FFmpeg安装成功

- 如果遇到编译失败，可能需要指定头文件和库路径，请添加该编译参数  
    ```-I/usr/local/include -L/usr/local/lib```  


- 彻底删除ffmpeg  
    ```sudo yum remove ffmpeg```.   
    ```sudo yum autoremove```.   
    ```rm -rf ~/.ffmpeg```.   
    ```sudo rm -rf /usr/local/include/ffmpeg```.   
    ```sudo rm -rf /usr/local/lib/libav*```.   