#ifndef SENSOR_PIR_H
#define SENSOR_PIR_H

extern volatile BaseType_t PIR_task_suspended; // Flag para controle de suspensão de task

void pir_task(void *pvParameter);

#endif // SENSOR_PIR_H