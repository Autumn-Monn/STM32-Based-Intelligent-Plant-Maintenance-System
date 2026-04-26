#include "beep.h"

#define BEEP_STAGE4A_PERIOD_MS  500U

static uint32_t g_beep_last_toggle_tick = 0U;

void beep_on(void)
{
  HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
}

void beep_off(void)
{
  HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
}

void beep_toggle(void)
{
  HAL_GPIO_TogglePin(BEEP_GPIO_Port, BEEP_Pin);
}

void beep_init(void)
{
  beep_off();
  g_beep_last_toggle_tick = HAL_GetTick();
}

void beep_stage4a_demo(void)
{
  uint32_t now;

  now = HAL_GetTick();
  if ((now - g_beep_last_toggle_tick) < BEEP_STAGE4A_PERIOD_MS)
  {
    return;
  }

  g_beep_last_toggle_tick = now;
  beep_toggle();
}
