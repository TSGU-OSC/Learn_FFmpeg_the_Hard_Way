# 滤镜的使用

### 生成一个二维码

 生成二维码————以生成ffmpeg官网为例
```bash
ffmpeg -f lavfi -i nullsrc=s=100x100,coreimage=filter=CIQRCodeGenerator@inputMessage=https\\\\\://FFmpeg.org/@inputCorrectionLevel=H -frames:v 1 QRCode.png
```

### 视频拼接

将两个横屏视频拼接为一个，宽度相加，高度保持一致
```bash
ffmpeg -i video1.mp4 -i video2_scaled.mp4 -filter_complex "hstack" output.mp4
```