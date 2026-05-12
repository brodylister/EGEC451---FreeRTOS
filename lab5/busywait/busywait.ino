/*
  Brody Lister
  California State University, Fullerton
  EGEC451
  Dr. Mike Turi
  May 4, 2026


  Measure the interface latency of an interrupt. 
This program will use a very similar setup to the last problem, but rather than have an interrupt attached to the rising edge of the signal, the task will poll the state of the wire in a loop.

Pseudocode:

measure_task()
  setup
  loop:
    wait for task event
    measure time with micros and store to end_time

timerCallback()
  write high to pin

setup()
  attach interrupt to rising edge of pin
*/

#define GPIO_WIRE_START 12
#define GPIO_WIRE_END 15
#define EXPERIMENT_INTERVAL_MS 1000 // delay between sending signals

volatile uint32_t start_time;
volatile uint32_t end_time;
TimerHandle_t timerh;
TaskHandle_t taskh;
static StaticTimer_t timer_obj;
int app_cpu;

bool startf = false;

void timer_callback(TimerHandle_t timerh) {
  start_time = micros();
  digitalWrite(GPIO_WIRE_START, HIGH);
  for (int i = 0; i < 40000; i++) {
    __asm__ __volatile__ ("nop");
  }
  digitalWrite(GPIO_WIRE_START, LOW);
}

void measure_task_func(void* argp) {
  while (true) {
    int read = digitalRead(GPIO_WIRE_END);
    if (read == 1) {
      end_time = micros();
      auto latency = end_time - start_time;
      printf("Latency: %d\n", latency);
      for (int i = 0; i < 40000; i++) {
        __asm__ __volatile__ ("nop");
      }
    }
  }
}

void setup() {
  app_cpu = xPortGetCoreID();

  delay(1000);  // allow USB to connect

  pinMode(GPIO_WIRE_START, OUTPUT);
  pinMode(GPIO_WIRE_END, INPUT_PULLDOWN);

  BaseType_t rc;

  rc = xTaskCreatePinnedToCore(
    measure_task_func,
    "measure_task",
    2048,
    nullptr,
    1,
    &taskh,
    app_cpu
  );
  assert(rc == pdPASS);

  timerh = xTimerCreateStatic(
    "timer",
    pdMS_TO_TICKS(EXPERIMENT_INTERVAL_MS),
    pdTRUE,
    nullptr,  //?
    timer_callback,
    &timer_obj
  );
  rc = xTimerStart(timerh, portMAX_DELAY);
  assert(rc == pdPASS);

}

void loop() {
  // put your main code here, to run repeatedly:
  vTaskDelete(nullptr);
}
