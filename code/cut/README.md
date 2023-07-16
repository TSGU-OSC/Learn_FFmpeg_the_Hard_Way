# ffmpeg常用命令

音视频剪切命令
```
ffmpeg -i input.mp3 -ss 00:00:00 -to 00:00:30 -c copy output.mp3
```

This will take an input audio file called input.mp3, trim it to only 30 seconds long, starting from the beginning, and save it to output.mp3.

The key options are:

* `-ss 00:00:00` - Seek to the start time, in this case the beginning
* `-to 00:00:30` - Set the duration or end time to 30 seconds
* `-c copy` - Stream copy without re-encoding, for faster processing
You can adjust the start and duration times as needed. Some other useful variations:

Trim from 30 seconds to 1 minute:
```
-ss 00:00:30 -to 00:01:00
```

Trim the last 30 seconds:
```
-ss -00:00:30
```
