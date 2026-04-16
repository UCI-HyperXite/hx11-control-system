/*
 * lidar.h
 *
 */

#ifndef INC_COMPONENTS_LIDAR_H_
#define INC_COMPONENTS_LIDAR_H_

#define LIDAR_ADD 0x62<<1  // i2c slave address of lidar lite

void lidar_init(I2C_HandleTypeDef *hi2c);
int retrieve_lidar_distance();
void lidar_config(int);

#endif /* INC_COMPONENTS_LIDAR_H_ */
