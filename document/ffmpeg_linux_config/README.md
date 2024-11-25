# Configure the FFmpeg environment in CentOS 7/8/9 and Ubuntu 22.04

- Install the tools required for compiling FFmpeg

- For CentOS: 
    ```sudo yum install nasm```  
    ```sudo yum update```  
    ```sudo yum groupinstall "Development Tools"```  
    ```sudo yum install git yasm cmake libtool```  

- For Ubuntu:  
    ```
    sudo apt-get install nasm
    sudo apt-get update 
    sudo apt-get install build-essential
    sudo apt-get install git yasm cmake libtool
    ```

- Install other dependencies: FFmpeg also depends on several other libraries and packages. 

- CentOS:  
    ```sudo yum install zlib-devel bzip2-devel openssl-devel ncurses-devel sqlite-devel readline-devel tk-devel gdbm-devel db4-devel libpcap-devel xz-devel expat-devel```  
- Ubuntu:  
    ```sudo apt-get install zlib1g-dev libbz2-dev libssl-dev libncurses5-dev libsqlite3-dev libreadline-dev tk-dev libgdbm-dev libdb-dev libpcap-dev liblzma-dev libexpat1-dev```

- Download FFmpeg Source Code: Navigate to the directory where you want to store the FFmpeg source code and use the following command to clone the FFmpeg Git repository: 
    ```
    git clone https://github.com/FFmpeg/FFmpeg.git
    cd FFmpeg
    git checkout n5.1.4
    ./configure --enable-gpl --enable-libx264 --enable-sdl --enable-shared
    make -j8
    sudo make install
    ```

- Test for Successful Configuration:
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
- The above information indicates that FFmpeg has been successfully installed.

- If you encounter compilation failures, you may need to specify the header file and library paths; please add the following compile parameters:

```
    -I/usr/local/include -L/usr/local/lib
```  


- Completely remove FFmpeg:
    ```sudo yum remove ffmpeg```.   
    ```sudo yum autoremove```.   
    ```rm -rf ~/.ffmpeg```.   
    ```sudo rm -rf /usr/local/include/ffmpeg```.   
    ```sudo rm -rf /usr/local/lib/libav*```.   