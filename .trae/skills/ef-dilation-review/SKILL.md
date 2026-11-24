---
name: ef-dilation-review
description: Review the E/F point dilation logic in Cross_State_ABLost_Action (Element.c) for index, bounds, counter-reset, flag-coupling, cache-timing and mirror-consistency bugs. Use when the user asks to check or re-check the E/F expansion or 膨胀 code. Do not use for feature implementation or generic code review.
---

# E/F 点膨胀代码复查

针对本项目 `Harderware/Element.c` 中 `Cross_State_ABLost_Action` 函数里 E/F 角点的查找、合规判断、膨胀与补线逻辑做专项复查。只报告问题和原因，不主动改代码，除非用户明确要求修改。

## 固定上下文（每次复查前先确认）

- 图像缓冲区：`Image1[GRAY_W][GRAY_H]`，本项目 `GRAY_W = 94`、`GRAY_H = 60`（见 `Harderware/Gray.h`），合法下标 `x ∈ [0,93]`、`y ∈ [0,59]`。
- 像素约定：`WHITE = 0xFF` 是赛道，`BLACK = 0x00` 是赛道外；E 是"左侧最后一列黑点"（黑左白右），F 是镜像（白左黑右）。
- 偏移表：`L_expansionX_Plus / L_expansionY_Plus`（E 用）、`R_expansionX_Plus / R_expansiony_Plus`（F 用），均 5 个元素、含 -1 偏移。
- 相关全局：`AX/AY/BX/BY` 只在 A/B 找到时更新（`Harderware/Gray.c`），进入 A/BLost 后是过期值。
- 用户约束：建议修复时**不得修改已有参数/变量名称**（如 `E_PointX`、`Normal_EPoitn_Flag` 等保持原样）。

## 复查清单（逐项过，每项都要给出"是否命中 + 行号 + 原因"）

### 1. 下标正确性
- 所有 `Image1[第一维][第二维]`：第一维必须是列（X 变量），第二维必须是行（Y 变量）。重点抓复制粘贴笔误（例如把 `E_PointY` 写成 `E_PointX`）。

### 2. 数组越界
- 逐个 `Image1[x±k][y±m]` 验证 `x±k ∈ [0,93]`、`y±m ∈ [0,59]`。
- 特别注意 `uint8` 下溢：`F_PointX - 1` 在 `F_PointX == 0` 时变成 255；`E_PointX + 1 / + 3` 在右边界附近超过 93。
- 合规判断里新加的 `x±3`、`y+2` 也要算边界。

### 3. 计数器复位与采样完整性
- `E_CountBlack / F_CountBlack` 必须**每轮清零**，且**采满 5 个点后再判阈值**，不能在 for 循环内部每采一个点就判。
- 警惕 `i = 0` 写在循环体末尾：紧接着的 `i++` 会跳过 i=0 那个采样点，导致第二轮起只采 4 点。建议用 `while` 或把判决移到 for 外。

### 4. 标志位耦合
- E、F 的合规判断和膨胀必须**各自独立**受 `E_Found / F_Found` 控制，不能用 `Normal_E == 1 && Normal_F == 1` 之类把两侧绑死——否则一侧失效会连带另一侧不膨胀、不缓存。
- 检查 `Normal_EPoitn_Flag / Normal_FPoitn_Flag` 的所有置 1 / 清 0 路径是否对称、是否漏判。

### 5. `Last_*` 缓存时机与初值
- `Last_E_PointX/Y`、`Last_F_PointX/Y`（static）应在"找到且合规"时更新，不能只在"膨胀触发"分支里更新。
- 更新应发生在**膨胀完成之后**，保证兜底锚点与合规分支实际使用的锚点一致。
- static 跨十字不复位：新一段十字会用上一段的旧锚点。
- 初值为 0：`Normal==0` 的兜底分支若直接用 (0,0) 当锚点，补线会被夹到边界 → 错误。必须有守卫（如只在曾合规过时用 Last，否则用当前帧点或直接 break）。

### 6. 合规判据的输入有效性
- 若判据依赖 `A_X/A_Y/B_X/B_Y` 等过期全局量，指出风险；`A_Y == 0` 时 `< A_Y` 恒假这类退化要提示。
- 任何基于当前图像像素的合规条件，先确认下标在边界内。

### 7. E/F 镜像一致性
- 偏移表、循环变量（E 用 `i`、F 用 `j`）、膨胀方向（E 右移、F 左移）要严格镜像。
- 补线锚点：E 用 `E_PointX`，F 若用 `F_PointX + 1` 需确认是有意的 1 像素偏移，否则是镜像不对称。

### 8. 强制类型转换
- `(int8)` 强转后再加偏移：坐标为 0 时结果为 -1，语义脆弱；建议 int16 中间变量 + 限幅。

### 9. 循环变量复用
- `i / j` 在"查找 → 膨胀 → 补线"之间反复使用，确认每个使用前都被重新初始化，没有沿用上一段的残留值。

### 10. 悬空定义 / 编译一致性
- 检查 `#define`（如 `Max_Expansion_Step`、`FindEF_Span`、`Stop_expansionBlackBlock` 等）是否定义且被引用；定义了却零引用的要指出。
- 函数签名与 `Harderware/Element.h` 声明、`Harderware/Gray.c` 调用点必须一致。

## 输出格式

- 先列"本轮已确认修复"的项（带行号）。
- 再列仍存在 / 新引入的问题，每条包含：**严重度**（致命 / 严重 / 中 / 低）、**行号链接**、**现象**、**原因**、**建议修法**。
- 不主动修改代码；用户要求改时再动手，并遵守"不改已有参数名"的约束。
