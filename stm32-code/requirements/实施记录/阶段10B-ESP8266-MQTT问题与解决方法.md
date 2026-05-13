# 阶段 10B：ESP8266 MQTT 连接与数据上报 — 问题与解决方法

## 问题 1：`typedef enums` 语法错误

**现象**：编译失败。

**原因**：`esp8266.h` 中写成了 `typedef enums`，多了一个 `s`。

**解决**：改为 `typedef enum`。

---

## 问题 2：OLED 显示 "W:Er" 但设备实际已连接

**现象**：OLED 右上角持续显示 `W:Er`，但 OneNET 平台显示设备在线。

**原因**：OLED 代码只判断了 `ESP_CONN_WIFI_OK` 状态显示 "W:OK"。但 WiFi 连接成功后状态机立即推进到 MQTT 阶段（`ESP_CONN_MQTT_USERCFG` → `ESP_CONN_MQTT_OK`），不再停留在 `ESP_CONN_WIFI_OK`，导致走入 `else` 分支。

**解决**：OLED 改用 `esp8266_mqtt_is_connected()` 判断，MQTT 连接成功显示 `M:OK`。

```c
// 修改前
if (ws == ESP_CONN_WIFI_OK) { oled_show_string(0, 96, "W:OK"); }

// 修改后
if (esp8266_mqtt_is_connected()) { oled_show_string(0, 96, "M:OK"); }
```

---

## 问题 3：OneNET 属性全部显示 undefined（浮点格式化失败）

**现象**：OneNET 平台所有 6 个属性值全部显示 `undefined`。

**原因**：工程使用 `--specs=nano.specs` 链接（newlib-nano），`snprintf` 的 `%.1f` 浮点格式化**默认不支持**，输出为空字符串，导致整个 JSON 格式损坏，OneNET 无法解析。

**解决**：不使用浮点格式化，改为整数拆分：

```c
// 修改前
float temp_c = (float)g_ctrl.temp_raw / 16.0f;
snprintf(..., "%.1f", temp_c);  // nano.specs 下输出为空

// 修改后
int16_t temp_int  = g_ctrl.temp_raw / 16;
int16_t temp_frac = (g_ctrl.temp_raw % 16) * 10 / 16;
if (temp_frac < 0) temp_frac = -temp_frac;
snprintf(..., "%d.%d", (int)temp_int, (int)temp_frac);
```

**备选方案**：添加链接器标志 `-u _printf_float`，但会增加约 5KB Flash。

---

## 问题 4：温度能上报但其他属性仍显示 undefined

**现象**：简化为只发 `temperature` 后 OneNET 能正确显示，恢复全部 6 个字段后其他属性仍为 `undefined`。

**原因**：OneNET 物模型 `pump_state` 和 `fan_state` 定义为 bool 类型，期望 JSON 布尔值 `true`/`false`，而代码用的是整数 `0`/`1`。

**解决**：

```c
// 修改前
"\"pump_state\":{\"value\":%u}", (unsigned)g_ctrl.pump_on   // 输出 0 或 1

// 修改后
"\"pump_state\":{\"value\":%s}", g_ctrl.pump_on ? "true" : "false"  // 输出 true 或 false
```

---

## 问题 5：`soil_moisture` 原始 ADC 值超出物模型范围

**现象**：`soil_moisture` 显示 `undefined`。

**原因**：物模型定义范围 0-100（百分比），但代码直接发送原始 ADC 值（0-4095），超出范围后 OneNET 拒绝。

**解决**：ADC 原始值转换为百分比：

```c
unsigned soil_pct = (unsigned)g_ctrl.soil_val * 100U / 4096U;
if (soil_pct > 100U) soil_pct = 100U;
```

---

## 问题 6：ESP8266 回显干扰 AT 响应匹配

**现象**：AT 指令偶尔匹配失败或超时。

**原因**：ESP8266 默认开启回显（ATE1），发送的 AT 命令文本会被回显到 RX 缓冲区，混在真正的响应中间，可能干扰 `strstr` 匹配。

**解决**：在 AT 测试成功后立即发送 `ATE0` 关闭回显，新增 `ESP_CONN_ATE0` 状态：

```
连接流程：AT → ATE0 → AT+CWMODE=1 → AT+CWJAP → MQTT配置 → MQTT连接 → 订阅
```

---

## 最终数据上报格式

```json
{
  "id": "123",
  "params": {
    "temperature": {"value": 23.6},
    "soil_moisture": {"value": 43},
    "pump_state": {"value": false},
    "fan_state": {"value": false},
    "mode": {"value": 0},
    "status": {"value": 0}
  }
}
```

## 关键经验总结

| 教训 | 说明 |
|------|------|
| nano.specs 不支持浮点 printf | STM32 嵌入式开发经典坑，用整数拆分替代 `%.1f` |
| OneNET bool 类型用 JSON boolean | `true`/`false` 而非 `0`/`1` |
| 物模型值域必须匹配 | 超出定义范围的值会被静默丢弃 |
| 关闭 ESP8266 回显 | `ATE0` 避免 RX 缓冲区污染 |
| 状态机设计注意覆盖性 | OLED 显示需覆盖所有连接阶段状态 |