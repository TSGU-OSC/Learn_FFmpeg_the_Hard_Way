# different types of frames serve specific roles in efficiently representing and transmitting video content

## I-Frame (Intra-Frame):

- Description: I-frames are standalone frames that `do not rely on other frames for decoding`. They are complete images with no dependency on previous or subsequent frames.
- Purpose: I-frames act as reference points for the decoding process. They provide a starting point for the decoding of subsequent frames.
## P-Frame (Predictive Frame):

- Description: P-frames are predicted frames that rely on previous I-frames or P-frames for decoding. They `contain only the changes (motion information)` from the reference frames.
- Purpose: P-frames help reduce redundancy by transmitting only the differences between the current frame and the reference frame, improving compression efficiency.
## B-Frame (Bidirectional Frame):

- Description: B-frames `use both preceding and subsequent frames` for motion compensation. They contain the most compressed information, representing the difference between the current frame and the frames before and after it.
- Purpose: B-frames further enhance compression efficiency by considering motion in both directions, but they may introduce more decoding complexity.