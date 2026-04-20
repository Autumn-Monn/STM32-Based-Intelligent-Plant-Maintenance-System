#include "led.h"

typedef struct
{
  GPIO_TypeDef *port;
  uint16_t pin;
  GPIO_PinState on_state;
  GPIO_PinState off_state;
} led_hw_t;

static const led_hw_t g_led_map[LED_COUNT] = {
    [LED_BOARD] = {LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET, GPIO_PIN_SET},
    [LED_GREEN] = {LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET, GPIO_PIN_RESET},
    [LED_RED] = {LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET, GPIO_PIN_RESET},
    [LED_BLUE] = {LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET, GPIO_PIN_RESET},
};

/**
 * @brief 写入指定LED的状态
 * 
 * @param led LED灯的ID，类型为led_id_t
 * @param state 要设置的GPIO引脚状态
 * @return 无返回值
 */
static void led_write(led_id_t led, GPIO_PinState state)
{
  if (led >= LED_COUNT)
  {
    return;
  }

  HAL_GPIO_WritePin(g_led_map[led].port, g_led_map[led].pin, state);
}

/**
 * @brief 初始化LED模块，将所有LED设置为关闭状态
 * 
 * @return 无返回值
 */
void led_init(void)
{
  led_id_t led;

  for (led = LED_BOARD; led < LED_COUNT; led++)
  {
    led_off(led);
  }
}

/**
 * @brief 打开指定的LED灯
 * 
 * @param led LED灯的ID，类型为led_id_t
 * @return 无返回值
 */
void led_on(led_id_t led)
{
  if (led >= LED_COUNT)
  {
    return;
  }

  led_write(led, g_led_map[led].on_state);
}

/**
 * @brief 关闭指定的LED灯
 * 
 * @param led LED灯的ID，类型为led_id_t
 * @return 无返回值
 */
void led_off(led_id_t led)
{
  if (led >= LED_COUNT)
  {
    return;
  }

  led_write(led, g_led_map[led].off_state);
}

/**
 * @brief 切换指定LED灯的状态（如果当前是开启则关闭，如果是关闭则开启）
 * 
 * @param led LED灯的ID，类型为led_id_t
 * @return 无返回值
 */
void led_toggle(led_id_t led)
{
  if (led >= LED_COUNT)
  {
    return;
  }

  if (HAL_GPIO_ReadPin(g_led_map[led].port, g_led_map[led].pin) == g_led_map[led].on_state)
  {
    led_off(led);
  }
  else
  {
    led_on(led);
  }
}

/**
 * @brief LED阶段一演示函数，依次点亮板载LED、绿灯、红灯、蓝灯并闪烁
 * 
 * @return 无返回值
 */
void led_stage1_demo(void)
{
  led_on(LED_BOARD);
  HAL_Delay(500);
  led_off(LED_BOARD);
  HAL_Delay(500);

  led_on(LED_GREEN);
  HAL_Delay(500);
  led_off(LED_GREEN);
  HAL_Delay(500);

  led_on(LED_RED);
  HAL_Delay(500);
  led_off(LED_RED);
  HAL_Delay(500);

  led_on(LED_BLUE);
  HAL_Delay(500);
  led_off(LED_BLUE);
  HAL_Delay(500);
}
