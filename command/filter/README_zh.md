# 滤镜的使用

## 查询滤镜信息

``ffmpeg -filters`` 查看所有可用的filter， 如果想要查看某个具体滤镜的参数信息，使用以下命令
```Shell
ffmpeg -h filter=<name of filter>
```


### 生成一个二维码

示例：生成 FFmpeg 官网链接的二维码
```bash
ffmpeg -f lavfi -i nullsrc=s=100x100,coreimage=filter=CIQRCodeGenerator@inputMessage=https\\\\\://FFmpeg.org/@inputCorrectionLevel=H -frames:v 1 QRCode.png
```

### 视频拼接

将两个横屏视频拼接为一个，宽度相加，高度保持一致
```bash
ffmpeg -i video1.mp4 -i video2_scaled.mp4 -filter_complex "hstack" output.mp4
```