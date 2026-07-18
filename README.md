# OV-Watch PL — 智能手表复刻项目

基于 [No-Chicken/OV-Watch](https://github.com/No-Chicken/OV-Watch) V2.4.5 开源智能手表的复刻项目，包含硬件设计与固件源码。

> **原作者**: 不吃油炸鸡 (No-Chicken)
> **版本**: V2.4.5
> **项目主页**: https://github.com/No-Chicken/OV-Watch

---

## 项目简介

OV-Watch 是一款基于 STM32F411CEU6 的开源智能手表，采用 FreeRTOS 实时操作系统 + LVGL 图形界面框架，驱动 1.69 寸 240x280 ST7789 IPS 屏幕，支持触摸操作。

本项目（OV_Watch_PL）是对其的完整复刻，在原版基础上进行了以下改进：

- 修复页面重入时时间/电量显示冻结的 bug
- 优化低功耗模式（Stop 模式电流从 18mA 降至 <1mA）
- 添加协作看门狗机制
- 整理调试记录与架构文档

---

## 硬件规格

| 项目 | 规格 |
|------|------|
| MCU | STM32F411CEU6 (Cortex-M4F, 100MHz, 512KB Flash, 128KB RAM) |
| 屏幕 | 1.69 寸 IPS, 240x280, ST7789V, SPI+DMA |
| 触摸 | CST816T 电容触摸, I2C |
| IMU | MPU6050 (加速度计+陀螺仪, DMP 计步) |
| 温湿度 | AHT21 |
| 气压计 | SPL06 |
| 电子罗盘 | LSM303DLH (磁力计+加速度计) |
| 心率血氧 | EM7028 (预编译库) |
| 蓝牙 | KT6328 BLE, UART 透传 |
| 存储 | BL24C02 EEPROM (2Kbit) |
| 电源 | 锂电池 + 充电管理, ADC 电量检测 |
| 看门狗 | 外部硬件看门狗 + 协作喂狗机制 |

---

## 软件架构

```
┌─────────────────────────────────────────┐
│         Application Layer               │
│  (页面逻辑: 表盘/菜单/设置/传感器/...)    │
├─────────────────────────────────────────┤
│         GUI Framework (LVGL v8.2)       │
├─────────────────────────────────────────┤
│         Page Manager (页面栈)            │
├─────────────────────────────────────────┤
│         HWInterface (硬件抽象层)         │
├─────────────────────────────────────────┤
│         FreeRTOS v10.3.1 (10个任务)      │
├─────────────────────────────────────────┤
│         BSP Drivers (板级驱动)           │
├─────────────────────────────────────────┤
│         STM32 HAL + CubeMX              │
└─────────────────────────────────────────┘
```

### FreeRTOS 任务

| 优先级 | 任务名 | 周期 | 职责 |
|--------|--------|------|------|
| High+3 | HardwareInitTask | 一次性 | 初始化硬件+LVGL, 自删除 |
| Low | LvHandlerTask | 1ms | LVGL 主循环 |
| Normal | WDOGFeedTask | 500ms | 协作看门狗 |
| High | PowerMgrTask | 10ms | 低功耗管理 (灭屏/Stop) |
| Normal | KeyTask | 1ms | 按键扫描 |
| Low+1 | ScrRenewTask | 10ms | 页面切换 |
| Low+1 | SensorTask | 500ms | 传感器采集 |
| Low+2 | DataSaveTask | 100ms | EEPROM 存储 |
| Low+2 | ChargCheckTask | 500ms | 充电检测 |
| Low+2 | MessageTask | 1000ms | BLE 收发 |

### 低功耗设计

- **Idle 模式**: 背光降至 5%, 功耗约 15mA → 5mA
- **Stop 模式**: STM32 Stop, 关闭 LCD/Touch/UART, 功耗约 50μA
- **唤醒源**: 按键 / 充电检测 / 抬腕 (MPU6050)

---

## 目录结构

```
OV_Watch_PL/
├── Hardware/                      # 硬件设计
│   ├── OV-Watch_V2.4_*.epro      # 立创 EDA 工程
│   ├── SCH_*.pdf                  # 原理图
│   ├── Gerber_*.zip               # Gerber 生产文件
│   ├── 配件采购清单.pdf             # BOM
│   └── 芯片手册/                   # 外设芯片手册
├── Software/OV_Watch/             # 固件源码
│   ├── Core/                      # CubeMX 生成 (HAL 初始化)
│   ├── Drivers/BSP/               # 板级驱动 (各外设独立)
│   ├── Middlewares/LVGL/           # LVGL v8.2 + 移植层
│   ├── User/
│   │   ├── Func/                  # HWInterface + PageManager
│   │   ├── Tasks/                 # FreeRTOS 任务
│   │   └── GUI_App/               # LVGL UI 页面
│   ├── MDK-ARM/                   # Keil 工程
│   └── OV_Watch.ioc               # CubeMX 配置
├── docs/                          # 架构文档
├── 软件架构设计.md                  # 详细架构设计
└── 调试bug/调试bug.md              # Bug 记录与解决方案 (17个)
```

---

## 构建

### 环境要求

- Keil MDK-ARM 5
- STM32CubeMX (如需修改引脚/外设配置)

### 编译步骤

1. 用 Keil 打开 `Software/OV_Watch/MDK-ARM/OV_Watch.uvprojx`
2. 点击 Build (F7)
3. 烧录到 STM32F411 开发板

### 清理构建产物

```bash
cd Software/OV_Watch
./KeilClear.bat
```

---

## 关键文件

| 文件 | 说明 |
|------|------|
| `User/Func/Inc/HWDataAccess.h` | HWInterface 硬件抽象层 + 编译开关 |
| `User/Tasks/Src/user_TasksInit.c` | FreeRTOS 任务与消息队列创建 |
| `User/Func/Src/PageManager.c` | 页面栈管理器 |
| `User/GUI_App/ui.c` | UI 入口 |
| `Core/Inc/FreeRTOSConfig.h` | FreeRTOS 配置 (32KB 堆, 1000Hz tick) |
| `Middlewares/LVGL/GUI/lv_conf.h` | LVGL 配置 (RGB565, 48KB 内存池) |
| `软件架构设计.md` | 完整架构设计文档 |
| `调试bug/调试bug.md` | 17 个 Bug 的现象/原因/修复记录 |

---

## 硬件图片

PCB 设计文件位于 `Hardware/` 目录，使用立创 EDA (EasyEDA) 打开 `.epro` 文件。

| 文件 | 说明 |
|------|------|
| `SCH_Core_*.pdf` | 核心板原理图 |
| `SCH_Back_*.pdf` | 背板原理图 |
| `Gerber_Core_*.zip` | 核心板 Gerber (可直接下单生产) |
| `Gerber_Back_*.zip` | 背板 Gerber |
| `配件采购清单.pdf` | BOM 采购清单 |

---

## 调试记录

项目记录了 17 个调试问题，详见 `Software/OV_Watch/调试bug/调试bug.md`，涵盖：

- FreeRTOS 栈溢出导致 HardFault
- LVGL 内存池耗尽 / Use-after-free
- 页面重入后显示冻结
- Stop 模式功耗优化 (18mA → <1mA)
- 抬腕亮屏功能调试
- 设置持久化 (EEPROM)

---

## 致谢

- **原作者**: [No-Chicken (不吃油炸鸡)](https://github.com/No-Chicken) — OV-Watch 全部硬件与软件设计
- **开源库**: [LVGL](https://lvgl.io/), [FreeRTOS](https://www.freertos.org/), [STM32 HAL](https://github.com/STMicroelectronics/STM32CubeF4)

---

## 许可证

本项目遵循原作者的开源许可。硬件设计与固件源码版权归原作者 [No-Chicken](https://github.com/No-Chicken/OV-Watch) 所有。
