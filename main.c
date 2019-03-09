// チャックプログラム

#include "sensit_types.h"
#include "sensit_api.h"
#include "error.h"
#include "button.h"
#include "battery.h"
#include "radio_api.h"
#include "hts221.h"
#include "ltr329.h"
#include "fxos8700.h"


// GLOBAL VARIABLES
u8 firmware_version[] = "TEMPLATE";
bool power_on = TRUE;
int magoff_count = 0;


void AccelerometerInitialize(){
    FXOS8700_reset();
    FXOS8700_set_transient_mode(FXOS8700_RANGE_2G, 1, 2);
}

// LEDを１秒だけ点灯
void led(rgb_color_e color) {
    SENSIT_API_set_rgb_led(color);
    if (color != RGB_OFF) {
        SENSIT_API_wait(1000);         // １秒待つ
        SENSIT_API_set_rgb_led(RGB_OFF);
    }
}

// LEDを指定した回数点滅
void blink(rgb_color_e color, int count, int interval) {
    for (int i = 0; i < count; i++) {
        SENSIT_API_set_rgb_led(color);
        if (color != RGB_OFF) {
            SENSIT_API_wait(interval);
            SENSIT_API_set_rgb_led(RGB_OFF);
            SENSIT_API_wait(interval);
        }
    }
}

// ボタンの処理
void ButtonInterrupt(){
    button_e btn;
    btn = BUTTON_handler();
    switch (btn) {
    case BUTTON_LONG_PRESS:
        if (power_on) {
            // 電源OFFの処理
            power_on = FALSE;
            SENSIT_API_set_rtc_alarm(60*60*24);     // タイマーOFF
            blink(RGB_MAGENTA, 8, 100);
        } else {
            // 電源ONの処理
            power_on = TRUE;
            blink(RGB_MAGENTA, 3, 100);
            AccelerometerInitialize();              // 加速度センサー初期化
        }
        break;
    case BUTTON_ONE_PRESS:
        blink(RGB_WHITE, 1, 100);
        // 磁石が付いた
        magoff_count = 0;
        SENSIT_API_set_rtc_alarm(60*60*24);
        break;
    case BUTTON_TWO_PRESSES:
        blink(RGB_RED, 2, 100);
        break;
    case BUTTON_THREE_PRESSES:
        blink(RGB_YELLOW, 3, 100);
        // 磁石が離れた
        magoff_count = 1;
        SENSIT_API_set_rtc_alarm(10);
        break;
    case BUTTON_FOUR_PRESSES:
        blink(RGB_WHITE, 4, 100);
        SENSIT_API_reset();        // Reset the device
        break;
    default:
        break;
    }
}

void AlarmInterrupt(){
    u8 payload = 0;
    ++magoff_count;
    if (magoff_count < 8) {
        blink(RGB_YELLOW, 1, 100);
    } else if (magoff_count > 17) {
        magoff_count = 0;
        SENSIT_API_set_rtc_alarm(60*60*24);
        // 送信（ペイロードは適当）
        RADIO_API_send_message(RGB_RED, &payload, 1, FALSE, NULL);
    } else {
        blink(RGB_RED, 1, 100);
    }
    if (magoff_count == 2) {
        SENSIT_API_set_rtc_alarm(2);
    } else if (magoff_count == 7) {
        SENSIT_API_set_rtc_alarm(1);
    }
}

void ReedSwitchInterrupt(){
    bool magnet;
    // Get reed switch state
    SENSIT_API_get_reed_switch_state(&magnet);
    if (magnet) {
        blink(RGB_YELLOW, 3, 100);
    } else {
        blink(RGB_RED, 3, 100);
    }
}

void AccelerometerInterrupt(){
    bool flag;
    FXOS8700_clear_transient_interrupt(&flag);   // Read transient interrupt register
    if (flag == TRUE) {
        blink(RGB_BLUE, 5, 100);
    } else {
        blink(RGB_RED, 5, 100);
    }
    AccelerometerInitialize();
}

int main()
{
    error_t err;
    u16 battery_level;

    // Configure button
    SENSIT_API_configure_button(INTERRUPT_BOTH_EGDE);

    // Initialize Sens'it radio
    err = RADIO_API_init();
    ERROR_parser(err);

    // Initialize temperature & humidity sensor
    err = HTS221_init();
    ERROR_parser(err);

    // Initialize light sensor
    err = LTR329_init();
    ERROR_parser(err);

    // Initialize accelerometer
    err = FXOS8700_init();
    ERROR_parser(err);

    // Put accelerometer in transient mode
    AccelerometerInitialize();

    // タイマーはセットしない
    SENSIT_API_set_rtc_alarm(60*60*24);

    // Clear pending interrupt
    pending_interrupt = 0;

    while (TRUE) {
        BATTERY_handler(&battery_level);       // Check of battery level

        // RTC alarm interrupt handler */
        if ((pending_interrupt & INTERRUPT_MASK_RTC) == INTERRUPT_MASK_RTC){
            if (power_on) {
                AlarmInterrupt();
            }
            pending_interrupt &= ~INTERRUPT_MASK_RTC;            // Clear interrupt
        }

        // Button interrupt handler
        if ((pending_interrupt & INTERRUPT_MASK_BUTTON) == INTERRUPT_MASK_BUTTON) {
            ButtonInterrupt();
            pending_interrupt &= ~INTERRUPT_MASK_BUTTON;            // Clear interrupt
        }

        // Reed switch interrupt handler
        if ((pending_interrupt & INTERRUPT_MASK_REED_SWITCH) == INTERRUPT_MASK_REED_SWITCH) {
            blink(RGB_YELLOW, 2, 100);
//            ReedSwitchInterrupt();
            pending_interrupt &= ~INTERRUPT_MASK_REED_SWITCH;            // Clear interrupt
        }

        // Accelerometer interrupt handler
        if ((pending_interrupt & INTERRUPT_MASK_FXOS8700) == INTERRUPT_MASK_FXOS8700) {
            if (power_on) {
                AccelerometerInterrupt();            
            }
            pending_interrupt &= ~INTERRUPT_MASK_FXOS8700;            // Clear interrupt
        }

        // Check if all interrupt have been clear
        if (pending_interrupt == 0) {
            SENSIT_API_sleep(FALSE);            // Wait for Interrupt
        }
    } /* End of while */
}


