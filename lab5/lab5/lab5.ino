/*
  Brody Lister
  California State University, Fullerton
  EGEC451
  Dr. Mike Turi
  May 4, 2026


  Measure the interface latency of an interrupt. 

  This program will use a the Arduino hardware timer macro "micros()" to measure the interface latency
  of an interrupt handler. Two pins will be wired together on the ESP32. A timer will routinely set the
  wire high to simulate the signal to be measured (recording its time with micros()). An ISR will be 
  attached to the rising edge of that pin to send a task notification to a task to record the time that  
  the input was measured (with micros()), and subtract the start and end time and then send it to the 
  serial monitor.

Pseudocode:

ISR()
  send task event

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
#define EXPERIMENT_INTERVAL_MS 1000 // delay between sending interrupt signals

volatile uint32_t start_time;
volatile uint32_t end_time;
TimerHandle_t timerh;
TaskHandle_t taskh;
static StaticTimer_t timer_obj;
int app_cpu;

bool startf = false;


void IRAM_ATTR isr() {
  vTaskNotifyGiveFromISR(taskh, nullptr);
}

void timer_callback(TimerHandle_t timerh) {
  start_time = micros();
  digitalWrite(GPIO_WIRE_START, HIGH);
  digitalWrite(GPIO_WIRE_START, LOW);
}

void measure_task_func(void* argp) {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    end_time = micros();
    auto latency = end_time - start_time;
    printf("Latency: %d\n", latency);
  }
}

void setup() {
  app_cpu = xPortGetCoreID();

  delay(1000);  // allow USB to connect

  pinMode(GPIO_WIRE_START, OUTPUT);
  pinMode(GPIO_WIRE_END, INPUT_PULLDOWN);
  attachInterrupt(GPIO_WIRE_END, isr, RISING);

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
