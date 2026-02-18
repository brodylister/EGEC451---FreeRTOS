// basic_tasks.ino
// MIT License (see file LICENSE)

// Change the following if you want to use
// different GPIO pins for the three LEDs

#define LED1 12  // GPIO 12
#define LED2 13  // etc.
#define LED3 15

struct s_led {
  byte gpio;           // LED GPIO number
  byte state;          // LED state
  unsigned napms;      // Delay to use (ms)
  TaskHandle_t taskh;  // Task handle
};

static s_led leds[2] = {
  { LED1, 0, 20, 0 },
  { LED2, 0, 100, 0 },
};

static void led_task_func_red(void *argp) {
  s_led *ledp = (s_led *)argp;
  unsigned stack_hwm = 0, temp;

  delay (1000);
  
  for (;;) {
    digitalWrite(ledp->gpio, ledp->state = 1);      // on at start of task
    temp = uxTaskGetStackHighWaterMark(nullptr);
    if (!stack_hwm || temp < stack_hwm) {
      stack_hwm = temp;
      printf("Task for gpio %d has stack hwm %u\n",
             ledp->gpio, stack_hwm);
    }
    delay(ledp->napms);
    digitalWrite(ledp->gpio, ledp->state = 0);      // off after napms
    taskYIELD();                                    // then yield
  }
}

static void led_task_func_green(void *argp) {
  s_led *ledp = (s_led *)argp;
  unsigned stack_hwm = 0, temp;

  delay(1000);

  for (;;) {
    digitalWrite(ledp->gpio, ledp->state ^= 1);     // forever toggles after napms
    temp = uxTaskGetStackHighWaterMark(nullptr);
    if (!stack_hwm || temp < stack_hwm) {
      stack_hwm = temp;
      printf("Task for gpio %d has stack hwm %u\n",
             ledp->gpio, stack_hwm);
    }
    delay(ledp->napms);
  }
}

void setup() {
  int app_cpu = 0;  // CPU number

  delay(500);  // Pause for serial setup

  app_cpu = xPortGetCoreID();
  printf("app_cpu is %d (%s core)\n",
         app_cpu,
         app_cpu > 0 ? "Dual" : "Single");

  printf("LEDs on gpios: ");

  pinMode(leds[1].gpio, OUTPUT);
  digitalWrite(leds[1].gpio, LOW);
  xTaskCreatePinnedToCore(            // red task
    led_task_func_red,
    "led_task",
    2048,
    &leds[1],
    1,
    &leds[1].taskh,
    app_cpu);
  printf("%d ", leds[1].gpio);
  
  pinMode(leds[2].gpio, OUTPUT);
  digitalWrite(leds[2].gpio, LOW);
  xTaskCreatePinnedToCore(            // green task
    led_task_func_green,
    "led_task",
    2048,
    &leds[2],
    1,
    &leds[2].taskh,
    app_cpu);
  printf("%d ", leds[2].gpio);

  putchar('\n');
}

void loop() {
  delay(1000);
}
