# mruby-fm3i2cmaster
FM3 I2CMaster class

## Methods

|method|parameter(s)|return|
|---|---|---|
|I2CMaster#new|MFS ch, pin location, bit rate|(nil)|
|I2CMaster#write|Slave address(7bit), command|(nil)|
|I2CMaster#read|Slave address(7bit), command|8bit data or ERROR(-1)|

## Constants

|constant||
|---|---|
|I2CMaster::MFS0|use MFS ch0|
|I2CMaster::MFS1|use MFS ch1|
|I2CMaster::MFS2|use MFS ch2|
|I2CMaster::MFS3|use MFS ch3|
|I2CMaster::MFS4|use MFS ch4|
|I2CMaster::MFS5|use MFS ch5|
|I2CMaster::MFS6|use MFS ch6|
|I2CMaster::MFS7|use MFS ch7|
|I2CMaster::PIN_LOC0|use pin location 0|
|I2CMaster::PIN_LOC1|use pin location 1|
|I2CMaster::PIN_LOC2|use pin location 2|

## Sample

    # 7bit slave address
    SLAVE_ADD = 0x48  
    
    a = I2CMaster.new(I2CMaster::MFS2, I2CMaster::PIN_LOC2, 100000)
    
    # write command 0x10
    a.write(SLAVE_ADD, 0x10)
    
    # read with command 0x03
    data = a.read(SLAVE_ADD, 0x03)

## License
個々のファイルについて、個別にライセンス・著作権表記があるものはそのライセンスに従います。
ライセンス表記がないものに関しては以下のライセンスを適用します。

Copyright 2015 Japan OSS Promotion Forum

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.