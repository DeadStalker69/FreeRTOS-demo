#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <semphr.h>

static const int led1 = 0;    // GPIO 0 for Red LED
static const int led2 = 1;    // GPIO 1 for Green LED
static const int button1 = 2; // GPIO 2 for Button1
static const int button2 = 3; // GPIO 3 for Button2

bool led1_state = false;      //used to track status of LED1
bool led2_state = false;      //used to track status of LED2
bool led1_previous_state = false;   //used to compare current state of LED1 to previous state to note if any change have occured. (Used by lcd function to know if LED state have changed and change the LCD print statment, so that LCD do not refresh continously and only when any change have occured.)
bool led2_previous_state = false;   //used to compare current state of LED2 to previous state.
bool blinking = false;        //used to know if no button is pressed(used because mutex does not allow LCD function to note changes while LEDs are in blinking state).

SemaphoreHandle_t mutex;      

LiquidCrystal_I2C lcd(0x27, 16, 2); 

void Toggle_LED(void *parameter) {
  while (1) {
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {   //Taking the mutex
      if (!digitalRead(button1) && !digitalRead(button2)) {   //checking for flase as internal pullup resistor is used and the buttons are high when in not pressed state.
        // Both buttons pressed: maintain current LED states
        digitalWrite(led1, led1_state ? HIGH : LOW);
        digitalWrite(led2, led2_state ? HIGH : LOW);
        blinking = false;
      } else if (!digitalRead(button1)) {
        // Button1 pressed: Turn on LED1 and turn off LED2
        digitalWrite(led1, HIGH);
        digitalWrite(led2, LOW);
        led1_previous_state = led1_state;
        led1_state = true;
        led2_previous_state = led2_state;
        led2_state = false;
        blinking = false;
      } else if (!digitalRead(button2)) {
        // Button2 pressed: Turn on LED2 and turn off LED1
        digitalWrite(led2, HIGH);
        digitalWrite(led1, LOW);
        led1_previous_state = led1_state;
        led1_state = false;
        led2_previous_state = led2_state;
        led2_state = true;
        blinking = false;
      } else {
        // No buttons pressed: Alternate LEDs
        digitalWrite(led1, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200));  
        digitalWrite(led1, LOW);
        digitalWrite(led2, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200));     //used 200ms blinking pattern as required by the task.
        digitalWrite(led2, LOW);
        blinking = true;
      }
      xSemaphoreGive(mutex);
    }

    vTaskDelay(pdMS_TO_TICKS(100));     //Fulfilling task requirement of reading input every 100ms
  }
}

void LCD_Write(void *parameter) {
  while (1) {
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
      if(blinking) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("LEDs are");
        lcd.setCursor(3,1);
        lcd.print("blinking");
      }
      else if (led1_previous_state != led1_state || led2_previous_state != led2_state) {    //changing the output at LCD when any one of the LED's state is changed.
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("RED LED: ");
        lcd.print(led1_state ? "ON " : "OFF");
  
        lcd.setCursor(0, 1);
        lcd.print("GREEN LED: ");
        lcd.print(led2_state ? "ON " : "OFF");
      }
      xSemaphoreGive(mutex);
    }

    vTaskDelay(pdMS_TO_TICKS(100));  
  }
}

void setup() {
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(button1, INPUT_PULLUP); 
  pinMode(button2, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  mutex = xSemaphoreCreateMutex();

  xTaskCreate(Toggle_LED, "Toggle LED", 1024, NULL, 1, NULL);
  xTaskCreate(LCD_Write, "LCD Write", 1024, NULL, 1, NULL);
}

void loop() {
  // Empty loop, tasks run independently
}
