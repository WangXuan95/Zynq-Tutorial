
Vivado+PetaLinux 系统搭建教程
===========================

—— 以 **[ZedBoard](http://zedboard.org/product/zedboard)** 为例

## 概述

本教程使用 **Vivado2019.1** + **PetaLinux2019.1** 在 **[ZedBoard](http://zedboard.org/product/zedboard)** 上搭建了一个 SoC 系统，以 PS（ARM CPU）为核心，使用 PL（FPGA）实现一些外设。

本教程并不只机械的讲述操作流程，而是在操作流程中穿插了一些知识点：

+ ZYNQ的基本架构和原理。例如：
    + PS与PL如何交互？：GP和HP接口怎么用？
    + 什么是MIO和EMIO？
    + 三种使用外设的方式：PS-MIO外设、PS-EMIO外设、PL外设。
+ 在Vivado中的硬件已经准备好后，如何使用PetaLinux生成Linux启动镜像，并让它在ZedBoard上启动。本教程讲解2种启动方式：
    + SD卡启动
    + SPI-Flash启动
+ 如何使用C语言编写Linux应用程序，把外设用起来，本教程提供2个示例：
    + GPIO流水灯示例
    + Central-DMA (CDMA) 内存数据搬移示例

## 开始阅读

请从《**Zedboard Vivado+PetaLinux 系统搭建教程.pdf**》开始阅读。

目录：

+ 概述
+ 准备工作
    + 安装 Vivado 2019.1
    + 安装 PetaLinux 2019.1
+ Vivado 硬件搭建
    + 建立工程
    + 建立BlockDesign
    + 编写Verilog顶层文件
    + 建立引脚约束
    + 综合、实现、生成bitstream、生成硬件描述（HDF文件）
+ 用PetaLinux制作SD卡系统镜像
    + ZYNQ启动原理
    + 制作SD卡启动镜像
    + 从SD卡启动
+ 运行软件
    + 测试串口
    + 测试以太网
    + 测试 GPIO
    + 测试 Central-DMA (CDMA) 
+ 用PetaLinux制作QSPI-flash启动镜像
    + 制作QSPI-flash启动镜像
    + 烧写QSPI-flash
    + 从QSPI-flash启动
