# renesas.alexa-dust-sensor.embedded

## Overview
This is an air quality detection application that runs on the [Renesas Synergy™ S3A7 Fast Prototyping Kit](https://www.renesas.com/en-us/products/software-tools/boards-and-kits/renesas-synergy-kits/renesas-synergy-s3a7-prototyping-kit.html) built in the Renesas e2Studio ISDE utilizing the [Synergy™ Software Package (SSP)](https://www.renesas.com/en-us/products/synergy/software.html). The application calculates dust particle concentration using the Seed Studio [PPD42NS Grove Dust Sensor](http://www.mouser.com/ds/2/744/Seeed_101020012-838657.pdf), and reports low pulse occupancy (LPO) data to [Renesas IoT Sandbox](https://www.renesas.com/en-us/solutions/proposal/iot-sandbox.html).

Upon resetting, the application flashes the S3A7 Fast Prototyping Kit’s blue LED while waiting for a one minute sensor warmup period to pass. Thereafter, the green LED will flash to indicate that dust particle concentration readings are taking place and being periodically reported to Renesas IoT Sandbox in units of pcs/0.01 cubic feet. The yellow LED is turned on after startup to indicate that initialization succeeded, whereas the red LED indicates that initialization failed.

Currently, the green and blue LEDs flash on and off every second. Dust concentration is calculated by default over a 15 second window and reported to the cloud at the end of each window. The dust calculation window is able to be modified remotely via Renesas IoT Sandbox. The Grove Dust Sensor outputs low when dust particles are detected, and once the warmup period is over, the blue LED is turned on during low pulses from the sensor.

The board can be provisioned with Wi-Fi and Renesas IoT Sandbox connections by tapping the LCD immediately after the board is powered up, connecting to the board as a Wi-Fi access point, then entering the network SSID and password of the network the board should connect to, and also entering the project and user MQTT IDs and API key and password that were provided to you when registering with Renesas IoT Sandbox.

The Grove Dust Sensor must be attached to Grove A (UART3); Grove B (UART2) does not have the necessary interrupt pin and therefore is not compatible with this implementation.

The application contains 4 ThreadX threads and drivers.
* The `HAL/Common` thread provides hardware abstraction and board support and contains the following components in its thread stack:
    * `g_cgc` CGC Driver on `r_cgc` 
    * `g_ioport` I/O Port Driver on `r_ioport`
    * `g_elc` ELC Driver on `r_elc`
* The `MQTT Thread` provides Renesas IoT Sandbox cloud communications and contains the following components in its thread stack:
    * `g_flash0` Flash Driver on `r_flash_lp`
    * `g_sf_wifi_nsal_nx0` NetX Port using Wi-Fi Framework on `sf_wifi_nsal_nx`
    * `g_sf_wifi0` GT202 Wi-Fi Device Driver on `st_wifi_gt202`
    * `g_spi0` SPI Driver on `r_rspi`
    * `g_external_irq0` External IRQ Driver on `r_icu`
* The `GUI Thread` provides LCD support and contains the following components in its thread stack:
    * `g_Backlight_PWM` Timer Driver on `r_gpt`
    * `g_sf_touch_panel_i2c0` Touch Panel Framework on `sf_touch_panel_i2c`
    * `g_transfer_dma` Transfer Driver on `r_dmac Software Activation`
    * `g_sf_message0` Messaging Framework on `sf_message`
    * `g_i2c0` I2C Master Driver on `r_riic`
    * `g_sf_external_irq0` External IRQ Framework on `sf_external_irq`
    * `Touch Panel Driver` on `touch_panel_sx8654`
    * `g_transfer0` Transfer Driver on `r_dtc Event IIC0 TXI`
    * `g_transfer1` Transfer Driver on `r_dtc Event IIC0 RXI`
    * `g_external_irq1` External IRQ Driver on `r_icu`
* The `Sensor Thread` provides Grove Dust Sensor support, including LPO calculations and reporting, and contains the following components in its thread stack:
    * `g_fmio0` Factory Microcontroller Information (FMI) driver on `r_fmi`
    * `g_toggle_timer` Timer Driver on `r_gpt`. Provides a 1 Hz heartbeat that is used to flash LEDs to indicate status, and to     determine if the dust concentration window has elapsed.
    * `g_sensor_input_irq` External IRQ Driver on `r_icu`. Used to receive callbacks when the sensor output changes from high to low or vice versa. This IRQ is used to record the start and accumulative duration of low pulses over the dust calculation time window.
    * `g_elapsed_time_timer` Timer Driver on `r_gpt`. Provides a microsecond resolution timer/counter for calculating elapsed time.

## Windows Installation
Note that the following steps may require creating Renesas and SSP accounts.
1. Install [e2studio](https://www.renesas.com/en-us/software/D4000382.html), selecting Synergy Software Package during installation.

2. Install [SSP v1.2.0](https://synergygallery.renesas.com/ssp/package#read) “SSP_Distribution_1.2.0.zip”

3. Install [WiFi Addon](https://synergygallery.renesas.com/ssp/utility#read) —> SSP Utilities —> “Wi-Fi Framework (with GT202 Drivers)” (Click down pointing arrow)

4. Copy the file `./installation/Renesas.S3_IOT_BOARD.1.2.0.pack` (in this repository) to your local e2Studio installation directory: `\Renesas\e2_studio\internal\projectgen\arm\Packs`

5. Clone or download this repo to, for example, `c:\Users\<username>\e2studio\`. The root directory is an e2studio workspace.

6. Open e2Studio. Open the workspace in e2Studio, for example, via `File -> Switch Workspace -> <navigate to the workspace directory>`

7. Install e2Studio License - Navigate to license file when prompted: `\Renesas\e2_studio\internal\projectgen\arm\Licenses`
Select `SSP_License_Example_EvalLicense_20160629`.

8. Double click the project’s `configuration.xml`, and click `Generate Project Content`. e2studio may need to be restarted once this is complete.

9. Attempt to build by selecting project `GroveDustSensor` in the project explorer, then clicking `Project -> Build Project`, by right clicking the project in the project explorer and selecting `Build Project`.
