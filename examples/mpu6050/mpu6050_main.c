/****************************************************************************
 * apps/examples/mpu6050/mpu6050_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <endian.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* MPU6050 Sensitivity Scale Factors (Default settings on reset) */
#define ACCEL_SCALE_FACTOR  16384.0f  /* ±2g range -> 16384 LSB/g */
#define GYRO_SCALE_FACTOR   131.0f    /* ±250 °/s range -> 131 LSB/(°/s) */

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Packed sensor register layout as read from the MPU6050 */
struct mpu_sensor_data_s
{
  int16_t x_accel;
  int16_t y_accel;
  int16_t z_accel;
  int16_t temp;
  int16_t x_gyro;
  int16_t y_gyro;
  int16_t z_gyro;
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * mpu6050_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  int count = 10; /* Read 10 samples by default */
  struct mpu_sensor_data_s raw_data;
  ssize_t nbytes;

  if (argc > 1)
    {
      count = atoi(argv[1]);
    }

  printf("Opening MPU6050 device /dev/imu0...\n");
  fd = open("/dev/imu0", O_RDONLY);
  if (fd < 0)
    {
      perror("ERROR: Failed to open /dev/imu0");
      return -1;
    }

  printf("Reading %d samples from MPU6050:\n", count);
  printf("---------------------------------------------------------------------------------\n");
  printf("  Sample | Accel X (g) | Accel Y (g) | Accel Z (g) | Temp (C) | Gyro X | Gyro Y | Gyro Z \n");
  printf("---------------------------------------------------------------------------------\n");

  for (int i = 0; i < count; i++)
    {
      /* Read a single snapshot of raw data from the IMU */
      nbytes = read(fd, &raw_data, sizeof(struct mpu_sensor_data_s));
      if (nbytes != sizeof(struct mpu_sensor_data_s))
        {
          perror("ERROR: Failed to read from IMU device");
          close(fd);
          return -1;
        }

      /* Convert Big-Endian register values to Host-Endian */
      int16_t ax = be16toh((uint16_t)raw_data.x_accel);
      int16_t ay = be16toh((uint16_t)raw_data.y_accel);
      int16_t az = be16toh((uint16_t)raw_data.z_accel);
      int16_t raw_temp = be16toh((uint16_t)raw_data.temp);
      int16_t gx = be16toh((uint16_t)raw_data.x_gyro);
      int16_t gy = be16toh((uint16_t)raw_data.y_gyro);
      int16_t gz = be16toh((uint16_t)raw_data.z_gyro);

      /* Calibrate raw values into standard physical units */
      float ax_g = (float)ax / ACCEL_SCALE_FACTOR;
      float ay_g = (float)ay / ACCEL_SCALE_FACTOR;
      float az_g = (float)az / ACCEL_SCALE_FACTOR;
      float temp_c = ((float)raw_temp / 340.0f) + 36.53f;
      float gx_dps = (float)gx / GYRO_SCALE_FACTOR;
      float gy_dps = (float)gy / GYRO_SCALE_FACTOR;
      float gz_dps = (float)gz / GYRO_SCALE_FACTOR;

      printf("   %4d  |   %7.3f   |   %7.3f   |   %7.3f   |  %5.1f   | %6.1f | %6.1f | %6.1f \n",
             i + 1, ax_g, ay_g, az_g, temp_c, gx_dps, gy_dps, gz_dps);

      /* Sleep for 50ms before reading the next sample */
      usleep(50 * 1000);
    }

  printf("---------------------------------------------------------------------------------\n");
  printf("Finished reading samples. Closing device.\n");
  close(fd);

  return 0;
}
