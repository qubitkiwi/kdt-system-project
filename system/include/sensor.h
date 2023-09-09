#ifndef _SENSOR_H
#define _SENSOR_H

#define SHM_BMP280_KEY 0x1000
#define SHM_BMP280_ID "/SHM_BMP280"


typedef struct {
  int temperature;
  int pressure;  
} BMP280_data_t;

#endif