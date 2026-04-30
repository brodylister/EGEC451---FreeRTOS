/*
  Brody Lister
  EGEC451
  Dr. Turi
  April 29, 2026

Instructions:
    Create a FreeRTOS embedded application with two FreeRTOS tasks, two buttons for input, and two LEDs for output. Each button will be associated with one task 
  and with one LED. A blocking semaphore or mutex (your choice) will govern access to the LEDs.

    When a button is pressed, an interrupt must be triggered. The LED semaphore or mutex must be obtained before toggling the LED. 
  FreeRTOS semaphore or mutex functions must be called by the tasks, not the interrupt handlers/ISRs, since the LED semaphore or mutex is blocking. 
  Therefore, you will need to use a data structure, such as a FreeRTOS queue or mailbox, to communicate from the interrupt handlers/ISRs to the tasks. 
  Also remember that FreeRTOS has separate functions with "fromISR" in their names; these functions are meant to be called by interrupt handlers/ISRs.
  The following describes an example of correct behavior:

  Task 1's button is pressed, and once Task 1 runs, it will obtain the LED semaphore or mutex and turn on Task 1 LED. Task 1 will continue to possess the semaphore or mutex while the Task 1 LED is on.
  Task 2's button is pressed, and once Task 2 runs, it will try to obtain the LED semaphore or mutex and be blocked.
  Task 1's button is pressed again, and once Task 1 runs, it will turn off Task 1 LED, since it already has the LED semaphore or mutex. It will then give up the semaphore or mutex.
  Task 2 should now be unblocked, and once Task 2 runs, it will have the LED semaphore or mutex and turn on Task 2 LED. Task 2 will continue to possess the semaphore or mutex while the Task 2 LED is on.
  Steps 2-4 may repeat, with Task 1 being blocked since Task 2 possess the LED semaphore or mutex.


  pseudocode:
  
globals
pins
appcpu
mutex
mailboxes

ISRs
-> send mailbox and exit

Tasks
-> setup
-> block infinitely on mailbox, XOR led control

setup()
pins
isrs
mutex
tasks


loop()

*/

// PINS
#define GPIO_WHITE_LED    12
#define GPIO_BLUE_LED     13
#define GPIO_WHITE_SWITCH 14  // positive logic
#define GPIO_BLUE_SWITCH  15  // positive logic

// GLOBAL VARS
int app_cpu = 0;
static int button_press_obj = 1;
QueueHandle_t       white_mailbox;
QueueHandle_t       blue_mailbox;
SemaphoreHandle_t   led_mutex;
TaskHandle_t        white_task;
TaskHandle_t        blue_task;

// ISRS

void IRAM_ATTR isr_white() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueOverwriteFromISR(white_mailbox, &button_press_obj, &xHigherPriorityTaskWoken);
//  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void IRAM_ATTR isr_blue() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueOverwriteFromISR(blue_mailbox, &button_press_obj, &xHigherPriorityTaskWoken);
//  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// TASKS
void whiteTask(void* argp) {
  QueueHandle_t white_mailbox = *(QueueHandle_t*)argp;
  int obj = 0;
  int state = 0;
  BaseType_t rc;

  for (;;) {
    rc = xQueueReceive(white_mailbox, &obj, portMAX_DELAY);
    assert(rc == pdPASS);
    assert(obj == 1);
    printf("White received from mailbox...");

    rc = xSemaphoreTake(led_mutex, portMAX_DELAY);
    assert(rc == pdPASS);
    if (state == 0) {
      digitalWrite(GPIO_WHITE_LED, HIGH);
      state = 1;
      printf("turned led ON  and now state = %u\n", state);
    } else {
      digitalWrite(GPIO_WHITE_LED, LOW);
      state = 0;
      printf("turned led OFF and now state = %u\n", state);
    }
    rc = xSemaphoreGive(led_mutex);
    assert(rc == pdPASS);
  }
}

void blueTask(void* argp) {
  QueueHandle_t blue_mailbox = *(QueueHandle_t*)argp;
  int obj = 0;
  int state = 0;
  BaseType_t rc;

  for (;;) {
    rc = xQueueReceive(blue_mailbox, &obj, portMAX_DELAY);
    assert(rc == pdPASS);
    assert(obj == 1);
    printf("Blue received from mailbox...");

    rc = xSemaphoreTake(led_mutex, portMAX_DELAY);
    if (state == 0) {
      digitalWrite(GPIO_BLUE_LED, HIGH);
      state = 1;
      printf("turned led ON  and now state = %u\n", state);
    } else {
      digitalWrite(GPIO_BLUE_LED, LOW);
      state = 0;
      printf("turned led OFF and now state = %u\n", state);
    }
    rc = xSemaphoreGive(led_mutex);
    assert(rc == pdPASS);
  }
}

void setup() {
  BaseType_t          rc;

  app_cpu = xPortGetCoreID();

  delay(1000);

  pinMode(GPIO_WHITE_LED, OUTPUT);
  digitalWrite(GPIO_WHITE_LED, LOW);
  pinMode(GPIO_BLUE_LED, OUTPUT);
  digitalWrite(GPIO_BLUE_LED, LOW);
  pinMode(GPIO_WHITE_SWITCH, INPUT_PULLDOWN);
  pinMode(GPIO_BLUE_SWITCH, INPUT_PULLDOWN);

  // ISR initialization
  attachInterrupt(GPIO_WHITE_SWITCH, isr_white, RISING);
  attachInterrupt(GPIO_BLUE_SWITCH, isr_blue, RISING);

  // Begin structure initialization
  led_mutex = xSemaphoreCreateMutex();      // create mutex (given)
  assert(led_mutex);
  white_mailbox = xQueueCreate(1, sizeof(button_press_obj));   // create white queue, size 1 int
  assert(white_mailbox);
  blue_mailbox = xQueueCreate(1, sizeof(button_press_obj));    // create blue queue, size 1 int
  assert(blue_mailbox);
  
  rc = xTaskCreatePinnedToCore(
    whiteTask,
    "whiteTask",
    2048,
    &white_mailbox,
    1,
    &white_task,
    app_cpu
  );
  assert(rc == pdPASS);

  xTaskCreatePinnedToCore(
    blueTask,
    "blueTask",
    2048,
    &blue_mailbox,
    1,
    &blue_task,
    app_cpu
  );
}

void loop() {
  vTaskDelete(nullptr);
}
