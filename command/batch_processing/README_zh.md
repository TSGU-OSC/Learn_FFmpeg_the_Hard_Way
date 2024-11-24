# ffmpeg 常用命令

批处理（e.g. 同一文件夹下视频转音频 操作环境 zsh on Unix）

```
for f in *.mp4; do
  ffmpeg -i "$f" -vn -acodec copy "${f%.*}.m4a"
done
```
这将循环遍历当前目录中的所有.mp4视频，并为每个视频生成具有相同基名的.m4a音频文件。

关键部分：

* `for f in *.mp4` - 循环遍历所有.mp4文件
* `-i "$f"` - 输入文件为当前视频
* `-vn` - 禁用视频流复制，因此只输出音频
* `-acodec copy` - 流复制音频而不重新编码
* `"${f%.*}.m4a"` - 输出到具有相同名称的.m4a文件