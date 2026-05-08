#include "relay.h"

typedef struct
{
  GPIO_TypeDef *port;
  uint16_t      pin;
  GPIO_PinState on_state;
  GPIO_PinState off_state;
} relay_hw_t;

/* 高电平触发：SET = 吸合，RESET = 断开 */
static const relay_hw_t g_relay_map[RELAY_COUNT] = {
    [RELAY_PUMP] = {RELAY_PUMP_GPIO_Port, RELAY_PUMP_Pin, GPIO_PIN_SET, GPIO_PIN_RESET},
    [RELAY_FAN]  = {RELAY_FAN_GPIO_Port,  RELAY_FAN_Pin,  GPIO_PIN_SET, GPIO_PIN_RESET},
};

#define RELAY_DEMO_PERIOD_MS  2000U

static uint32_t g_relay_demo_tick = 0U;
static uint8_t  g_relay_demo_step = 0U;

void relay_init(void)
{
  relay_id_t id;

  for (id = RELAY_PUMP; id < RELAY_COUNT; id++)
  {
    relay_off(id);
  }
  g_relay_demo_tick = HAL_GetTick();
  g_relay_demo_step = 0U;
}

void relay_on(relay_id_t relay)
{
  if (relay >= RELAY_COUNT)
  {
    return;
  }

  HAL_GPIO_WritePin(g_relay_map[relay].port,
                    g_relay_map[relay].pin,
                    g_relay_map[relay].on_state);
}

void relay_off(relay_id_t relay)
{
  if (relay >= RELAY_COUNT)
  {
    return;
  }

  HAL_GPIO_WritePin(g_relay_map[relay].port,
                    g_relay_map[relay].pin,
                    g_relay_map[relay].off_state);
}

void relay_toggle(relay_id_t relay)
{
  if (relay >= RELAY_COUNT)
  {
    return;
  }

  if (HAL_GPIO_ReadPin(g_relay_map[relay].port,
                       g_relay_map[relay].pin) == g_relay_map[relay].on_state)
  {
    relay_off(relay);
  }
  else
  {
    relay_on(relay);
  }
}

void relay_stage4b_demo(void)
{
  uint32_t now;

  now = HAL_GetTick();
  if ((now - g_relay_demo_tick) < RELAY_DEMO_PERIOD_MS)
  {
    return;
  }

  g_relay_demo_tick = now;

  switch (g_relay_demo_step)
  {
    case 0: relay_on(RELAY_PUMP);  break;
    case 1: relay_off(RELAY_PUMP); break;
    case 2: relay_on(RELAY_FAN);   break;
    case 3: relay_off(RELAY_FAN);  break;
    default: break;
  }

  g_relay_demo_step = (g_relay_demo_step + 1U) % 4U;
}