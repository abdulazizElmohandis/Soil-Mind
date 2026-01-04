#ifndef APP_ESP32_H
#define APP_ESP32_H



#define APP_TASK_CORE 0
#define APP_TASK_PRIORITY 3  // Higher than MQTT (2)
#define APP_TASK_STACK_SIZE 3072

void app1_task();





#endif // APP_ESP32_H