# Compress/Rescale Videos/Images using FFmpeg

## Images

> Images are nothing but videos with just one frame. This is the reason why most of the video commands are applicable here as well.

* For example, to compress an image to a lower resolution, say 1080p, use the following command:

```
ffmpeg -i INPUT.jpg -filter:v scale=-2:1080 OUTPUT.jpg     
```

or 

```
ffmpeg -i INPUT.jpg -filter:v scale=1920:1080 OUTPUT.jpg   
```

* The above command will reduce the file size, but the visual quality will also be reduced. To prevent such reduction, use the FFmpeg’s default settings i.e. no need to supply any flags:

```
ffmpeg -i INPUT.jpg  -compression_level 50 OUTPUT.jpg 
```

or

```
ffmpeg -i INPUT.jpg  -qscale:v 2 OUTPUT.jpg 
```