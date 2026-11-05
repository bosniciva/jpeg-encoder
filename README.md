# JPEG Encoder Implementation

A performance-focused **JPEG Encoder** designed to compress uncompressed digital images (such as BMP) into standard JFIF/JPEG-compliant formats. This project implements low-level digital image processing algorithms, matrix transformations, and specific block-interleaving patterns required by the JPEG standard.

## Features & Pipeline Implementation

The encoder processes raw images through a strict digital signal processing pipeline:
*   **Color Space Conversion & Padding:** Transforms input data from RGB to YUV420 chroma subsampling. Dynamically handles image padding if dimensions are not divisible by 16.
*   **Level Shifting:** Performs unsigned-to-signed range adjustment by applying a -128 offset to the Luminance (Y) channel components.
*   **2D Forward Discrete Cosine Transform (DCT):** Splits the image into $8 \times 8$ blocks and applies 2D DCT transformations to decouple high and low-frequency spatial data.
*   **Dynamic Quantization Matrix Scaling:** Utilizes a custom `quantQuality()` function to dynamically scale quantization coefficients based on user-defined quality parameters.
*   **Zig-Zag Serialization:** flattens 2D frequency matrices into 1D arrays using a standard Zig-Zag scanning pattern optimized for entropy coding.
*   **Chroma Subsampling Block Interleaving:** Strictly schedules blocks in memory according to the $16 \times 16$ macroblock standard (4 sequential Y blocks followed by 1 U block and 1 V block).

---

## Experimental Results

Below are the compression ratio analysis tables under various encoding profiles.

### Image Baseline Analysis

| Input Image | Dimensions | Input Size | Output Size (KiB) |
| :--- | :--- | :--- | :--- |
| Mandrill / Baboon | 512 x 512 | 768 KB | 76.6 |
| New Year (Sample A) | 800 x 600 | 1.37 MB | 42.5 |
| Cat Portrait | 1024 x 1024 | 2.16 MB | 169.0 |
| Green Cat | 604 x 488 | 65.8 KB | 51.5 |
| Playing Kitten | 960 x 640 | 246 KB | 136.0 |
| New Year (Sample B) | 800 x 600 | 87.9 KB | 42.4 |

### Compression Quality Scaling Impact
*Test Case Asset: `playing_kitten.jpg` (Baseline Input: 246 KB | High Compression Baseline at Quality = 1: 13.6 KB)*

| Quality Profile Parameter | Output Size (KiB) |
| :---: | :---: |
| **10** | 31.9 |
| **20** | 50.3 |
| **30** | 65.5 |
| **40** | 79.0 |
| **50** | 90.0 |
| **60** | 103.0 |
| **70** | 126.0 |
| **80** | 150.0 |
| **90** | 236.0 |
| **100** | 543.0 |

---

## Architecture & Build Details

The project layout separates development headers, source logic, build outputs, and validation sets:

*   `/src` — Core application logic and algorithmic pipeline pipelines.
*   `/inc` — C++ header files and function definitions.
*   `/in_img` — Reference test images.
*   `/outImages` — Compressed output image directory.
*   `/imgQuality` — Comparative assets for quantization tuning.

### Key Classes & API Methods
*   `JPEGBitStreamWriter`: Handles the standard bitstream framing.
    *   `writeHeader()`: Writes standard JFIF compliant file markers.
    *   `writeQuantizationTables()`: Packs 1D mapped serialization vectors.
    *   `writeImageInfo()`: Commits global structural image dimensions and channels.
*   `writeBlockY()`, `writeBlockU()`, `writeBlockV()`: Channel-specific data injectors utilizing precise byte alignment mechanisms.
