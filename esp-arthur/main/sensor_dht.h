#ifndef SENSOR_DHT_H
#define SENSOR_DHT_H

void dht_task(void *pvParameter);

bool dht_callback(void *args);

#endif // SENSOR_DHT_H