#include "ds18b20.h"
#include "debug_uart.h"

#define DS18B20_CMD_SKIP_ROM       0xCCU
#define DS18B20_CMD_CONVERT_T      0x44U
#define DS18B20_CMD_READ_SCRATCH   0xBEU

#define DS18B20_TASK_PERIOD_MS     2000U
#define DS18B20_CONVERT_WAIT_MS    750U

typedef enum
{
  DS18B20_STATE_IDLE,
  DS18B20_STATE_CONVERTING
} ds18b20_state_t;

static uint32_t        g_ds18b20_task_tick = 0U;
static ds18b20_state_t g_ds18b20_state     = DS18B20_STATE_IDLE;
static uint32_t        g_convert_start     = 0U;
static int16_t         g_cached_raw        = 0;
static uint8_t         g_data_valid        = 0U;

/* ---------- microsecond delay via DWT cycle counter ---------- */

 /**
 * @brief 初始化DWT周期计数器用于微秒级延时
 * 
 * @return 无返回值
 */
static void dwt_init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0U;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * @brief 微秒级延时函数
 * 
 * @param us 延时的微秒数
 * @return 无返回值
 */
static void delay_us(uint32_t us)
{
  uint32_t start = DWT->CYCCNT;
  uint32_t ticks = us * (SystemCoreClock / 1000000U);
  while ((DWT->CYCCNT - start) < ticks)
  {
  }
}

/* ---------- one-wire GPIO helpers (open-drain throughout) ---------- */

/**
 * @brief 初始化DS18B20使用的GPIO引脚
 * 
 * @return 无返回值
 */
static void ow_gpio_init(void)
{
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin   = DS18B20_GPIO_PIN;
  gpio.Mode  = GPIO_MODE_OUTPUT_OD;
  gpio.Pull  = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(DS18B20_GPIO_PORT, &gpio);
  HAL_GPIO_WritePin(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN, GPIO_PIN_SET);
}

/**
 * @brief 将one-wire总线拉低
 * 
 * @return 无返回值
 */
static void ow_pull_low(void)
{
  HAL_GPIO_WritePin(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN, GPIO_PIN_RESET);
}

/**
 * @brief 释放one-wire总线（允许其被上拉电阻拉高）
 * 
 * @return 无返回值
 */
static void ow_release(void)
{
  HAL_GPIO_WritePin(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN, GPIO_PIN_SET);
}

/**
 * @brief 读取one-wire总线上的电平状态
 * 
 * @return uint8_t 总线电平状态，高电平返回1，低电平返回0
 */
static uint8_t ow_read_pin(void)
{
  return (HAL_GPIO_ReadPin(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN) == GPIO_PIN_SET) ? 1U : 0U;
}

/* ---------- one-wire protocol ---------- */

/**
 * @brief 发送one-wire复位脉冲并等待设备应答
 * 
 * @return ds18b20_status_t 设备状态，成功返回DS18B20_OK，否则返回DS18B20_NO_DEV
 */
static ds18b20_status_t ow_reset(void)
{
  ow_pull_low();
  delay_us(480);

  ow_release();
  delay_us(70);

  uint8_t presence = ow_read_pin();
  delay_us(410);

  if (presence != 0U)
  {
    return DS18B20_NO_DEV;
  }
  return DS18B20_OK;
}

/**
 * @brief 向one-wire总线写入单个比特
 * 
 * @param bit 要写入的比特值（0或1）
 * @return 无返回值
 */
static void ow_write_bit(uint8_t bit)
{
  ow_pull_low();

  if (bit)
  {
    delay_us(6);
    ow_release();
    delay_us(64);
  }
  else
  {
    delay_us(60);
    ow_release();
    delay_us(10);
  }
}

/**
 * @brief 从one-wire总线读取一个比特
 * 
 * @return uint8_t 读取到的比特值（0或1）
 */
static uint8_t ow_read_bit(void)
{
  ow_pull_low();
  delay_us(6);

  ow_release();
  delay_us(9);

  uint8_t val = ow_read_pin();
  delay_us(55);

  return val;
}

/**
 * @brief 向one-wire总线写入一个字节
 * 
 * @param byte 要写入的字节
 * @return 无返回值
 */
static void ow_write_byte(uint8_t byte)
{
  for (uint8_t i = 0U; i < 8U; i++)
  {
    ow_write_bit(byte & 0x01U);
    byte >>= 1;
  }
}

/**
 * @brief 从one-wire总线读取一个字节
 * 
 * @return uint8_t 读取到的字节值
 */
static uint8_t ow_read_byte(void)
{
  uint8_t byte = 0U;
  for (uint8_t i = 0U; i < 8U; i++)
  {
    byte >>= 1;
    if (ow_read_bit())
    {
      byte |= 0x80U;
    }
  }
  return byte;
}

/* ---------- CRC8 (Dallas/Maxim) ---------- */

/**
 * @brief 计算CRC8校验值 (Dallas/Maxim算法)
 * 
 * @param crc 当前CRC值
 * @param data 新加入的数据字节
 * @return uint8_t 更新后的CRC值
 */
static uint8_t crc8_byte(uint8_t crc, uint8_t data)
{
  for (uint8_t i = 0U; i < 8U; i++)
  {
    uint8_t mix = (crc ^ data) & 0x01U;
    crc >>= 1;
    if (mix)
    {
      crc ^= 0x8CU;
    }
    data >>= 1;
  }
  return crc;
}

/* ---------- DS18B20 low-level operations ---------- */

/**
 * @brief 启动DS18B20温度转换
 * 
 * @return ds18b20_status_t 操作状态
 */
static ds18b20_status_t ds18b20_start_convert(void)
{
  ds18b20_status_t st;

  __disable_irq();
  st = ow_reset();
  if (st != DS18B20_OK)
  {
    __enable_irq();
    return st;
  }
  ow_write_byte(DS18B20_CMD_SKIP_ROM);
  ow_write_byte(DS18B20_CMD_CONVERT_T);
  __enable_irq();

  return DS18B20_OK;
}

/**
 * @brief 读取DS18B20暂存器数据并校验CRC
 * 
 * @param raw 指向存储原始温度数据的指针
 * @return ds18b20_status_t 操作状态
 */
static ds18b20_status_t ds18b20_read_scratchpad(int16_t *raw)
{
  ds18b20_status_t st;
  uint8_t scratch[9];
  uint8_t crc = 0U;

  __disable_irq();
  st = ow_reset();
  if (st != DS18B20_OK)
  {
    __enable_irq();
    return st;
  }
  ow_write_byte(DS18B20_CMD_SKIP_ROM);
  ow_write_byte(DS18B20_CMD_READ_SCRATCH);

  for (uint8_t i = 0U; i < 9U; i++)
  {
    scratch[i] = ow_read_byte();
  }
  __enable_irq();

  for (uint8_t i = 0U; i < 8U; i++)
  {
    crc = crc8_byte(crc, scratch[i]);
  }
  if (crc != scratch[8])
  {
    return DS18B20_CRC_ERR;
  }

  *raw = (int16_t)((uint16_t)scratch[1] << 8 | scratch[0]);
  return DS18B20_OK;
}

/* ---------- public API ---------- */

/**
 * @brief 同步读取DS18B20原始温度数据（阻塞式，包含转换等待时间）
 * 
 * @param raw 指向存储原始温度数据的指针
 * @return ds18b20_status_t 操作状态
 */
ds18b20_status_t ds18b20_read_temp_raw(int16_t *raw)
{
  ds18b20_status_t st = ds18b20_start_convert();
  if (st != DS18B20_OK)
  {
    return st;
  }
  HAL_Delay(DS18B20_CONVERT_WAIT_MS);
  return ds18b20_read_scratchpad(raw);
}

/**
 * @brief 同步读取DS18B20浮点温度值（阻塞式）
 * 
 * @param temperature 指向存储浮点温度值的指针
 * @return ds18b20_status_t 操作状态
 */
ds18b20_status_t ds18b20_read_temp(float *temperature)
{
  int16_t raw;
  ds18b20_status_t st = ds18b20_read_temp_raw(&raw);
  if (st != DS18B20_OK)
  {
    return st;
  }
  *temperature = (float)raw / 16.0f;
  return DS18B20_OK;
}

/**
 * @brief 通过UART打印温度数据
 * 
 * @param raw 原始温度数据
 * @return 无返回值
 */
static void ds18b20_print_temp(int16_t raw)
{
  int16_t integer = raw / 16;
  uint16_t frac   = ((uint16_t)(raw > 0 ? raw : -raw) & 0x0FU) * 625U;

  debug_uart_send_string("[DS18] Temp=");
  if (raw < 0 && integer == 0)
  {
    debug_uart_send_string("-");
  }
  debug_uart_send_number((int32_t)integer);
  debug_uart_send_string(".");

  char frac_buf[5];
  frac_buf[0] = '0' + (char)(frac / 1000U);
  frac_buf[1] = '0' + (char)((frac / 100U) % 10U);
  frac_buf[2] = '0' + (char)((frac / 10U) % 10U);
  frac_buf[3] = '0' + (char)(frac % 10U);
  frac_buf[4] = '\0';
  debug_uart_send_string(frac_buf);

  debug_uart_send_string("C raw=");
  debug_uart_send_number((int32_t)raw);
  debug_uart_send_line("");
}

/**
 * @brief 初始化DS18B20模块及相关硬件资源
 * 
 * @return 无返回值
 */
void ds18b20_init(void)
{
  dwt_init();
  g_ds18b20_task_tick = HAL_GetTick();
  g_ds18b20_state = DS18B20_STATE_IDLE;

  __HAL_RCC_GPIOA_CLK_ENABLE();
  ow_gpio_init();

  ds18b20_status_t st = ow_reset();
  if (st == DS18B20_OK)
  {
    debug_uart_send_line("[DS18] DS18B20 detected");
  }
  else
  {
    debug_uart_send_line("[DS18] DS18B20 NOT found!");
  }
}

/**
 * @brief DS18B20非阻塞任务处理函数，需在主循环中周期性调用
 *        采用状态机机制实现异步温度采集
 * 
 * @return 无返回值
 */
void ds18b20_task(void)
{
  uint32_t now = HAL_GetTick();

  switch (g_ds18b20_state)
  {
    case DS18B20_STATE_IDLE:
    {
      if ((now - g_ds18b20_task_tick) < DS18B20_TASK_PERIOD_MS)
      {
        return;
      }

      ds18b20_status_t st = ds18b20_start_convert();
      if (st == DS18B20_OK)
      {
        g_ds18b20_state = DS18B20_STATE_CONVERTING;
        g_convert_start = now;
      }
      else
      {
        debug_uart_send_line("[DS18] No device");
        g_ds18b20_task_tick = now;
      }
      break;
    }

    case DS18B20_STATE_CONVERTING:
    {
      if ((now - g_convert_start) < DS18B20_CONVERT_WAIT_MS)
      {
        return;
      }

      int16_t raw = 0;
      ds18b20_status_t st = ds18b20_read_scratchpad(&raw);

      if (st == DS18B20_OK)
      {
        g_cached_raw = raw;
        g_data_valid = 1U;
        ds18b20_print_temp(raw);
      }
      else if (st == DS18B20_NO_DEV)
      {
        debug_uart_send_line("[DS18] No device");
      }
      else if (st == DS18B20_CRC_ERR)
      {
        debug_uart_send_line("[DS18] CRC error");
      }
      else
      {
        debug_uart_send_line("[DS18] Read error");
      }

      g_ds18b20_state = DS18B20_STATE_IDLE;
      g_ds18b20_task_tick = now;
      break;
    }

    default:
      g_ds18b20_state = DS18B20_STATE_IDLE;
      break;
  }
}

/**
 * @brief 获取最近一次成功读取的缓存原始温度值
 * 
 * @return int16_t 缓存的原始温度值
 */
int16_t ds18b20_get_cached_raw(void)
{
  return g_cached_raw;
}

/**
 * @brief 检查缓存的温度数据是否有效
 * 
 * @return uint8_t 数据有效性标志，1表示有效，0表示无效
 */
uint8_t ds18b20_is_valid(void)
{
  return g_data_valid;
}