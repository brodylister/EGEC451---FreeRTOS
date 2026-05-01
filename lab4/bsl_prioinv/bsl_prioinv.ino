// Brody Lister
// CSU Fullerton
// EGEC451 - Dr. Mike Turi
// April 22, 2026
// Demonstrate priority inversion and inheritance
/*

  Three tasks will run, Red, Blue, and Green. Red will have prio 1, Blue prio 3, and Green prio 2. 
  The priority inversion situation looks like this:
  
  * Blue and Green are blocked.
  * Red is running, and locks a binary semaphore.
  * Before Red is done, Green unblocks and runs.
  * Before Green is done, Blue unblocks, and tries to access the shared resource, but the semaphore is still locked.
  * Since the resource is locked, Blue blocks, and Green resumes.
  * Thus, priority inversion.

  The solution involves a mutex and looks very similar:

  * Blue and Green are blocked.
  * Red is running, and locks a mutex.
  * Before Red is done, Green unblocks and runs.
  * Before Green is done, Blue unblocks, and tries to access the shared resource, but the mutex is locked.
  * Since the mutex is locked, Red inherits the priority and runs until giving up the mutex.
  * Since the mutex is now unlocked, Blue can run.
  * Thus, no inversion.
  
  Each task will flash an LED of their color, on and off, every several milliseconds, while running. 
  This allows us to see which task is currently running on a scope.

  The delays between the tasks unblocking will be significant so that we can measure it in real time.

*/


#define RedPin 12
#define BluePin 15
#define GreenPin 13

#define USE_MUTEX_INHERITANCE 0


static int app_cpu = 0;
static SemaphoreHandle_t mutex;


// Lock resource
static void lock_resource() {
  BaseType_t rc;

  rc = xSemaphoreTake(mutex, portMAX_DELAY);
  assert(rc == pdPASS);

  printf("Mutex Taken\n");
}

// Unlock resource
static void unlock_resource() {
  BaseType_t rc;

  printf("Mutex Given\n");
  rc = xSemaphoreGive(mutex);
  assert(rc == pdPASS);
}

// flicker LED
static void flickerLED(int pin) {
  digitalWrite(pin,HIGH);
  for (int i = 0; i < 3000000; i++) {
    __asm__ __volatile__ ("nop");
  }

  digitalWrite(pin,LOW);
  for (int i = 0; i < 3000000; i++) {
    __asm__ __volatile__ ("nop");
  }
}

// **** TASKS
static void redTask(void* argp) {

  printf("Red Task Initializing\n");

  flickerLED(RedPin);
  flickerLED(RedPin);
  flickerLED(RedPin);

  lock_resource();

  for (int i = 0; i < 10; i++) {
    flickerLED(RedPin);
  }
  unlock_resource();
  while (true) {
    flickerLED(RedPin);
  }
}

static void blueTask(void* argp) {

  printf("Blue Task Initializing\n");

  delay(4000);
  printf("Blue unblocked\n");
  
  lock_resource();

  while (true) {
    flickerLED(BluePin);
  }
}

static void greenTask(void* argp) {

  printf("Green Task Initializing\n");
  
  delay(2000);
  printf("Green unblocked\n");

  while (true) {
    flickerLED(GreenPin);
  }
}

void setup() { 
  // put your setup code here, to run once:
  app_cpu = xPortGetCoreID();
  BaseType_t rc;
  TaskHandle_t red, blue, green;

  delay(1000);    // usb connect

  pinMode(RedPin, OUTPUT);
  digitalWrite(RedPin, LOW);
  pinMode(GreenPin, OUTPUT);
  digitalWrite(GreenPin, LOW);
  pinMode(BluePin, OUTPUT);
  digitalWrite(BluePin, LOW);

#if USE_MUTEX_INHERITANCE
  mutex = xSemaphoreCreateMutex();
  assert(mutex);

  printf("Using a mutex for priority inheritance.\n");
#else 
  mutex = xSemaphoreCreateBinary();
  assert(mutex);
  rc = xSemaphoreGive(mutex);
  assert(rc == pdPASS);

  printf("Using a semaphore without priority inheritance.\n");
#endif

  rc = xTaskCreatePinnedToCore(
    redTask,
    "redTask",
    2048,
    nullptr,
    1,
    &red,
    app_cpu
  );
  assert(rc == pdPASS);
  assert(red);

  rc = xTaskCreatePinnedToCore(
    greenTask,
    "greenTask",
    2048,
    nullptr,
    2,
    &green,
    app_cpu
  );
  assert(rc == pdPASS);
  assert(green);

  rc = xTaskCreatePinnedToCore(
    blueTask,
    "blueTask",
    2048,
    nullptr,
    3,
    &blue,
    app_cpu
  );
  assert(rc == pdPASS);
  assert(blue);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
}
