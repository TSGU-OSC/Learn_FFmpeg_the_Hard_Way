# ffmpeg common commands

Batch processing (e.g. Video to audio operating environment in the same folder zsh on Unix)

```
for f in *.mp4; do
  ffmpeg -i "$f" -vn -acodec copy "${f%.*}.m4a"
done
```
This will loop through all .mp4 videos in the current directory, and for each one generate an .m4a audio file with the same basename.

The key parts:

* `for f in *.mp4` - Loops through all .mp4 files
* `-i "$f"` - Input file is the current video
* `-vn` - Disables video stream copying, so only audio is output
* `-acodec copy` - Stream copy the audio without re-encoding
* `"${f%.*}.m4a"` - Output to an .m4a file with the same name