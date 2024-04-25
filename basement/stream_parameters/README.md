We'll talk about some parameters of video or audio streams

### Learn timestample the hard way

> This title is inspired by Leandro Moreira's tutorial, particularly echoing the approach of Learn FFmpeg libav the Hard Way.

#### What is pts and dts 

- PTS (Presentation Timestamp) and DTS (Decode Time Stamp) 

#### ffmpeg use threee timestamps in different bases 

- `tbn` = the time base in AVStream that has come from the container

- `tbc` = the time base in AVCodecContext for the codec used for a particular stream

- `tbr` = tbr is guessed from the video stream and is the value users want to see when they look for the video frame rate