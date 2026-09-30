<br> <p align="right">\[[简体中文](README.md) | [English](README_EN.md)\]</p>

# ResNet

## 模型来源

本示例所使用的模型来源如下：<br>
https://github.com/pytorch/vision/blob/main/torchvision/models/resnet.py

## 模型精度

量化后的 NVT 模型精度数据：[link](precision.md)

## NVT 模型

转换后的 NVT 模型二进制文件可从以下地址下载：<br>
https://github.com/Novatek-OSS/novaic-model-zoo/releases

## 示例

本文档中，`<model>` 表示 `resnet`。

### 克隆仓库

将模型仓库克隆到 Novatek SDK 的 `code/zoo/` 目录中。

### 构建示例

1. 请依照 Novatek SDK 的构建步骤进行操作。
2. 执行以下命令：
```sh
cd code/zoo/samples/<model>
make
```
3. 编译后的示例可执行文件为 `classification_sample`，位于示例目录中。

### 安装示例

执行以下命令，将示例可执行文件、标签文件及所需的共享库复制到 `output/classification_sample` 目录中。
```sh
make install
```

### 部署到设备

1. 格式化 SD 卡。
2. 将 loader 二进制文件复制到 SD 卡中（例如 NT98635 使用 `LD98635T.bin`）。
3. 将固件二进制文件复制到 SD 卡中（例如 NT98635 使用 `FW98635A.bin`）。
4. 将 `output/classification_sample` 目录复制到 SD 卡根目录（`<sd_sample_dir>`）。
5. 将示例模型文件 `nvt_model.bin` 复制到 `<sd_sample_dir>` 中。
6. 复制一张测试用的 JPEG 图片（例如 ImageNet ILSVRC2012_val_00000002.JPEG）到 `<sd_sample_dir>` 中。
7. 将 SD 卡插入设备并开机。SD 卡会挂载为设备上的 `/mnt/sd`。
8. 将 `libjpeg.so.10` 从 `<sd_sample_dir>` 复制到设备的 `/usr/lib/` 目录中。

### 运行推理

执行以下命令：
```sh
cd /mnt/sd/classification_sample
./classification_sample ILSVRC2012_val_00000002.JPEG
```

### 输出结果

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
