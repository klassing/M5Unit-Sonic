#include "Unit_Sonic.h"

/* 
    Additions made by Ryan Klassing to convert the driver from blocking to
    instead be timer based, improving compatibility with other frameworks.

*/

/* Initializes the I2C bus for the sensor*/
void SONIC_I2C::begin(TwoWire* wire, uint8_t addr, uint8_t sda, uint8_t scl, uint32_t speed) {
    _wire  = wire;
    _addr  = addr;
    _sda   = sda;
    _scl   = scl;
    _speed = speed;
    _wire->begin((int)_sda, (int)_scl, (uint32_t)_speed);
    _sensor_busy = false;
}

/* Gets the raw distance in mm of the sensor */
float SONIC_I2C::getDistance() {
    /* 
        Since we now have a global data tracking the integer of the readings, 
        simply make sure its up to date and then convert it to a floating point
    */

    /* Ensure the global data is up to date -- but don't use the returned value, it will be truncated */
    getDistance_uint16();

    /* Convert the distance to floating point and in mm */
    float Distance = float(_sensor_data) / 1000.0;

    /* Clamp the max distance */
    return min(Distance, float(SONIC_MAX_DISTANCE));
}

/* Gets the raw distance in mm of the sensor, truncated to the nearest mm */
uint16_t SONIC_I2C::getDistance_uint16() {
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
    }

    /* Clamp the max distance */
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

/*! @brief Initialize the Sonic. */
void SONIC_IO::begin(uint8_t trig, uint8_t echo) {
    _trig = trig;
    _echo = echo;
    pinMode(_trig,
            OUTPUT);  // Sets the trigPin as an Output. 将 TrigPin 设置为输出
    pinMode(_echo,
            INPUT);  // Sets the echoPin as an Input. 将 echoPin 设置为输入
}

float SONIC_IO::getDuration() {
    digitalWrite(_trig, LOW);
    delayMicroseconds(2);
    // Sets the trigPin on HIGH state for 10 micro seconds.
    // 将TrigPin设定为高电平状态，持续10微秒。

    digitalWrite(_trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trig, LOW);

    // Reads the echoPin, returns the sound wave travel time in microseconds
    //读取echoPin，返回以微秒为单位的声波移动时间
    float duration = pulseIn(_echo, HIGH);

    return duration;
}

/*! @brief Get distance data. */
float SONIC_IO::getDistance() {
    // Calculating the distance
    float Distance = getDuration() * 0.34 / 2;
    if (Distance > 4500.00) {
        return 4500.00;
    } else {
        return Distance;
    }
}