# ffmpeg常用编解码器及参数设置
## 相关概念
### 转码流程图
```
 _______              ______________
|       |            |              |
| input |  demuxer   | encoded data |   decoder
| file  | ---------&gt; | packets      | -----+
|_______|            |______________|      |
                                           v
                                       _________
                                      |         |
                                      | decoded |
                                      | frames  |
                                      |_________|
 ________             ______________       |
|        |           |              |      |
| output | &lt;-------- | encoded data | &lt;----+
| file   |   muxer   | packets      |   encoder
|________|           |______________|



```
* ffmpeg calls the libavformat library (containing demuxers) to read input files and get packets containing encoded data from them. When there are multiple input files, ffmpeg tries to keep them synchronized by tracking lowest timestamp on any active input stream.

* Encoded packets are then passed to the decoder (unless streamcopy is selected for the stream, see further for a description). The decoder produces uncompressed frames (raw video/PCM audio/...) which can be processed further by filtering (see next section). After filtering, the frames are passed to the encoder, which encodes them and outputs encoded packets. Finally those are passed to the muxer, which writes the encoded packets to the output file.
### 命令示例
```
ffmpeg -i input.mp4 -c:a aac -c:v libx264 -c:s copy -b:a 320k -b:v 3000k output.mp4
```
* `ffmpeg` 指ffmpeg二进制可执行文件
* `-i` 指流stream 所有的输入流文件前都要加上 `-i` 参数
* `input.mp4` 指输入的文件 此处应替换会输入文件的实际路径
* `-c:a aac`  其中 `c` 代表 `codec` ，`a`  代表 `audio` 设置音频编码器为aac
* `-c:v libx264` 其中 `c` 代表 `codec`， `v` 代表 `video` 设置视频编码器为libx624 对应的视频编码就是 `H264` / `AVC` / `AVC1` 
* `-c:s copy` 设置字幕流进行copy 
* `-b:a 320k` 设置音频编码比特率为  320 KB/s
* `-b:v 3000k` 设置视频编码比特率为 3000 KB/s
* `output.mp4` 设置输出文件的路径及命名
### 编解码器查询方式
#### 查询全部编解码器
```
ffmpeg -codecs
```
#### 只查询全部编码器
```
ffmpeg -encoders
```
#### 只查询全部解码器
```
ffmpeg -decoders
```
### 常用参数

#### 控制视频帧速率
```
ffmpeg -i input.avi -r 24 output.mp4
```
* `-r 24` 控制帧速率为24fps
#### 控制视频分辨率

* `-vf scale=xxx:xxx` 自定义视频分辨率



#### 复制流
```
ffmpeg -i input.mp4 -c copy out.mp4
```
* `copy`参数一般在针对`codec`生效，其作用在于复制你流而跳过解码编码的过程，一般用于更改文件格式。流程图如下

```
 _______              ______________            ________
|       |            |              |          |        |
| input |  demuxer   | encoded data |  muxer   | output |
| file  | ---------  | packets      | -------  | file   |
|_______|            |______________|          |________|


```
#### 选择流
`ffmpeg` 提供了 `-map` 选项让使用者可以对流进行操作，比如视频流，音频流，字幕流等。同时可以使用`an` `vn` `sn` `dn`选项分别禁用音频流，视频流，字幕流，数据流。

#### 控制编码质量
* `-crf` 此参数用于控制编码质量，默认`-crf 23`，数值越大，文件体积越小，质量越差，数值越大则相反 
* `-b `此参数用于控制码率 如`-b:v 3000k`
* `-preset` 此参数是ffmpeg自带的预设，有slow,high等多个选项  

### map示例命令
假设有以下文件
```
input file 'A.avi'
      stream 0: video 640x360
      stream 1: audio 2 channels

input file 'B.mp4'
      stream 0: video 1920x1080
      stream 1: audio 2 channels
      stream 2: subtitles (text)
      stream 3: audio 5.1 channels
      stream 4: subtitles (text)
```
输入以下命令 输出的三个文件分别包含了哪些流？
```
ffmpeg -i A.avi -i B.mp4 out1.mkv out2.wav -map 1:a -c:a copy out3.mov
```
* `out1.mkv` 由于`mkv`全称`Matroska` 是一个可以包含视频，音频，字幕的容器，因此默认情况视频流会抓取`B.mp4`的`stream 0`，因为这个视频流有最高的分辨率，音频会抓取`B.mp4`的`stream 3`，因为这个音频流有着最多的通道数，字幕则会`B.mp4`的`stream 2`，因为这是`A.mp4`和`B.mp4`中第一个字幕流
* `out2.wav` 由于`wav`是一个只包含音频文件的容器，因此抓取两个文件中通道数最多的`B.mp4`的`stream 3`
* `out3.mov`  由于`-map 1:a -c:a copy`指定并复制了`B.mp4`中的音频流，因此包含`B.mp4`中的`stream 1` 和 `stream3`



### 关于视频无法导入Final Cut Pro等剪辑软件的解决方案

#### HEVC videos cannot be imported into Final Cut Pro
* For some strange reason HEVC with Codec ID "hev1" is not supported (at least in Big Sur).

* Fix: losslessly convert it to "hvc1":

```
ffmpeg -i input.mp4 -c copy -tag:v hvc1 output.mp4
```

* QuickTime Player refuses to play some H.265/HEVC flavors. Currently macOS 11 Big Sur is more forgiving but it still has the following issues with "Codec ID" and "Chroma subsampling" options.

* You can check all those pesky details with apps like Invisor (my favorite because it neatly highlights differences when a folder is dropped on it) or MediaInfo.

* H.265 Codec ID hvc1 plays OK (as well as H.264 Codec ID avc1).

* H.265 Codec ID hev1 has an error message "This file contains media which isn't compatible with QuickTime Player" and plays audio only. There is a lossless fix if you install ffmpeg and add '-tag:v hvc1' without re-encoding in the Terminal.

* Chroma subsampling 4:2:0 (Bit depth 8 bits) plays OK.

* Chroma subsampling 4:2:2 (Bit depth 10 bits) has an error message "This file contains media which isn't compatible with QuickTime Player". Opens anyway in Big Sur but fails in Mojave.


