# Changelog

## Unreleased

**Note:** Unreleased changes are checked in but not part of an official release (available through the Arduino IDE or PlatfomIO) yet. This allows you to test WiP features and give feedback to them.

## [0.0.4] - 2024-05-26
- Updated IO portion of the driver to be non blocking as well

## [0.0.3] - 2024-04-28
- Forked from [M5stack M5Unit-Sonic v0.0.2](https://github.com/m5stack/M5Unit-Sonic/releases/tag/0.0.2) and made the following changes:
    - Converted driver to be timer based instead of a blocking driver
    - Changed to return uint16_t data for all measurements (the sensor varies a few mm anyway, so 1mm accuracy is good enough)

## Note

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).