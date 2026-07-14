# Findings: UI Font Color & Position

## 页面结构发现

### 标签分类
- **图标标签** (`ui_Image*`): 大部分已设置颜色（白色/语义色），命名规律是 `Image` 前缀但实际是 `lv_label_create`
- **文本标签** (`ui_Label*`): 大部分未设置颜色，LVGL 默认黑色，在黑底上不可见
- **动态标签** (`ui_Time*`, `ui_battery*`, `ui_Date*`): 时间/电量等，HOME 页面有颜色，其他页面缺失

### 页面标题标签（左上角）
| 文件 | 变量 | 文本 |
|------|------|------|
| ui_Setting_Dis_Bri_Page.c | ui_Label13 | "Display & Brightness" |
| ui_Setting_Barttery_Saver_Page.c | ui_Label21 | "Battery-Saver Mode" |
| ui_Setting__SystemUpdates_Page.c | ui_Label19 | "System&Updates" |
| ui_Setting_About_Page.c | ui_Label4 | "About" |
| ui_Setting_Page.c | ui_Label10 | "Setting" |
| ui_Time_Count_Page.c | ui_Label14 | "Stopwatch" |

### 时间标签位置（TOP_RIGHT 对齐）
- 原始 x=-5，文本右侧紧贴屏幕边缘，容易被裁切
- 修改后 x=-15，留出 10px 右侧空间
- Setting_Page 使用分列显示（Hour/Colon/Minute 三个标签），需同步调整

### LVGL 默认颜色
- LVGL 默认文字颜色为黑色 (0x000000)
- 黑底页面 + 黑色文字 = 不可见
- 所有页面背景均为黑色 (0x000000)

## STOP 模式唤醒问题

### 根因分析
1. **GPIO 时序竞争**：EXTI 在下降沿唤醒 CPU，但恢复时钟需要时间，此时 GPIO 可能已恢复高电平，`!KEY1` 为 false → `goto sleep`
2. **MPU6050 未重初始化**：STOP 模式后 I2C/MPU 状态不确定，`MPU_isHorizontal()` 返回不可靠值
3. **RTC 每秒唤醒循环**：每次 `goto sleep` 等待下一次 RTC 唤醒（~1s），多次失败累积为 10+s

### 修复方案
- KEY1/KEY2 使用 EXTI 标志位替代 GPIO 轮询，消除时序竞争
- 按键按下时跳过 MPU 抬腕检测，直接唤醒
- 唤醒后重新初始化 MPU6050，确保后续抬腕检测可靠

### 已修改文件清单
| 文件 | 颜色修改 | 位置修改 |
|------|---------|---------|
| ui_Application_Page.c | 5 标签 | - |
| ui_Application1_Page.c | 3 标签 | - |
| ui_Charging_Page.c | 3 标签 | - |
| ui_Menu_Page.c | 4 标签 | - |
| ui_Setting_Page.c | 4 标签 | 3 标签 |
| ui_Setting_Dis_Bri_Page.c | 2 标签 | 1 标签 |
| ui_Setting_Barttery_Saver_Page.c | 2 标签 | 1 标签 |
| ui_Setting__SystemUpdates_Page.c | 2 标签 | 1 标签 |
| ui_Setting_About_Page.c | 8 标签 | 1 标签 |
| ui_Time_Count_Page.c | 4 标签 | 1 标签 |
| ui_Setting__SystemUpdates_Poweroff_Page.c | 1 标签 | - |
