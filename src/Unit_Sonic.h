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

    #define SONIC_IO_TRIG_PULSE_US 10   //10us needed to start the pulse from the chip
    #define SONIC_IO_TIMEOUT_MS 120 //((2 * SONIC_MAX_DISTANCE / 343) + 1)     //Sets a timeout for the total time needed to measure the maximum distance - accounting for out/return flight

    #define SONIC_SOUND_US_TO_UM(x) (x * 343)                                       //Sound travels 343um in 1us
    #define U32_SONIC_PULSE_TO_UM(x) (uint32_t)(SONIC_SOUND_US_TO_UM(x)/2)          //Pulses include time-to-target + return flight --> only need half the pulse width, measured in micrometers
    #define U16_SONIC_UM_TO_MM(x) (uint16_t)(x/1000)                                //Convert to truncated mm
    #define F_SONIC_UM_TO_MM(x) float(x/1000.0)                                     //Convert to mm floating point

    class SONIC_I2C {
        public:
            /* Initializes the I2C bus for the sensor - returns whether it was detected or not */
            uint8_t begin(TwoWire* wire = &Wire, uint8_t addr = 0x57, uint8_t sda = SDA, uint8_t scl = SCL, uint32_t speed = 200000L);

            /* 
                Checks whether or not new data is available - this should be polled in the user's loop.  Once the function
                returns true --> the user should get the new data by calling the respective getDistance() or getDistance_uint16()
            */
            uint8_t readingAvailable();

            /* 
                Gets the raw distance in mm of the sensor.  This will always contain the latest reading.  If the user wishes
                to implement any averaging, it is necessary to only call this function once readingAvailable() returns true.
                Otherwise, the user will read the same data over and over throwing off the average count.
            */
            float getDistance();

            /* 
                Gets the raw distance truncated to the nearest mm of the sensor.  This will always contain the latest reading.  
                If the user wishes to implement any averaging, it is necessary to only call this function once readingAvailable() returns true.
                Otherwise, the user will read the same data over and over throwing off the average count.
            */
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
        public:
            /* Initializes the private variables for the sensor */
            void begin(uint8_t trig_pin = 26, uint8_t echo_pin = 32);

            /* 
                This should be called by the user's ISR on the echo pin, set to RISING edge trigger.
                This will start the pulse measuring timer.
            */
            void echo_isr_rising();

            /* 
                This should be called by the user's ISR on the echo pin, set to RISING edge trigger.
                This will be used to calculate the duration of the echo pulse
            */
           void echo_isr_falling();

            /* 
                Checks whether or not new data is available - this should be polled in the user's loop.  Once the function
                returns true --> the user should get the new data by calling the respective getDistance() or getDistance_uint16()
            */
            uint8_t readingAvailable();

            /* 
                Gets the raw distance in mm of the sensor.  This will always contain the latest reading.  If the user wishes
                to implement any averaging, it is necessary to only call this function once readingAvailable() returns true.
                Otherwise, the user will read the same data over and over throwing off the average count.
            */
            float getDistance();

            /* 
                Gets the raw distance truncated to the nearest mm of the sensor.  This will always contain the latest reading.  
                If the user wishes to implement any averaging, it is necessary to only call this function once readingAvailable() returns true.
                Otherwise, the user will read the same data over and over throwing off the average count.
            */
            uint16_t getDistance_uint16();

            /* Allows the calling functions to check whether or not the sensor is busy */
            uint8_t getStatus();

        private:

            /* Private function to start various timers */
            void start_timer(uint32_t *timer);

            /* Private function to check if a timer has expired */
            uint8_t timer_expired(uint32_t *timer, uint32_t timeout);

            /* Private function to stop/reset a timer */
            void stop_timer(uint32_t *timer); 

            /* Private function to clear variables when we've completed a new measurement */
            void data_collected();

            /* Private variables to keep track of pin settings */
            uint8_t _trig_pin;
            uint8_t _echo_pin;

            /* Private variables for tracking the IO pulse duration */
            uint32_t _sensor_pulse_start = 0;
            uint32_t _sensor_timeout_timer = 0;
            uint32_t _sensor_pulse_duration = 0;
            uint32_t _sensor_data = SONIC_MAX_DISTANCE;
            uint8_t _sensor_busy = false;
            uint8_t _sensor_data_ready = false;
    };

#endif