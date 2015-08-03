# Smart Thermostat - Arduino

This repository is accompanying the blog post [Making your own smart 'machine learning' thermostat using Arduino, AWS, HBase, Spark, Raspberry PI and XBee](http://niektemme.com/2015/07/31/smart-thermostat/ @@to do). This blog post describes building and programming your own smart thermostat. 

This smart thermostat is based on three feedback loops. 
- **I. The first loop is based on an Arduino directly controling the boiler.  (this repostiory)**
- II. The second feedback loop is a Raspberry PI that receives temperature data en boiler status information from the Arduino and sends instructions to the Arduino. [Smart Thermostat - Raspberry PI Repository](https://github.com/niektemme/smarttherm-rpi)
- II. The third and last feedback loop is a server in the Cloud. This server uses machine learning to optimize the boiler control model that is running on the Raspberry PI. [Smart Thermostat - AWS - HBase - Spark Repository Repository](https://github.com/niektemme/smarttherm-rpi)

## Installation & Setup

### Hardware setup
The hardware setup is described in detail in the [blog post]( http://niektemme.com/2015/07/31/smart-thermostat/ @@) mentioned above. 

### Dependencies
The following Arduino libraries are required
- LiquidCrystal (LCD) - should be installed by default
- SoftwareSerial - should be installed by default
- XBee - [Google code page](https://code.google.com/p/xbee-arduino/)

### Installation
The Arduino sketch in the smarttherm subfolder of this repository can be uploaded to the arduino as with any Arduino sketch.
