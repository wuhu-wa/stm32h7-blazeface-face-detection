# STM32H7 BlazeFace 人脸检测与云台追踪系统

基于 **STM32H743VGT6** 微控制器与 **ST X-CUBE-AI** 边缘人工智能推理框架实现的嵌入式实时人脸检测与舵机云台追踪系统。

---

## 🌟 项目特性

- **高性能边缘推理**：基于 ARM Cortex-M7 (480 MHz) 架构与 AXI SRAM 优化，使用 ST X-CUBE-AI 部署轻量级 **BlazeFace** 神经网络模型。
- **高速图像采集**：通过 DCMI 接口配合 DMA 连续采集 OV2640 图像数据，结合 CPU D-Cache 无效化保障缓存一致性与低延迟处理。
- **实时显示与 OSD 标注**：驱动 1.54 寸 SPI LCD（240×240 分辨率）实时显示视频流，叠加人脸检测框、置信度、实时帧率（FPS）及 AI 推理耗时（ms）。
- **闭环云台追踪与巡检**：
  - **人脸追踪**：计算人脸水平中心偏离误差，动态调节 TIM1 PWM 舵机占空比，设置死区抑制机械抖动。
  - **丢失巡检**：当目标移出视野超过设定时间后，舵机自动进入往复扇形扫描巡检模式，重新捕捉人脸。
- **故障诊断与指示**：内置摄像头/AI 初始化错误自检界面与 LED 心跳指示。

---

## 🛠️ 硬件配置与外设

| 外设/模块 | 硬件型号 / 协议 | 接口 / 引脚 | 说明 |
| :--- | :--- | :--- | :--- |
| **主控芯片** | STM32H743VGT6 | LQFP100, 480 MHz | 1MB RAM / 1MB Flash |
| **摄像头** | OV2640 模块 | DCMI + SCCB (I2C) | 采集输出 RGB565，连续 DMA 传输 |
| **显示屏** | 1.54 寸 IPS LCD (240×240) | SPI (ST7789) | DC: PE15, BL: PD15 |
| **执行机构** | 舵机云台 | TIM1_CH4 (PWM) | 周期 20ms，脉宽 500~2500μs 控制角度 |
| **调试输出** | 串口调试器 | USART1 (PA9/PA10) | 波特率 115200，重定向 `printf` |
| **指示灯** | 板载 LED | GPIO (PE3) | 帧循环与运行状态指示 |

---

## 📂 目录结构

```text
├── Core/               # STM32CubeMX 生成的核心代码 (主循环 main.c、时钟、外设初始化)
├── Drivers/            # STM32H7xx HAL 库及 CMSIS 头文件
├── Middlewares/        # ST AI 中间件及运行库
├── User/               # 用户外设驱动组件
│   ├── Inc/            # 驱动头文件 (OV2640、LCD、SCCB、LED 等)
│   └── Src/            # 驱动实现 (dcmi_ov2640.c, lcd_spi_154.c 等)
├── X-CUBE-AI/          # X-CUBE-AI 模型生成文件
│   ├── App/            # AI 预处理、模型调用与后处理 (app_x-cube-ai.c)
│   └── Target/         # AI 运行环境底层配置
├── MDK-ARM/            # Keil uVision5 工程目录 (detect.uvprojx)
├── detect.ioc          # STM32CubeMX 项目工程配置文件
└── README.md           # 项目说明文档
```

---

## 🚀 快速上手

### 1. 开发环境要求
- **Keil MDK-ARM** (v5.30 及以上，支持 ARM Compiler 5 / 6)
- **STM32CubeMX** (v6.x 及以上)
- **X-CUBE-AI** 扩展包 (v7.x 或更高)
- 硬件调试器：ST-LINK V2 / V3 或 J-Link

### 2. 编译与烧录
1. 使用 Keil 打开 `MDK-ARM/detect.uvprojx`。
2. 点击 **Rebuild All** 编译工程（确保包含 math 库链接）。
3. 使用调试器将固件下载至 STM32H743 目标板中。

### 3. 运行与验证
- 目标板上电后，屏幕首先展示 `AI INIT` 状态信息。
- 初始化完成后，LCD 屏幕将显示摄像头实时画面。
- 将面部对准摄像头，画面将出现绿色人脸检测框，同时云台舵机将随人脸水平移动自动跟踪对焦；人脸移开后将自动进入左右寻迹扫描。

---

## 📄 开源许可

本项目遵循 MIT 协议。
