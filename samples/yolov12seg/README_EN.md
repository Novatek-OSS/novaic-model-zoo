<br> <p align="right">\[[简体中文](README.md) | [English](README_EN.md)\]</p>

# YOLOv12-seg

## Model Source

The model source of this sample is:<br>
https://github.com/sunsmarterjie/yolov12/releases/download/seg/yolov12n-seg.pt

## Model Precision

The precision data of quantized NVT models: [link](precision.md)

## NVT Model

The converted NVT model bin can be downloaded from:<br>
https://github.com/Novatek-OSS/novaic-model-zoo/releases

## Sample

In this document, `<model>` refers to `yolov12seg`.

### Clone Repository

Clone the model zoo repository into the `code/zoo/` directory of the Novatek SDK.

### Build Sample

1. Follow the steps for building the Novatek SDK.
2. Execute these commands:
```sh
cd code/zoo/samples/<model>
make
```
3. The compiled sample executable is `<model>_sample` in the sample directory.

### Install Sample

Execute the following command to copy the sample executable, label file, and required shared libraries to the `output/<model>_sample` directory.
```sh
make install
```

### Deploy to Device

1. Format an SD card.
2. Copy the loader binary to the SD card (e.g., `LD98635T.bin` for NT98635).
3. Copy the firmware binary to the SD card (e.g., `FW98635A.bin` for NT98635).
4. Copy the `output/<model>_sample` directory to the SD card root (`<sd_sample_dir>`).
5. Copy the sample model file `nvt_model.bin` to `<sd_sample_dir>`.
6. Copy a test JPEG image (e.g., [bus.jpg](https://github.com/ultralytics/ultralytics/blob/main/ultralytics/assets/bus.jpg)) to `<sd_sample_dir>`.
7. Insert the SD card into the device and power it on. The SD card is mounted as `/mnt/sd` on the device.
8. Copy `libjpeg.so.10` from `<sd_sample_dir>` to `/usr/lib/` on the device.

### Run Inference

Execute these commands:
```sh
cd /mnt/sd/<sd_sample_dir>
./<model>_sample bus.jpg
```

### Output

```
Top 4 results:
+----+--------------+--------+--------+--------+--------+--------+
| ID |    Class     |  Conf  |  xmin  |  ymin  |  xmax  |  ymax  |
+----+--------------+--------+--------+--------+--------+--------+
|  5 |     bus      |  90.0% |     0  |   222  |   809  |   744  |
|  0 |    person    |  86.6% |    54  |   396  |   254  |   903  |
|  0 |    person    |  84.5% |   217  |   411  |   340  |   859  |
|  0 |    person    |  84.5% |   673  |   380  |   809  |   882  |
+----+--------------+--------+--------+--------+--------+--------+
save merged mask to mask.bin, size 810x1080
```
