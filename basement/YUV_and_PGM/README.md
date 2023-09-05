# YUV and PGM

### What is YUV?

YUV is a color encoding system that represents color images or video taking human perception into account. Some key points:

* YUV separates color information into luma (brightness/contrast) and chroma (color) components.

* The Y component encodes luma information. The U and V components encode chroma information.

* Human vision is more sensitive to brightness than color, so more bandwidth can be allocated to luma.

* YUV formats can support both uncompressed and compressed formats.

* Common YUV formats include:

* * YUV444 - No chroma subsampling
* * YUV422 - Chroma samples halved horizontally
* * YUV420 - Chroma samples quartered in both dimensions
* YUV is the standard color encoding system used in video compression like MPEG and video capture.

* YUV allows bandwidth optimization for human perception needs. More luma resolution, less chroma.

* YUV colors are converted to RGB for display on screens.

So in summary, YUV is a color encoding that separates brightness from color and prioritizes bandwidth for luminance based on human perception. It is widely used in digital video applications and compression.



#### Y - Luminance

Represents brightness and luminance information
Grayscale image on its own
Encoded with highest bandwidth/resolution
#### U - Chrominance (blue projection)

Represents blue color information
With V, allows reconstructing color
Encoded at lower bandwidth than Y
#### V - Chrominance (red projection)

Represents red color information
With U, allows reconstructing color
Encoded at lower bandwidth than Y
#### To summarize:

* Y component contains brightness/luma information
* U component contains blue projection/chroma
* V component contains red projection/chroma

Y is allocated more bandwidth compared to U and V
The YUV system takes advantage of human vision's higher sensitivity to brightness vs color. This allows bandwidth optimization by encoding more luma detail in Y, and less chroma in U and V.

During conversion to RGB, the Y, U and V components are combined to reproduce the original color image. But encoding them separately in YUV allows more efficient compression.


### PGM

`PGM` stands for "Portable Graymap" and is a grayscale image file format.

#### Some key properties of the PGM image format:

* Uses plain ASCII text header
* Can represent grayscale images with arbitrary precision
* Supports binary or ASCII encoding of pixel data
Lossless format, no compression
* Common in academic research and image processing
* Supported by many image manipulation programs
#### The PGM file structure consists of:

* A header with format info (P5 for binary format)
* Width and height of image
* Maximum gray value (typically 255)
* Pixel data in binary or ASCII format

PGM is popular in contexts where a simple, flexible grayscale format is needed, like scientific imaging and machine learning datasets.

It offers a compact lossless representation of grayscale images while being portable across platforms and applications, unlike proprietary formats. Many image processing libraries like OpenCV can load and write PGM too.

So in summary, PGM is a flexible open standard format tailored to monochrome image data, widely used in research and academic contexts.

### What is the difference of YUV and PGM

The main differences between YUV and PGM image formats are:

#### Color vs grayscale:

* YUV encodes color images, representing luminance and chrominance components
* PGM is grayscale, containing only luminance/intensity values
#### Pixel encoding:

* YUV stores color information encoded into separate components
* PGM directly stores grayscale intensity values as pixels
#### Compression:

* YUV supports both uncompressed and compressed variants like YUV420
* PGM is always uncompressed
#### Use cases:

* YUV is used widely in video processing and compression
* PGM is common in academic image processing and machine learning
#### Components:

* YUV has 3 components (Y, U, V) representing luminance, chrominance
* PGM has only a single grayscale intensity component

So in summary, the main differences are:

* YUV represents color, PGM is grayscale only
* YUV encodes color in components, PGM has direct pixels
* YUV has compressed formats, PGM is uncompressed
* YUV used for video, PGM for research
* YUV has 3 components, PGM has 1
YUV and PGM serve different purposes, one for color video and one for grayscale images. But both contain luminance/brightness information.