#include "control.h"
#include "soil.h"
#include "ds18b20.h"
#include "relay.h"
#include "led.h"
#include "beep.h"
#include "debug_uart.h"

/* ---- thresholds (ADC raw 12-bit: 0=wet, 4095=dry for typical capacitive sensor) ---- */
#define SOIL_DRY_THRESHOLD     2800U
#define SOIL_WET_THRESHOLD     2200U

/* ---- temperature thresholds (raw * 16, i.e. 35.0C = 560, 30.0C = 480) ---- */
#define TEMP_HIGH_THRESHOLD    560
#define TEMP_HIGH_RECOVER      480
#define TEMP_LOW_THRESHOLD     80
#define TEMP_LOW_RECOVER       160

/* ---- timing ---- */
#define CONTROL_PERIOD_MS      1000U
#define ALARM_BEEP_PERIOD_MS   500U
#define LOG_PERIOD_MS          5000U

/* ---- pump safety: maximum continuous run time ---- */
#define PUMP_MAX_RUN_MS        60000U

static ctrl_status_t g_status;
static uint32_t      g_control_tick    = 0U;
static uint32_t      g_alarm_tick      = 0U;
static uint32_t      g_log_tick        = 0U;
static uint32_t      g_pump_start_tick = 0U;

/**
 * @brief 根据土壤湿度平均值更新土壤级别状态
 * 
 * @param soil_avg 土壤湿度的平均值
 * @return 无返回值
 */
static void control_update_soil(uint16_t soil_avg)
{
  switch (g_status.soil_level)
  {
    case CTRL_SOIL_OK:
    case CTRL_SOIL_WET:
      if (soil_avg >= SOIL_DRY_THRESHOLD)
      {
        g_status.soil_level = CTRL_SOIL_DRY;
      }
      break;

    case CTRL_SOIL_DRY:
      if (soil_avg <= SOIL_WET_THRESHOLD)
      {
        g_status.soil_level = CTRL_SOIL_OK;
      }
      break;

    default:
      g_status.soil_level = CTRL_SOIL_OK;
      break;
  }
}

/**
 * @brief 根据温度原始值更新温度级别状态
 * 
 * @param temp_raw 温度的原始值
 * @return 无返回值
 */
static void control_update_temp(int16_t temp_raw)
{
  switch (g_status.temp_level)
  {
    case CTRL_TEMP_OK:
      if (temp_raw >= TEMP_HIGH_THRESHOLD)
      {
        g_status.temp_level = CTRL_TEMP_HIGH;
      }
      else if (temp_raw <= TEMP_LOW_THRESHOLD)
      {
        g_status.temp_level = CTRL_TEMP_LOW;
      }
      break;

    case CTRL_TEMP_HIGH:
      if (temp_raw <= TEMP_HIGH_RECOVER)
      {
        g_status.temp_level = CTRL_TEMP_OK;
      }
      break;

    case CTRL_TEMP_LOW:
      if (temp_raw >= TEMP_LOW_RECOVER)
      {
        g_status.temp_level = CTRL_TEMP_OK;
      }
      break;

    default:
      g_status.temp_level = CTRL_TEMP_OK;
      break;
  }
}

/**
 * @brief 控制水泵的开关状态
 * 
 * @return 无返回值
 */
static void control_drive_pump(void)
{
  uint32_t now = HAL_GetTick();

  if (g_status.soil_level == CTRL_SOIL_DRY)
  {
    if (!g_status.pump_running)
    {
      relay_on(RELAY_PUMP);
      g_status.pump_running = 1U;
      g_pump_start_tick = now;
      debug_uart_send_line("[CTRL] Pump ON (soil dry)");
    }
    else if ((now - g_pump_start_tick) >= PUMP_MAX_RUN_MS)
    {
      relay_off(RELAY_PUMP);
      g_status.pump_running = 0U;
      g_status.alarm_active = 1U;
      debug_uart_send_line("[CTRL] Pump SAFETY OFF (max run exceeded)");
    }
  }
  else
  {
    if (g_status.pump_running)
    {
      relay_off(RELAY_PUMP);
      g_status.pump_running = 0U;
      debug_uart_send_line("[CTRL] Pump OFF (soil ok)");
    }
  }
}

/**
 * @brief 控制风扇的开关状态
 * 
 * @return 无返回值
 */
static void control_drive_fan(void)
{
  if (g_status.temp_level == CTRL_TEMP_HIGH)
  {
    if (!g_status.fan_running)
    {
      relay_on(RELAY_FAN);
      g_status.fan_running = 1U;
      debug_uart_send_line("[CTRL] Fan ON (temp high)");
    }
  }
  else
  {
    if (g_status.fan_running)
    {
      relay_off(RELAY_FAN);
      g_status.fan_running = 0U;
      debug_uart_send_line("[CTRL] Fan OFF (temp ok)");
    }
  }
}

/**
 * @brief 控制报警装置（蜂鸣器和红灯）
 * 
 * @return 无返回值
 */
static void control_drive_alarm(void)
{
  uint8_t should_alarm = 0U;

  if (g_status.temp_level == CTRL_TEMP_LOW)
  {
    should_alarm = 1U;
  }
  if (g_status.alarm_active)
  {
    should_alarm = 1U;
  }

  if (should_alarm)
  {
    uint32_t now = HAL_GetTick();
    if ((now - g_alarm_tick) >= ALARM_BEEP_PERIOD_MS)
    {
      g_alarm_tick = now;
      beep_toggle();
    }
    led_on(LED_RED);
  }
  else
  {
    beep_off();
    led_off(LED_RED);
  }
}

/**
 * @brief 更新指示灯状态（根据当前工作模式）
 * 
 * @return 无返回值
 */
static void control_update_indicators(void)
{
  if (g_status.mode == CTRL_MODE_AUTO)
  {
    led_on(LED_GREEN);
    led_off(LED_BLUE);
  }
  else
  {
    led_off(LED_GREEN);
    led_on(LED_BLUE);
  }
}

/**
 * @brief 记录系统状态到调试串口
 * 
 * @return 无返回值
 */
static void control_log_status(void)
{
  uint32_t now = HAL_GetTick();
  if ((now - g_log_tick) < LOG_PERIOD_MS)
  {
    return;
  }
  g_log_tick = now;

  debug_uart_send_string("[CTRL] mode=");
  debug_uart_send_string(g_status.mode == CTRL_MODE_AUTO ? "AUTO" : "MANUAL");

  debug_uart_send_string(" soil=");
  switch (g_status.soil_level)
  {
    case CTRL_SOIL_DRY: debug_uart_send_string("DRY");  break;
    case CTRL_SOIL_WET: debug_uart_send_string("WET");  break;
    default:            debug_uart_send_string("OK");   break;
  }

  debug_uart_send_string(" temp=");
  switch (g_status.temp_level)
  {
    case CTRL_TEMP_HIGH: debug_uart_send_string("HIGH"); break;
    case CTRL_TEMP_LOW:  debug_uart_send_string("LOW");  break;
    default:             debug_uart_send_string("OK");   break;
  }

  debug_uart_send_string(" pump=");
  debug_uart_send_string(g_status.pump_running ? "ON" : "OFF");
  debug_uart_send_string(" fan=");
  debug_uart_send_string(g_status.fan_running ? "ON" : "OFF");
  debug_uart_send_string(" alarm=");
  debug_uart_send_line(g_status.alarm_active ? "YES" : "NO");
}

/**
 * @brief 初始化控制系统
 * 
 * @return 无返回值
 */
void control_init(void)
{
  g_status.mode          = CTRL_MODE_AUTO;
  g_status.soil_level    = CTRL_SOIL_OK;
  g_status.temp_level    = CTRL_TEMP_OK;
  g_status.pump_running  = 0U;
  g_status.fan_running   = 0U;
  g_status.alarm_active  = 0U;

  g_control_tick    = HAL_GetTick();
  g_alarm_tick      = g_control_tick;
  g_log_tick        = g_control_tick;
  g_pump_start_tick = 0U;

  relay_off(RELAY_PUMP);
  relay_off(RELAY_FAN);
  beep_off();

  debug_uart_send_line("[CTRL] Stage9 control init (AUTO mode)");
}

/**
 * @brief 控制系统的主任务循环
 * 
 * @return 无返回值
 */
void control_task(void)
{
  uint32_t now = HAL_GetTick();
  if ((now - g_control_tick) < CONTROL_PERIOD_MS)
  {
    return;
  }
  g_control_tick = now;

  uint16_t soil_avg = soil_read_avg(8);

  int16_t temp_raw = 0;
  uint8_t temp_ok  = ds18b20_is_valid();
  if (temp_ok)
  {
    temp_raw = ds18b20_get_cached_raw();
  }

  control_update_soil(soil_avg);

  if (temp_ok)
  {
    control_update_temp(temp_raw);
  }

  if (g_status.mode == CTRL_MODE_AUTO)
  {
    control_drive_pump();
    control_drive_fan();
  }

  control_drive_alarm();
  control_update_indicators();
  control_log_status();
}

/**
 * @brief 设置控制模式（自动/手动）
 * 
 * @param mode 要设置的控制模式
 * @return 无返回值
 */
void control_set_mode(ctrl_mode_t mode)
{
  if (mode == g_status.mode)
  {
    return;
  }

  g_status.mode = mode;

  if (mode == CTRL_MODE_MANUAL)
  {
    relay_off(RELAY_PUMP);
    relay_off(RELAY_FAN);
    g_status.pump_running = 0U;
    g_status.fan_running  = 0U;
    g_status.alarm_active = 0U;
    beep_off();
    debug_uart_send_line("[CTRL] Switched to MANUAL mode");
  }
  else
  {
    debug_uart_send_line("[CTRL] Switched to AUTO mode");
  }
}

/**
 * @brief 获取当前控制模式
 * 
 * @return ctrl_mode_t 当前的控制模式
 */
ctrl_mode_t control_get_mode(void)
{
  return g_status.mode;
}

/**
 * @brief 获取当前控制状态
 * 
 * @return const ctrl_status_t* 指向当前控制状态的常量指针
 */
const ctrl_status_t *control_get_status(void)
{
  return &g_status;
}