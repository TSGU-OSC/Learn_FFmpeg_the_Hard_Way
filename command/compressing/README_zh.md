# 使用FFmpeg压缩视频图像或调整视频图像尺寸

## 图像

> 图像不过就是只有一帧的视频。这就是为什么大部分视频的命令在这里同样适用的原因。

* 例如，要将图像压缩到较低的分辨率，比如1080p，可以使用以下命令：

```
ffmpeg -i INPUT.jpg -filter:v scale=-2:1080 OUTPUT.jpg     
```

或者 

```
ffmpeg -i INPUT.jpg -filter:v scale=1920:1080 OUTPUT.jpg   
```

* 上述命令将会减小文件大小，但视觉质量也会降低。为了防止这种降低，可以使用FFmpeg的默认设置，无需提供任何参数:

```
ffmpeg -i INPUT.jpg  -compression_level 50 OUTPUT.jpg 
```

或者

```
ffmpeg -i INPUT.jpg  -qscale:v 2 OUTPUT.jpg 
```
