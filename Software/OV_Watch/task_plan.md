# Task Plan: OV-Watch UI Polish

## Goal
统一 OV-Watch 各页面的字体颜色为白色，并修正右上角时间标签的显示位置。

## Current Phase
Phase 3

## Phases

### Phase 1: 检测未设置字体颜色的标签
- [x] 扫描所有 UI 页面文件，找出未调用 `set_style_text_color` 的标签
- [x] 汇总报告给用户（45 个标签，跳过 2 个空标签）
- **Status:** complete

### Phase 2: 设置字体颜色为白色
- [x] 将 41 个标签设为白色 (0xFFFFFF)
- [x] 用户要求 Setting 相关页面撤回部分修改（保留时间+左上角标题）
- [x] 撤回 12 个 Setting 页面标签的颜色设置
- [x] 用户要求 About 页面全部改回白色
- [x] 恢复 About 页面 6 个标签为白色
- **Status:** complete

### Phase 3: 调整右上角时间标签位置
- [x] 用户反馈时间显示偏右不全
- [x] 将 7 个页面的 TOP_RIGHT 时间标签 x 各减 10
- [x] Setting_Page 的分列时间标签（HH:MM）三列各减 10
- **Status:** complete

### Phase 4: 修复 STOP 模式唤醒慢问题
- [x] 分析唤醒验证逻辑，定位根因（GPIO 时序竞争 + MPU 未重初始化）
- [x] 添加 KEY1/KEY2 EXTI 标志位（stm32f4xx_it.c/h）
- [x] 修改唤醒验证逻辑，使用 EXTI 标志替代 GPIO 轮询
- [x] 唤醒后重新初始化 MPU6050
- **Status:** complete

### Phase 5: 验证 & 提交
- [ ] 编译验证
- [ ] Git 提交
- **Status:** pending

## Decisions Made
| Decision | Rationale |
|----------|-----------|
| 跳过 ui_Label30/31 | 空标签，所在 Panel 已设 HIDDEN |
| Setting 页面只保留时间+标题白色 | 用户明确要求，卡片内容标签恢复原样 |
| About 页面全部白色 | 用户明确要求 |
| 时间标签统一左移 10px | 用户反馈右侧显示不全 |

## Errors Encountered
| Error | Attempt | Resolution |
|-------|---------|------------|
| 无 | - | - |
