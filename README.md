# wsjpeg
![](https://img.shields.io/badge/standard-C89-brightgreen)
![](https://img.shields.io/badge/code-1%20file-brightgreen)
![](https://img.shields.io/badge/license-LGPL%20v2.1%2B-brightgreen)

## 简介

这是一个单文件的 JPEG 编码器，可以将未经压缩的 24 位 BMP 位图压缩为 JPEG 图像。  
本编码器主要用于学习了解 JPEG 的压缩原理。同时，这也是我毕业设计项目中的一部分。


## 编译

wsjpeg 代码符合 ANSI C 标准（C89/C90），因此可被近乎所有支持 C 语言的编译器编译，包括 Microsoft Visual C++ 6.0，甚至是 1988 年的 Borland Turbo C 2.0。

wsjpeg 不依赖 C 标准库外的任何第三方库。

如果您需要使用双精度浮点运算，请在头部添加 `#define USE_DOUBLE` ，或者在命令行使用 `-DUSE_DOUBLE` 编译选项（如果可用）。使用双精度浮点运算可以得到更加精确的计算结果。

编译命令行示例：
```shell
cc -O3 -DUSE_DOUBLE wsjpeg.c -o wsjpeg
```

## 命令行参数
```
wsjpeg INPUT.bmp OUTPUT.jpg [quality]
```

参数             | 说明
----------------|------------------------
`INPUT.bmp`     |   需要压缩的 BMP 文件路径
`OUTPUT.jpg`    |   输出的 JPG 文件路径
`quality`（可选）|  质量因数，可以是 0-100 之间的整数。数值越大，输出图片质量越高，同时将产生更大的文件。默认值为 75 。


## 输入输出文件规格

**输入文件：** 输入文件需为 24 位且未经压缩的 BMP 位图。单色位图、16 色位图、256 色等 BMP 位图不被支持。被 RLE 压缩的 BMP 位图亦不被支持，尽管这类格式十分少见。

**输出文件：** 输出文件为 JPEG 编码的图片文件，顺序式编码，固定使用 ISO/IEC 10918-1 : 1993(E) 中 K.3.1 给出的推荐 Huffman 表，使用规格为 4:2:0 的色度抽样 <sup>[[?]](https://zh.wikipedia.org/wiki/%E8%89%B2%E5%BA%A6%E6%8A%BD%E6%A0%B7#4:2:0)</sup>。

## 如何获得 BMP 格式的 24-bit 位图

由于 BMP 文件体积较大，我们在网络上接触到的多数是已被压缩过的 JPG 图像，或者是 PNG 图像。

可以使用以下几种方法将这些图片转换为未经压缩的 BMP 图片。

- 使用 [画图](https://support.microsoft.com/zh-cn/windows/-%E7%94%BB%E5%9B%BE-%E5%B8%AE%E5%8A%A9-d62e155a-1775-6da4-0862-62a3e9e5a511)：  
    [文件] - [保存为] - [保存类型: 24位位图 (*.bmp; *.dib)]

- 使用 [JS Paint](https://jspaint.app/)：  
    [File] - [Save As] - [Save As Type: 24-bit Bitmap (*.bmp; *.dib)]

- 使用 [ImageMagick](https://imagemagick.org/)：  
    ```shell
    convert "/path/to/image" -type truecolor "image.bmp"
    ```

- 使用 [FFmpeg](https://ffmpeg.org/)：  
    ```shell
    ffmpeg -i "/path/to/image" -pix_fmt bgr24 "image.bmp"
    ```

## 测试
**测试机型：** MacBook Air (M1, 2020)  
**编译参数：** `clang -O3 -DUSE_DOUBLE wsjpeg.c -o wsjpeg`  
**图像来源：** https://esahubble.org/images/heic1502a/
```terminal
time ./wsjpeg "Andromeda Galaxy.bmp" "Andromeda Galaxy.jpg"
        0.61 real         0.56 user         0.04 sys
```
**Andromeda Galaxy.bmp（输入）**  
种类：Windows BMP 图像  
尺寸：10000 × 3197 像素  
大小：95,910,054 字节

**Andromeda Galaxy.jpg（输出）**  
种类：JPEG 图像  
尺寸：10000 × 3197 像素  
大小：14,283,501 字节


## 已知问题或缺陷

本程序可以在绝大多数现代计算机上高效运行。但是，对于一些古老机型（如运行着 MS-DOS 系统的）或者嵌入式设备（如单片机）需要注意以下两点。

1. 此 JPEG 编码器依赖于浮点数运算，计算效率取决于 CPU 浮点数运算性能，在不含 FPU 的处理器上运行效率低下。

2. 此 JPEG 编码器会将输入文件、输出文件的所有内容缓存至 RAM，需确保 RAM 能够同时容得下输入的 BMP 文件数据和输出的 JPG 文件数据。


## 开源许可证

GNU Lesser General Public License v2.1 or later (LGPL v2.1+)
