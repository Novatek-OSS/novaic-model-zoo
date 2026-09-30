<br> <p align="right">\[[简体中文](README.md) | [English](README_EN.md)\]</p>

# ResNet

## Model Source

The model source of this sample is:<br>
https://github.com/pytorch/vision/blob/main/torchvision/models/resnet.py

## Model Precision

The precision data of quantized NVT models: [link](precision.md)

## NVT Model

The converted NVT model bin can be downloaded from:<br>
https://github.com/Novatek-OSS/novaic-model-zoo/releases

## Sample

In this document, `<model>` refers to `resnet`.

### Clone Repository

Clone the model zoo repository into the `code/zoo/` directory of the Novatek SDK.

### Build Sample

1. Follow the steps for building the Novatek SDK.
2. Execute these commands:
```sh
cd code/zoo/samples/<model>
make
```
3. The compiled sample executable is `classification_sample` in the sample directory.

### Install Sample

Execute the following command to copy the sample executable, label file, and required shared libraries to the `output/classification_sample` directory.
```sh
make install
```

### Deploy to Device

1. Format an SD card.
2. Copy the loader binary to the SD card (e.g., `LD98635T.bin` for NT98635).
3. Copy the firmware binary to the SD card (e.g., `FW98635A.bin` for NT98635).
4. Copy the `output/classification_sample` directory to the SD card root (`<sd_sample_dir>`).
5. Copy the sample model file `nvt_model.bin` to `<sd_sample_dir>`.
6. Copy a test JPEG image (e.g., ImageNet ILSVRC2012_val_00000002.JPEG) to `<sd_sample_dir>`.
7. Insert the SD card into the device and power it on. The SD card is mounted as `/mnt/sd` on the device.
8. Copy `libjpeg.so.10` from `<sd_sample_dir>` to `/usr/lib/` on the device.

### Run Inference

Execute these commands:
```sh
cd /mnt/sd/classification_sample
./classification_sample ILSVRC2012_val_00000002.JPEG
```

### Output

```
Classification Results:
+------+-------------+-------+--------------------------------------------+
|  ID  |    Score    |  No.  |                   Categories               |
+------+-------------+-------+--------------------------------------------+
|   1  |  0.9492140  |  795  | ski                                        |
|   2  |  0.0216372  |  970  | alp                                        |
|   3  |  0.0002747  |  537  | dogsled, dog sled, dog sleigh              |
|   4  |  0.0001221  |  792  | shovel                                     |
|   5  |  0.0001221  |  796  | ski mask                                   |
+------+-------------+-------+--------------------------------------------+
```
