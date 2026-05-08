/**
 * Stage 9 demo: sensor-actuator closed-loop control.
 *
 * Builds on stages 1-8. Integrates soil moisture + DS18B20 temperature
 * readings with pump/fan relay control, LED status, and buzzer alarm.
 *
 * Integration guide (main.c USER CODE blocks):
 *
 * 1) Add includes:
 *      #include "stage9_demo.h"
 *
 * 2) In USER CODE BEGIN 2 (after other _init calls):
 *      stage9_demo_init();
 *
 * 3) In main while(1) loop (USER CODE BEGIN 3):
 *      stage9_demo_task();
 *
 * Key bindings (during runtime):
 *   K1 — Toggle auto/manual mode
 *   K2 — (Manual) Toggle pump
 *   K3 — (Manual) Toggle fan
 *   K4 — Clear alarm
 *
 * Serial commands:
 *   'a' — Switch to AUTO mode
 *   'm' — Switch to MANUAL mode
 *   'p' — (Manual) Toggle pump
 *   'f' — (Manual) Toggle fan
 *   'c' — Clear alarm
 *   's' — Print current status
 */

#include "stage9_demo.h"
#include "i2c_test.h"
#include "ds18b20.h"
#include "control.h"
#include "soil.h"
#include "key.h"
#include "relay.h"
#include "beep.h"
#include "debug_uart.h"

static void handle_key_events(void)
{
  key_event_t evt = key_get_event();

  switch (evt)
  {
    case KEY_EVENT_1_PRESSED:
    {
      ctrl_mode_t cur = control_get_mode();
      control_set_mode(cur == CTRL_MODE_AUTO ? CTRL_MODE_MANUAL : CTRL_MODE_AUTO);
      break;
    }

    case KEY_EVENT_2_PRESSED:
      if (control_get_mode() == CTRL_MODE_MANUAL)
      {
        relay_toggle(RELAY_PUMP);
        debug_uart_send_line("[KEY] Manual pump toggle");
      }
      break;

    case KEY_EVENT_3_PRESSED:
      if (control_get_mode() == CTRL_MODE_MANUAL)
      {
        relay_toggle(RELAY_FAN);
        debug_uart_send_line("[KEY] Manual fan toggle");
      }
      break;

    case KEY_EVENT_4_PRESSED:
      beep_off();
      debug_uart_send_line("[KEY] Alarm cleared");
      break;

    default:
      break;
  }
}

static void handle_serial_commands(void)
{
  while (debug_uart_has_data())
  {
    uint8_t cmd = debug_uart_read_byte();
    switch (cmd)
    {
      case 'a':
        control_set_mode(CTRL_MODE_AUTO);
        break;

      case 'm':
        control_set_mode(CTRL_MODE_MANUAL);
        break;

      case 'p':
        if (control_get_mode() == CTRL_MODE_MANUAL)
        {
          relay_toggle(RELAY_PUMP);
          debug_uart_send_line("[CMD] Manual pump toggle");
        }
        else
        {
          debug_uart_send_line("[CMD] Ignored: in AUTO mode");
        }
        break;

      case 'f':
        if (control_get_mode() == CTRL_MODE_MANUAL)
        {
          relay_toggle(RELAY_FAN);
          debug_uart_send_line("[CMD] Manual fan toggle");
        }
        else
        {
          debug_uart_send_line("[CMD] Ignored: in AUTO mode");
        }
        break;

      case 'c':
        beep_off();
        debug_uart_send_line("[CMD] Alarm cleared");
        break;

      case 's':
      {
        const ctrl_status_t *st = control_get_status();
        debug_uart_send_string("[STATUS] mode=");
        debug_uart_send_string(st->mode == CTRL_MODE_AUTO ? "AUTO" : "MANUAL");
        debug_uart_send_string(" pump=");
        debug_uart_send_string(st->pump_running ? "ON" : "OFF");
        debug_uart_send_string(" fan=");
        debug_uart_send_string(st->fan_running ? "ON" : "OFF");

        debug_uart_send_string(" soil_raw=");
        debug_uart_send_number((int32_t)soil_read_avg(4));

        if (ds18b20_is_valid())
        {
          int16_t raw = ds18b20_get_cached_raw();
          debug_uart_send_string(" temp_raw=");
          debug_uart_send_number((int32_t)raw);
        }
        else
        {
          debug_uart_send_string(" temp=N/A");
        }
        debug_uart_send_line("");
        break;
      }

      default:
        break;
    }
  }
}

void stage9_demo_init(void)
{
  debug_uart_send_line("========================================");
  debug_uart_send_line("  Stage 9 Demo: Closed-Loop Control");
  debug_uart_send_line("========================================");
  debug_uart_send_line("  K1=mode  K2=pump  K3=fan  K4=clr alarm");
  debug_uart_send_line("  Serial: a=auto m=manual p=pump f=fan");
  debug_uart_send_line("          c=clear s=status");
  debug_uart_send_line("========================================");

  i2c_test_init();
  ds18b20_init();
  control_init();

  debug_uart_send_line("[DEMO] Stage9 init complete");
}

void stage9_demo_task(void)
{
  key_scan();
  handle_key_events();
  handle_serial_commands();

  ds18b20_task();
  soil_stage6_demo();
  i2c_test_task();
  control_task();
}