#include "soil.h"
#include "adc.h"
#include "debug_uart.h"

extern ADC_HandleTypeDef hadc1;

#define SOIL_DEMO_PERIOD_MS  1000U
#define SOIL_AVG_SAMPLES     8U

static uint32_t g_soil_demo_tick = 0U;

void soil_init(void)
{
  g_soil_demo_tick = HAL_GetTick();
}

uint16_t soil_read_raw(void)
{
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, 10);
  uint16_t val = (uint16_t)HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);
  return val;
}

uint16_t soil_read_avg(uint8_t samples)
{
  if (samples == 0U) samples = 1U;
  if (samples > 16U) samples = 16U;

  uint16_t buf[16];
  uint8_t i, j;

  for (i = 0U; i < samples; i++)
  {
    buf[i] = soil_read_raw();
  }

  /* 冒泡排序取中位数，过滤异常跳变 */
  for (i = 0U; i < samples - 1U; i++)
  {
    for (j = 0U; j < samples - i - 1U; j++)
    {
      if (buf[j] > buf[j + 1U])
      {
        uint16_t tmp = buf[j];
        buf[j] = buf[j + 1U];
        buf[j + 1U] = tmp;
      }
    }
  }

  return buf[samples / 2U];  /* 返回中位数 */
}

uint8_t soil_adc_to_percent(uint16_t adc_val)
{
  if (adc_val >= 4095U) return 0U;
  if (adc_val == 0U) return 100U;
  uint16_t pct = (4095U - adc_val) * 100U / 4095U;
  return (uint8_t)(pct > 100U ? 100U : pct);
}

void soil_stage6_demo(void)
{
  uint32_t now = HAL_GetTick();
  if ((now - g_soil_demo_tick) < SOIL_DEMO_PERIOD_MS)
  {
    return;
  }
  g_soil_demo_tick = now;

  uint16_t raw = soil_read_raw();
  uint16_t avg = soil_read_avg(SOIL_AVG_SAMPLES);

  debug_uart_send_string("SOIL raw=");
  debug_uart_send_number((int32_t)raw);
  debug_uart_send_string(" avg=");
  debug_uart_send_number((int32_t)avg);
  debug_uart_send_line("");
}