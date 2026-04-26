#include "exti_demo.h"

#include "led.h"

#define EXTI_DEMO_BLINK_PERIOD_MS    500U
#define EXTI_DEMO_DEBOUNCE_TIME_MS   30U

static volatile uint8_t g_key1_exti_pressed_flag = 0U;
static uint32_t g_key1_last_irq_tick = 0U;
static uint8_t g_key1_override_active = 0U;
static uint8_t g_key1_release_pending = 0U;
static uint32_t g_key1_release_tick = 0U;
static led_id_t g_rb_active_led = LED_RED;
static led_id_t g_rb_saved_led = LED_RED;
static uint32_t g_rb_last_toggle_tick = 0U;

static void exti_demo_set_red_blue_state(led_id_t active_led)
{
  if (active_led == LED_RED)
  {
    led_on(LED_RED);
    led_off(LED_BLUE);
  }
  else
  {
    led_on(LED_BLUE);
    led_off(LED_RED);
  }
}

static void exti_demo_update_red_blue_blink(void)
{
  uint32_t now;

  if (g_key1_override_active != 0U)
  {
    return;
  }

  now = HAL_GetTick();
  if ((now - g_rb_last_toggle_tick) < EXTI_DEMO_BLINK_PERIOD_MS)
  {
    return;
  }

  g_rb_last_toggle_tick = now;
  if (g_rb_active_led == LED_RED)
  {
    g_rb_active_led = LED_BLUE;
  }
  else
  {
    g_rb_active_led = LED_RED;
  }

  exti_demo_set_red_blue_state(g_rb_active_led);
}

static void exti_demo_handle_key1_press(void)
{
  if (g_key1_exti_pressed_flag == 0U)
  {
    return;
  }

  g_key1_exti_pressed_flag = 0U;

  if (g_key1_override_active != 0U)
  {
    return;
  }

  if (HAL_GPIO_ReadPin(KEY_1_GPIO_Port, KEY_1_Pin) != GPIO_PIN_RESET)
  {
    return;
  }

  g_rb_saved_led = g_rb_active_led;
  g_key1_override_active = 1U;
  g_key1_release_pending = 0U;

  led_off(LED_RED);
  led_off(LED_BLUE);
  led_on(LED_GREEN);
}

static void exti_demo_handle_key1_release(void)
{
  uint32_t now;

  if (g_key1_override_active == 0U)
  {
    return;
  }

  if (HAL_GPIO_ReadPin(KEY_1_GPIO_Port, KEY_1_Pin) == GPIO_PIN_RESET)
  {
    g_key1_release_pending = 0U;
    return;
  }

  now = HAL_GetTick();
  if (g_key1_release_pending == 0U)
  {
    g_key1_release_pending = 1U;
    g_key1_release_tick = now;
    return;
  }

  if ((now - g_key1_release_tick) < EXTI_DEMO_DEBOUNCE_TIME_MS)
  {
    return;
  }

  g_key1_release_pending = 0U;
  g_key1_override_active = 0U;
  led_off(LED_GREEN);
  g_rb_active_led = g_rb_saved_led;
  exti_demo_set_red_blue_state(g_rb_active_led);
  g_rb_last_toggle_tick = now;
}

void exti_demo_init(void)
{
  g_key1_exti_pressed_flag = 0U;
  g_key1_last_irq_tick = 0U;
  g_key1_override_active = 0U;
  g_key1_release_pending = 0U;
  g_key1_release_tick = 0U;
  g_rb_active_led = LED_RED;
  g_rb_saved_led = LED_RED;

  led_init();
  exti_demo_set_red_blue_state(g_rb_active_led);
  g_rb_last_toggle_tick = HAL_GetTick();
}

void exti_demo_task(void)
{
  exti_demo_handle_key1_press();
  exti_demo_handle_key1_release();
  exti_demo_update_red_blue_blink();
}

void exti_demo_gpio_exti_callback(uint16_t gpio_pin)
{
  uint32_t now;

  if (gpio_pin != KEY_1_Pin)
  {
    return;
  }

  now = HAL_GetTick();
  if ((now - g_key1_last_irq_tick) < EXTI_DEMO_DEBOUNCE_TIME_MS)
  {
    return;
  }

  g_key1_last_irq_tick = now;
  g_key1_exti_pressed_flag = 1U;
}
