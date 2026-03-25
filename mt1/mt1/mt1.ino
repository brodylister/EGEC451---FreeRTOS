/*
Brody Lister, Midterm 1
EGEC451, Dr. Mike Turi
March 25, 2026

# Exam Specification
Create a FreeRTOS embedded application which reads input from a button and displays output to an LED. Your application must behave as follows:

A periodic (auto-reload) timer will periodically read from a button with a period of one second. Once the button's status is read, 
place its status in a queue or mailbox. Note, this behavior should be run from the timer's callback function which is executed by the FreeRTOS daemon task.

A separate FreeRTOS task will read the button's status from the queue or mailbox. If the status read from the queue or mailbox 
indicates that the button is/was pressed, then this task will turn on the LED. If the status read from the queue or mailbox 
indicates that the button is/was not pressed, then this task will turn off the LED. If the queue or mailbox is empty, then this task must block.
*/


#define GPIOLED 12
#define GPIOBUTTON 13

static int wait = 1000;
static StaticTimer_t timer_obj;

QueueHandle_t queue;
TimerHandle_t timer;
TaskHandle_t task;
bool startf = false;

// callback from timer every (wait)ms to check the button:
void timer_cb_checkButton(TimerHandle_t timerHandle) {
  QueueHandle_t queue = *(QueueHandle_t*)pvTimerGetTimerID(timerHandle);
  int msg;
  while (!startf) {
    // do nothing
  }
  msg = digitalRead(GPIOBUTTON);
  xQueueSendToBack(queue, &msg, portMAX_DELAY);   // block forever if full
}

// constantly check the queue to see the state of the button,
// and change the LED accordingly:
void task_updateLED(void* argp) {
  QueueHandle_t handle = *(QueueHandle_t*)argp;
  int msg;
  BaseType_t rc;

  digitalWrite(GPIOLED, LOW);   // led starts off

  while (!startf) {
    // do nothing
  }

  for (;;) {
    rc = xQueueReceive(
      handle,
      &msg,
      portMAX_DELAY       // block forever if empty
    );
    assert(rc == pdPASS);

    if (msg == 0) {
      digitalWrite(GPIOLED, LOW);
    } else if (msg == 1) {
      digitalWrite(GPIOLED, HIGH);
    }
  }
}


// setup the queue, timer, and task, then start the timer:
void setup() {
  // put your setup code here, to run once:
  BaseType_t rc;

  int app_cpu = xPortGetCoreID();

  delay(2000);      // allow usb to connect

  pinMode(GPIOBUTTON, INPUT_PULLDOWN);
  pinMode(GPIOLED, OUTPUT);

  queue = xQueueCreate(
    1,              // no of items
    sizeof(int)     // size of items
  );
  assert(queue);

  timer = xTimerCreateStatic(
    "timer",
    pdMS_TO_TICKS(wait),    // delay
    pdTRUE,
    &queue,
    timer_cb_checkButton, 
    &timer_obj
  );
  assert(timer);

  rc = xTaskCreatePinnedToCore(
    task_updateLED,
    "updateLED",
    2048,
    &queue,
    1,
    &task,
    app_cpu
  );
  assert(rc == pdPASS);
  assert(task);

  rc = xTimerStart(timer, portMAX_DELAY);
  assert(rc == pdPASS);

  startf = true;
}

void loop() {
  // put your main code here, to run repeatedly:
  vTaskDelete(nullptr);
}
