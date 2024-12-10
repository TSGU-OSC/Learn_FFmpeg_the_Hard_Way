## x264 Installation

- Source Compilation:
```
    git clone https://code.videolan.org/videolan/x264.git
    cd x264
    ./configure --prefix=/usr/x264/ --includedir=/usr/local/include --libdir=/usr/local/lib --enable-shared
    make -j16
    sudo make install

```

- Configure Variables：
```
    vim ~/.bashrc
```
    
- Add the environment variables at the end of the file:
```
    export PATH="/usr/local/nasm/bin:$PATH"
    export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
```

- Activate the settings:
```
    source ~/.bashrc
```

- Verify Installation:
```
    richard@richard-MS-7A54:$ pkg-config --libs x264
    -L/usr/local/lib -lx264
```

- richard@richard-MS-7A54:~$ ls /usr/local/include/
```
    x264_config.h  x264.h
```