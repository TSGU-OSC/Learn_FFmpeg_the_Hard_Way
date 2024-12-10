## ffprobe使用教程

### ffprobe显示视频帧数据

ffprobe -v error -show_entries stream=avg_frame_rate,r_frame_rate,nb_frames out.mp4
