## ffprobe usage tutorial 

### ffprobe displays video frame date

ffprobe -v error -show_entries stream=avg_frame_rate,r_frame_rate,nb_frames out.mp4

