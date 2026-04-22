// Brody Lister, California State University Fullerton
// Demonstrate use of a locking semaphore
// Dr. Mike Turi, EGEC451
// March 27, 2026

/*
I plan to have one non-reentrant "thinker" task, which will slowly swap two values back and forth, then go to sleep for a while. A second task, the "interrupter", will be modifying a temporary global variable constantly. The "thinker" will use the global variable to swap its two variables, and it should be visible when the data becomes modified unintentionally.

pseudocode:

global int temp;

thinker()       [ non reentrant ]
 take semaphore
 set x to 2, y to 4
 temp = x,
 delay (critical, could interrupt)
 x = y, y = temp
 print x and y
 give semaphore

interruptor()
 infinite loop setting temp to 100, 200, ...


Printing x and y will show whether their values are successfully swapping back and forth, or if the corrupted data is being used from the interruptor. Both tasks will have a preprocessor directive controlling use of the semaphore macro. I won't print the temp variable to keep it readable.

Expected output without semaphore:
x is 2, y is 4
x is 4, y is 2
x is 2, y is 100
...

Expected output with semaphore:
x is 2, y is 4
x is 4, y is 2
x is 2, y is 4
...

*/

#define USE_SEMAPHORE 0

static SemaphoreHandle_t sem;
static unsigned seed = 123456;

BaseType_t global_variable;



static void interrupterTask(void *argp) {
  BaseType_t x = 0;
  BaseType_t rc;

  for (;;) {

#if USE_SEMAPHORE
  rc = xSemaphoreTake(sem, portMAX_DELAY);
  assert(rc == pdPASS);
#endif

    x = (x+100)%1000 + 100;     // 100, 200, ... 800, 900, 1000, 100, ...
    global_variable = x;
    delay(rand_r(&seed)%5 + 1);

#if USE_SEMAPHORE
  rc = xSemaphoreGive(sem);
  assert(rc == pdPASS);
#endif

  } 
}

static void thinkerTask(void *argp) {       // non-reentrant
  BaseType_t rc;

  for (;;) {

#if USE_SEMAPHORE
  rc = xSemaphoreTake(sem, portMAX_DELAY);
  assert(rc == pdPASS);
#endif

  int x = 2;
  int y = 4;        // do this every time so we can see the changes

  for (unsigned i = 0; i < 2; ++i ) {
    print(x, y);
    global_variable = x;
    x = y;
    delay(10);      // make interruption more likely
    y = global_variable;
  }
  print(x, y);

#if USE_SEMAPHORE
  rc = xSemaphoreGive(sem);
  assert(rc == pdPASS);
#endif

  }
}


static void print(int x, int y) {
  printf("x is %u, y is %u\n",
  x,
  y);
}

void setup() {
  // put your setup code here, to run once:
  BaseType_t rc;
  TaskHandle_t interrupter, thinker;

  int app_cpu = xPortGetCoreID();
  printf("2");
  delay(1000);        // allow USB to connect

#if USE_SEMAPHORE
  printf("1");
  sem = xSemaphoreCreateBinary();
printf("1");
  assert(sem);
  printf("1");
  rc = xSemaphoreGive(sem);
  printf("1");
  assert(rc == pdPASS);
  printf("1");
  assert(sem);
  printf("Using Semaphore Protection.\n");
#endif

  rc = xTaskCreatePinnedToCore(
    interrupterTask,
    "interrupterTask",
    5000,
    nullptr,
    1,
    &interrupter,
    app_cpu
  );
  assert(rc == pdPASS);
  assert(interrupter);


  rc = xTaskCreatePinnedToCore(
    thinkerTask,
    "thinkerTask",
    5000,
    nullptr,
    1,
    &thinker,
    app_cpu
  );
  assert(rc == pdPASS);
  assert(thinker);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
}
