#ifndef _SENSOR_H
#define _SENSOR_H

#define SHM_SENSOR_KEY 0x1000

typedef struct {
  int humidity;
  int pressure;
  int temperature;
} sensor_data_t;

#endif