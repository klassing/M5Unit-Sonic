/*!
 * @brief Aa ultrasonic distance sensor From M5Stack
 * @copyright Copyright (c) 2022 by M5Stack[https://m5stack.com]
 *
 * @Links [Unit Sonic I2C](https://docs.m5stack.com/en/unit/sonic.i2c)
 * @Links [Unit Sonic IO](https://docs.m5stack.com/en/unit/sonic.io)
 * @version  V0.0.2
 * @date  2022-07-21
 */

/* 
    Additions made by Ryan Klassing to convert the driver from blocking to
    instead be timer based, improving compatibility with other frameworks.

*/
#ifndef _UNIT_SONIC_H_
    #define _UNIT_SONIC_H_

    #include "Arduino.h"
    #include "Wire.h"
    #include "pins_arduino.h"

    #define SONIC_MAX_DISTANCE 4500     //4500mm is the farthest distance this chip can detect
    #define SONIC_MIN_DISTANCE 20       //20mm is the smallest we expect to be able to read
    #define SONIC_I2C_DATA_TIME 120     //120ms needed to get data from the chip

    class SONIC_I2C {
        public:
            /* Initializes the I2C bus for the sensor*/
            void begin(TwoWire* wire = &Wire, uint8_t addr = 0x57, uint8_t sda = SDA, uint8_t scl = SCL, uint32_t speed = 200000L);

            /* Gets the raw distance in mm of the sensor */
            float getDistance();

            /* Gets the raw distance in mm of the sensor, truncated to the nearest mm */
            uint16_t getDistance_uint16();

            /* Allows the calling functions to check whether or not the sensor is busy */
            uint8_t getStatus();

        private:
            /* Private variables to be used for setting up the I2C parameters for this sensor*/
            uint8_t _addr;
            TwoWire* _wire;
            uint8_t _scl;
            uint8_t _sda;
            uint8_t _speed;

            /* Private function to start various timers */
            void start_timer(uint32_t *timer);

            /* Private function to check if a timer has expired */
            uint8_t timer_expired(uint32_t *timer, uint32_t timeout);

            /* Private function to stop/reset a timer */
            void stop_timer(uint32_t *timer);

            /* Private variables to keep track of the measurement_ready timer */
            uint32_t _sensor_data_timer = 0;
            uint32_t _sensor_data = SONIC_MAX_DISTANCE;
            uint8_t _sensor_busy = false;

    };

    class SONIC_IO {
        private:
            uint8_t _trig;
            uint8_t _echo;

        public:
            void begin(uint8_t trig = 26, uint8_t echo = 32);
            float getDuration();
            float getDistance();
    };

#endif