#ifndef _SENSOR_H
#define _SENSOR_H

#define SHM_BMP280_KEY 0x1000

typedef struct {
  int humidity;
  int pressure;
  int temperature;
} BMP280_data_t;

#endif