## 安装开发工具和依赖项：打开终端，并使用以下命令安装必要的开发工具和依赖项:

- 在CentOS7中配置ffmpeg环境
- 安装编译FFmpeg时所需的工具  
    ```sudo yum install nasm```  
    ```sudo yum update```  
    ```sudo yum groupinstall "Development Tools"```  
    ```sudo yum install git yasm cmake libtool```

- 安装其他依赖项：FFmpeg还依赖于其他一些库和软件包
    ```sudo yum install zlib-devel bzip2-devel openssl-devel ncurses-devel sqlite-devel readline-devel tk-devel gdbm-devel db4-devel libpcap-devel xz-devel expat-devel```

- 下载FFmpeg源代码：进入您要存储FFmpeg源代码的目录，并使用以下命令克隆FFmpeg的Git仓库
    ```wget https://ffmpeg.org/releases/ffmpeg-4.0.tar.gz```  
    ```tar -xf ffmpeg-4.0.tar.gz```  
    ```cd ffmpeg-4.0```

- 编译和安装FFmpeg：进入FFmpeg源代码目录，并执行以下命令来编译和安装FFmpeg
    ```./configure --enable-shared```.   
    ```如果依赖h264的话用下面这条命令，前提安装有h264```.   
    ```./configure --enable-gpl --enable-libx264 --enable-shared --extra-ldflags=-L/usr/local/lib --extra-cflags=-I/usr/local/include```. 
    ```make```. 
    ```sudo make install```. 

- 配置开发环境：打开您使用的文本编辑器，创建一个新的文件，例如ffmpeg-dev.sh，并将以下内容添加到文件中
    ```export LD_LIBRARY_PATH=/usr/local/lib```
    ```export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig```

    - 然后执行
    ```source ffmpeg-dev.sh```
    - 没有反馈则说明成功

- 配置动态库路径：
    - 将FFmpeg的动态库路径/usr/local/lib添加到/etc/ld.so.conf文件中
        ```sudo echo "/usr/local/lib" >> /etc/ld.so.conf```  
        ```sudo ldconfig```

- 测试是否配置成功
    ```echo $LD_LIBRARY_PATH```  
    ```echo $PKG_CONFIG_PATH```
    - 如果输出了显示正确的路径，则说明环境变量已正确设置
    - 使用以下命令：检测按照和配置
    ```pkg-config --modversion libavcodec```
    - 如果输出显示了正确的版本号。说明ffmpeg库的安装和配置是成功的

- 彻底删除ffmpeg
    ```sudo yum remove ffmpeg```  
    ```sudo yum autoremove```  
    ```rm -rf ~/.ffmpeg```  
    ```sudo rm -rf /usr/local/include/ffmpeg```  
    ```sudo rm -rf /usr/local/lib/libav*```  