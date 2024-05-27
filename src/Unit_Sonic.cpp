#include "Unit_Sonic.h"

/* 
    Additions made by Ryan Klassing to convert the driver from blocking to
    instead be timer based, improving compatibility with other frameworks.

*/

/* Initializes the I2C bus for the sensor*/
uint8_t SONIC_I2C::begin(TwoWire* wire, uint8_t addr, uint8_t sda, uint8_t scl, uint32_t speed) {
    _wire  = wire;
    _addr  = addr;
    _sda   = sda;
    _scl   = scl;
    _speed = speed;
    _wire->end();   //Verify the I2C bus hasn't been initialized previously with different configs
    _wire->begin((int)_sda, (int)_scl, (uint32_t)_speed);
    _sensor_busy = false;

    /* Verify that a sensor was detected */
    _wire->beginTransmission(_addr);

    /* endTransmission returns 0 on success*/
    return !_wire->endTransmission();
}

/* 
    Checks whether or not new data is available - this should be polled in the user's loop.  Once the function
    returns true --> the user should get the new data by calling the respective getDistance() or getDistance_uint16()
*/
uint8_t SONIC_I2C::readingAvailable() {
    /* 
        I'm not able to find a datasheet for this chip, so reverse engineering a bit from 
        the original driver.  They send 0x01 to the chip, wait 120ms, then read 3 bytes back.
        I would normally assume this is simply reading from 3 bytes starting at page 0x1,
        but since they wait 120ms between the write and read, I'm guessing 0x01 triggers
        the pulse, the chip performs the measurement, and then the chip makes the data available.

        So, we'll simply treat this like an ADC that has a conversion time by keeping track of a 
        "data ready" timer flag.  If the flag hasn't expired, we'll simply return the old measurement
        data.  If the timer has expired, then we'll grab new data to return.  Easy peasy.
   */

    if(!_sensor_busy) {

        /* Trigger a data collection */
        _wire->beginTransmission(_addr);    // Transfer data to 0x57. 将数据传输到0x57
        _wire->write(0x01);                 // Trigger the sensor reading
        _wire->endTransmission();           // Stop data transmission with the Ultrasonic

        /* Start the timer and flag that the sensor is busy */
        _sensor_busy = true;
        start_timer(&_sensor_data_timer);
    }

    /* See if the new data is available */
    if(timer_expired(&_sensor_data_timer, SONIC_I2C_DATA_TIME)) {
        const uint8_t bytes_to_read = 3;

        /* Read the data from the sensor */
        _wire->requestFrom(_addr, bytes_to_read);       // Request 3 bytes from Ultrasonic Unit

        /* Clear the old data */
        _sensor_data = 0;

        /* Read the bytes and shift the data as needed (data is big endian) */
        for (uint8_t i = bytes_to_read; i > 0; i--) {_sensor_data |= (_wire->read() << (8 * (i - 1)));}

        /* Flag that the sensor is no longer busy and stop the timer */
        _sensor_busy = false;
        stop_timer(&_sensor_data_timer);

        /* Flag that the sensor has data available */
        return true;
    }

    /* If we made it here, there isn't any new data */
    return false;
}

/* 
    Gets the raw distance in mm of the sensor.  This will always contain the latest reading.  If the user wishes
    to implement any averaging, it is necessary to only call this function once readingAvailable() returns true.
    Otherwise, the user will read the same data over and over throwing off the average count.
*/
float SONIC_I2C::getDistance() {
    /* 
        Since we now have a global data tracking the integer of the readings, 
        simply make sure its up to date and then convert it to a floating point
    */

    /* Convert the distance to floating point and in mm */
    float Distance = float(_sensor_data) / 1000.0;

    /* Clamp the max distance */
    return min(Distance, float(SONIC_MAX_DISTANCE));
}

/* 
    Gets the raw distance truncated to the nearest mm of the sensor.  This will always contain the latest reading.  
    If the user wishes to implement any averaging, it is necessary to only call this function once readingAvailable() returns true.
    Otherwise, the user will read the same data over and over throwing off the average count.
*/
uint16_t SONIC_I2C::getDistance_uint16() {

    /* Clamp the max distance to be returned */
    return min((uint16_t)(_sensor_data / 1000), (uint16_t)SONIC_MAX_DISTANCE);
}

/* Allows the calling functions to check whether or not the sensor is busy */
uint8_t SONIC_I2C::getStatus() {
    return _sensor_busy;
}

/* Private function to start various timers */
void SONIC_I2C::start_timer(uint32_t *timer) {*timer = millis();}

/* Private function to check if a timer has expired */
uint8_t SONIC_I2C::timer_expired(uint32_t *timer, uint32_t timeout) {return (millis() - *timer > timeout && *timer != 0) ? true : false;}

/* Private function to stop/reset a timer */
void SONIC_I2C::stop_timer(uint32_t *timer) {*timer = 0;}

/* Initializes the private variables for the sensor */
void SONIC_IO::begin(uint8_t trig_pin /*=26*/, uint8_t echo_pin /*=32*/) {
    _trig_pin = trig_pin;
    _echo_pin = echo_pin;
    _sensor_pulse_duration = 0;
    _sensor_busy = false;
    _sensor_data_ready = false;

    pinMode(_trig_pin, OUTPUT);
    pinMode(_echo_pin, INPUT);
}

/* 
    This should be called by the user's ISR on the echo pin, set to RISING edge trigger.
    This will start the pulse measuring timer.
*/
void SONIC_IO::echo_isr_rising() {
    /* Start the pulse measurement timer */
    _sensor_pulse_start = micros();

}

/* 
    This should be called by the user's ISR on the echo pin, set to RISING edge trigger.
    This will be used to calculate the duration of the echo pulse
*/
void SONIC_IO::echo_isr_falling() {
    /* Calculate the pulse duration */
    _sensor_pulse_duration = micros() - _sensor_pulse_start;

    /* Flag that the sensor is no longer busy */
    _sensor_data_ready = true;

}

/* 
    Checks whether or not new data is available - this should be polled in the user's loop.  Once the function
    returns true --> the user should get the new data by calling the respective getDistance() or getDistance_uint16()
*/
uint8_t SONIC_IO::readingAvailable() {
    /* 
        I'm not able to find a datasheet for this chip, so reverse engineering a bit from 
        the original driver.  They send a 10us pulse on the _trig_pin, then measure a pulse
        of LOW-HIGH-LOW (duration of HIGH) in microseconds, to determine the amount of time
        for sound to travel to the target and return.
   */

    if(!_sensor_busy) {

        /* Trigger a data collection */
        digitalWrite(_trig_pin, HIGH);
        delayMicroseconds(SONIC_IO_TRIG_PULSE_US);
        digitalWrite(_trig_pin, LOW);

        /* Start the timeout timer and flag that the sensor is busy */
        _sensor_busy = true;
        _sensor_data_ready = false;
        start_timer(&_sensor_timeout_timer);
    }

    /* See if there is new data available */
    if(_sensor_data_ready) {
        _sensor_data = U32_SONIC_PULSE_TO_UM(_sensor_pulse_duration);

        data_collected();

        /* Flag that the sensor has data available */
        return true;
    }

    /* See if a timeout has occured */
    if(timer_expired(&_sensor_timeout_timer, SONIC_IO_TIMEOUT_MS)) {
        /* Clear all pending measurements since the object is too far away to measure */
        _sensor_data = SONIC_MAX_DISTANCE;
        data_collected();

        /* Flag that the sensor has data available */
        return true;
    }

    /* If we made it here, there isn't any new data */
    return false;
}

/* 
    Gets the raw distance in mm of the sensor.  This will always contain the latest reading.  If the user wishes
    to implement any averaging, it is necessary to only call this function once readingAvailable() returns true.
    Otherwise, the user will read the same data over and over throwing off the average count.
*/
float SONIC_IO::getDistance() {
    /* Clamp the max distance to be returned */
    return min(F_SONIC_UM_TO_MM(_sensor_data), float(SONIC_MAX_DISTANCE));
}

/* 
    Gets the raw distance truncated to the nearest mm of the sensor.  This will always contain the latest reading.  
    If the user wishes to implement any averaging, it is necessary to only call this function once readingAvailable() returns true.
    Otherwise, the user will read the same data over and over throwing off the average count.
*/
uint16_t SONIC_IO::getDistance_uint16() {

    /* Clamp the max distance to be returned */
    return min(U16_SONIC_UM_TO_MM(_sensor_data), (uint16_t)SONIC_MAX_DISTANCE);
}

/* Allows the calling functions to check whether or not the sensor is busy */
uint8_t SONIC_IO::getStatus() {
    return _sensor_busy;
}

/* Private function to start various timers */
void SONIC_IO::start_timer(uint32_t *timer) {*timer = millis();}

/* Private function to check if a timer has expired */
uint8_t SONIC_IO::timer_expired(uint32_t *timer, uint32_t timeout) {return (millis() - *timer > timeout && *timer != 0) ? true : false;}

/* Private function to stop/reset a timer */
void SONIC_IO::stop_timer(uint32_t *timer) {*timer = 0;}

/* Private function to clear variables when we've completed a new measurement */
void SONIC_IO::data_collected() {
    stop_timer(&_sensor_timeout_timer);
    _sensor_data_ready = false;
    _sensor_busy = false;
}