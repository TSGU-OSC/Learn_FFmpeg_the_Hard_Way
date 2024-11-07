# SDL2 下载

### 安装前请前往ffmpeg_CentOS7_config安装需要的软件包

## 编译、安装
```
    git clone https://github.com/libsdl-org/SDL.git
    cd SDL
    git checkout release-2.30.4
    ./configure --enable-audio --enable-alsa
    make -j8
    sudo make install
```
- 设置音频驱动：
```
    export SDL_AUDIODRIVER=alsa
```

