### Here is how to calculate the uncompressed size of a video with a given resolution, frame rate, bit depth, and duration:

* Get the resolution: 1920 x 1080 (width x height)
* Calculate number of pixels per frame: Pixels = Width x Height Pixels = 1920 x 1080 = 2,073,600 pixels
* Get bit depth per pixel (commonly 8, 10, or 12 bits). Let's assume 8 bit.
* Calculate bits per pixel: Bits per pixel = Bit depth / 8 For 8 bit depth: Bits per pixel = 8 / 8 = 1 byte
* Get the frame rate: 30 FPS
* Calculate number of frames: Frames = Duration * Frame rate For a 1 minute video (60 seconds) at 30 FPS: Frames = 60 x 30 = 1800 frames
* Calculate total size: Total size = Pixels x Bits per pixel x Frames = 2,073,600 x 1 byte x 1800 frames = ~3,732 megabytes
#### So for a 1920x1080 30FPS 8-bit 1 minute video, the uncompressed size is approximately 3.7 GB.

#### You can adjust the parameters like bit depth, duration, resolution etc and calculate the size accordingly. Let me know if you need any clarification!