# Use of the Filter

### Generate a two - dimensional code

 Take generating the official website of ffmpeg as an example
```bash
ffmpeg -f lavfi -i nullsrc=s=100x100,coreimage=filter=CIQRCodeGenerator@inputMessage=https\\\\\://FFmpeg.org/@inputCorrectionLevel=H -frames:v 1 QRCode.png
```

### Video splicing

Stitch two horizontal videos into one, with the sum of their widths and the same height
```bash
ffmpeg -i video1.mp4 -i video2_scaled.mp4 -filter_complex "hstack" output.mp4
```