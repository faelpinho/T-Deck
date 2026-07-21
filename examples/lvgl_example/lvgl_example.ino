#include <TFT_eSPI.h>
#include <lvgl.h>
#include <Arduino.h>
#include "TouchDrvGT911.hpp"
#include "utilities.h"

TFT_eSPI        tft;
TouchDrvGT911 touch;

// LilyGo  T-Deck  control backlight chip has 16 levels of adjustment range
// The adjustable range is 0~15, 0 is the minimum brightness, 15 is the maximum brightness
void setBrightness(uint8_t value)
{
    static uint8_t level = 0;
    static uint8_t steps = 16;
    if (value == 0) {
        digitalWrite(BOARD_BL_PIN, 0);
        delay(3);
        level = 0;
        return;
    }
    if (level == 0) {
        digitalWrite(BOARD_BL_PIN, 1);
        level = steps;
        delayMicroseconds(30);
    }
    int from = steps - level;
    int to = steps - value;
    int num = (steps + to - from) % steps;
    for (int i = 0; i < num; i++) {
        digitalWrite(BOARD_BL_PIN, 0);
        digitalWrite(BOARD_BL_PIN, 1);
    }
    level = value;
}


// !!! LVGL !!!
// !!! LVGL !!!
// !!! LVGL !!!
static void disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );
    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )&color_p->full, w * h, false );
    tft.endWrite();
    lv_disp_flush_ready( disp );
}

int16_t x[5], y[5];



/*Read the touchpad*/
static void touchpad_read( lv_indev_drv_t *indev_driver, lv_indev_data_t *data )
{
    data->state =  LV_INDEV_STATE_REL;

    // data->state = getTouch(data->point.x, data->point.y) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    if (touch.isPressed()) {
        Serial.println("Pressed!");
        uint8_t touched = touch.getPoint(x, y, touch.getSupportTouchPoint());
        if (touched > 0) {
            data->state = LV_INDEV_STATE_PR;
            data->point.x = x[0];
            data->point.y = y[0];

            Serial.print(millis());
            Serial.print("ms ");
            for (int i = 0; i < touched; ++i) {
                Serial.print("X[");
                Serial.print(i);
                Serial.print("]:");
                Serial.print(x[i]);
                Serial.print(" ");
                Serial.print(" Y[");
                Serial.print(i);
                Serial.print("]:");
                Serial.print(y[i]);
                Serial.print(" ");
            }
            Serial.println();
        }
    }
}


void setupLvgl()
{
#define LVGL_BUFFER_SIZE            (TFT_WIDTH * TFT_HEIGHT * sizeof(lv_color_t))

    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t *buf = (lv_color_t *)ps_malloc(LVGL_BUFFER_SIZE);
    if (!buf) {
        Serial.println("menory alloc failed!");
        delay(5000);
        assert(buf);
    }
    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
    Serial.println( LVGL_Arduino );
    Serial.println( "I am LVGL_Arduino" );

    lv_init();

    lv_group_set_default(lv_group_create());

    lv_disp_draw_buf_init( &draw_buf, buf, NULL, LVGL_BUFFER_SIZE );

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );

    /*Change the following line to your display resolution*/
    disp_drv.hor_res = TFT_HEIGHT;
    disp_drv.ver_res = TFT_WIDTH;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.full_refresh = 1;
    lv_disp_drv_register( &disp_drv );

    /*Initialize the  input device driver*/

    /*Register a touchscreen input device*/
    static lv_indev_drv_t indev_touchpad;
    lv_indev_drv_init( &indev_touchpad );
    indev_touchpad.type = LV_INDEV_TYPE_POINTER;
    indev_touchpad.read_cb = touchpad_read;
    lv_indev_drv_register( &indev_touchpad );
}

void setup()
{
    Serial.begin(115200);

    Serial.println("lvgl example");

    //! The board peripheral power control pin needs to be set to HIGH when using the peripheral
    pinMode(BOARD_POWERON, OUTPUT);
    digitalWrite(BOARD_POWERON, HIGH);

    //! Set CS on all SPI buses to high level during initialization
    pinMode(BOARD_SDCARD_CS, OUTPUT);
    pinMode(RADIO_CS_PIN, OUTPUT);
    pinMode(BOARD_TFT_CS, OUTPUT);

    digitalWrite(BOARD_SDCARD_CS, HIGH);
    digitalWrite(RADIO_CS_PIN, HIGH);
    digitalWrite(BOARD_TFT_CS, HIGH);

    pinMode(BOARD_SPI_MISO, INPUT_PULLUP);
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); //SD

    pinMode(BOARD_BOOT_PIN, INPUT_PULLUP);
    pinMode(BOARD_TBOX_G02, INPUT_PULLUP);
    pinMode(BOARD_TBOX_G01, INPUT_PULLUP);
    pinMode(BOARD_TBOX_G04, INPUT_PULLUP);
    pinMode(BOARD_TBOX_G03, INPUT_PULLUP);

    Serial.print("Init display id:");
    Serial.println(USER_SETUP_ID);

    tft.begin();
    tft.setRotation( 1 );
    tft.fillScreen(TFT_BLACK);

    // Set touch int input
    pinMode(BOARD_TOUCH_INT, INPUT);
    delay(20);

    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);


    touch.setPins(-1, BOARD_TOUCH_INT);
    if (!touch.begin(Wire, GT911_SLAVE_ADDRESS_L)) {
        while (1) {
            Serial.println("Failed to find GT911 - check your wiring!");
            delay(1000);
        }
    }

    Serial.println("Init GT911 Sensor success!");

    // Set touch max xy
    touch.setMaxCoordinates(320, 240);

    // Set swap xy
    touch.setSwapXY(true);

    // Set mirror xy
    touch.setMirrorXY(false, true);


    setupLvgl();

    const int offset = 0;

    struct align_str {
        int16_t x;
        int16_t y;
        lv_align_t align;

    } tmp[] = {

        {0, 0, LV_ALIGN_TOP_LEFT},
        {0, 0, LV_ALIGN_TOP_MID},
        {0, 0, LV_ALIGN_TOP_RIGHT},

        {0, offset, LV_ALIGN_TOP_LEFT},
        {0, offset, LV_ALIGN_TOP_MID},
        {0, offset, LV_ALIGN_TOP_RIGHT},

        {0, 0, LV_ALIGN_LEFT_MID},
        {0, 0, LV_ALIGN_CENTER},
        {0, 0, LV_ALIGN_RIGHT_MID},

        {0, offset, LV_ALIGN_LEFT_MID},
        {0, offset, LV_ALIGN_CENTER},
        {0, offset, LV_ALIGN_RIGHT_MID},

        {0, 0, LV_ALIGN_BOTTOM_LEFT},
        {0, 0, LV_ALIGN_BOTTOM_MID},
        {0, 0, LV_ALIGN_BOTTOM_RIGHT},
    };

    // Test touch boundaries
    for (int i = 0; i < sizeof(tmp) / sizeof(tmp[0]); ++i) {
        lv_obj_t *btn1 = lv_btn_create(lv_scr_act());
        lv_obj_set_width(btn1, 60);
        lv_obj_add_flag(btn1, LV_OBJ_FLAG_CHECKABLE);
        // lv_obj_add_event_cb(btn1, border_touch_event_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_align(btn1, tmp[i].align, tmp[i].x, tmp[i].y);
        lv_obj_t *label = lv_label_create(btn1);
        // lv_obj_set_style_text_font(label, &lv_font_montserrat_20, LV_PART_MAIN);
        lv_label_set_text_fmt(label, "%d", i);
        lv_obj_center(label);
    }

    // Adjust backlight
    pinMode(BOARD_BL_PIN, OUTPUT);
    setBrightness(16);
}

void loop()
{
    lv_timer_handler();
    delay(5);
}