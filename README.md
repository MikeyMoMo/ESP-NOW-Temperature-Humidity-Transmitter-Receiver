# ESP-NOW-Temperature-Humidity-Transmitter-Receiver for T5 Display
Using ESP32 with temperature/humidity sensors to read and transmit to a receiver.  Can have 1 inside and multiple outside transmitters.  ESP-NOW for transport.

________________________________________
# ESP NOW Temperature & Humidity Transmitter

📖 Overview
This project implements a low power ESP32 based transmitter that reads temperature and relative humidity from supported sensors (AHT10/20 or BME280) and sends the data via ESP NOW to a paired receiver. If no sensor is detected, it generates simulated values for testing.

  ⚙️ Features

    •	Sensor support: 

      o	AHT10/20 (via Adafruit AHTX0 library)

      o	BME280 (via Adafruit BME280 library)

    •	ESP NOW communication: 

      o	Peer registration with target MAC address

      o	CRC8 checksum for data integrity

      o	Sequence numbers to avoid duplicate processing

    •	Resilience: 

      o	Tracks cumulative good/bad sends across deep sleep cycles

      o	Backoff and reboot logic after repeated failures

      o	Daily restart after 288 transmissions to clear memory detritus

    •	Low power operation: 

      o	Deep sleep between transmissions

      o	Adjustable jitter to reduce collision risk

    •	Debugging: 

      o	Optional verbose output with SHOW_DETAILS flag

      o	Detailed error reporting for ESP NOW send failures


🛠️ Dependencies

  •	Arduino IDE 2.x (supports relative includes)

  •	Libraries: 

      o	esp_now (ESP32 core)

      o	WiFi (ESP32 core)

      o	Adafruit AHTX0

      o	Adafruit Sensor

      o	Adafruit BME280

  •	Custom headers: 

      o	Definitions.h

      o	Utilities.h

      o	../CommonFile_v2.h


🚀 Usage
  1.	Flash the sketch to a LilyGO T5 v2.13 (ESP32 with ePaper).
  
  2.	Configure the target MAC address of the receiver in targetMAC[].
  
  3.	Power on the device. 
  
      o	On first boot, counters are initialized.
    
      o	Sensor detection runs automatically.
    
      o	Data is transmitted every ~5 minutes with jitter.
    
  4.	Monitor output via Serial (115200 baud) if SHOW_DETAILS is enabled.



📊 Data Format

    •	Temperature (°C × 100)
  
    •	Relative Humidity (% × 100)
  
    •	Packet version
  
    •	Sequence number
  
    •	Cumulative good/bad sends
  
    •	CRC8 checksum

  
🔒 Reliability

    •	RTC_NOINIT_ATTR variables preserve counters across resets.
  
    •	RTC_DATA_ATTR variables persist across deep sleep.
  
    •	Automatic reboot after repeated send failures or daily cycle limit.
  
________________________________________

# ESP NOW Temperature & Humidity Receiver

📖 Overview

  This project implements an ESP32 based receiver that listens for temperature and humidity data sent via ESP NOW from one or more transmitters. It validates, processes, and displays the readings on a LilyGO T5 v2.13 ePaper display, while also tracking statistics and maintaining a rolling 24 hour history.

  ⚙️ Features

    •	Multi transmitter support: 

      o	Designed for two transmitters (Inside and Outside), indexed starting at 1 for clarity.

    •	Data handling: 

      o	Stores temperature and humidity readings per transmitter.

      o	Tracks packet sequence numbers to detect duplicates and missed packets.

    o	  Clears stale data if readings are too old.

    •	Validation: 

      o	CRC8 checksum verification for data integrity.

      o	Version checking for packet compatibility.

    •	Time management: 

      o	NTP synchronization via SNTP.

      o	Automatic timezone and DST offset deduction.

      o	Maintains local and UTC time for accurate logging.

    •	Statistics: 

      o	Tracks minimum and maximum temperatures over a 24 hour rolling window.

      o	Computes dew point from outside readings.

    •	Display output: 

      o	Shows Inside and Outside readings on the T5 ePaper.

      o	Updates every 5 minutes during the day, hourly at night.

      o	Handles “n/a” gracefully when data is missing.

    •	Resilience: 

      o	Detects and clears stale readings.

      o	Reboots if WiFi or time synchronization fails repeatedly.

🛠️ Dependencies

    •	Arduino IDE - Any version

    •	Libraries: 

      o	esp_now (ESP32 core)

      o	WiFi (ESP32 core)

      o	esp_sntp (ESP32 core)

      o	TimeLib

      o	vector (C++ STL)

    o	GFX_Setups (for T5 ePaper display)

      •	Custom headers: 

    o	Definitions.h

    o	Utilities.h

    o	../CommonFile_v2.h

🚀 Usage

    1.	Flash the sketch to a LilyGO T5 v2.13 (ESP32 with ePaper).

    2.	Configure transmitter MAC addresses (e.g., insideMAC) to match your hardware.

    3.	Power on the device. 

      o	Initializes WiFi and synchronizes time via NTP.

      o	Sets up ESP NOW to listen for packets.

      o	Displays connection status and waits for data.

    4.	Receives and validates packets, updates display every cycle.

📊 Data Format

    •	Temperature (°C × 100, converted to °C or °F depending on config)

    •	Relative Humidity (% × 100)

    •	Packet version and sequence number

    •	Cumulative good/bad sends from transmitter

    •	CRC8 checksum

🔒 Reliability

    •	Duplicate detection: Ignores repeated packets.

    •	Missed packet tracking: Counts gaps in sequence numbers.

    •	Stale data clearing: Resets readings if too old.

    •	Rolling history: Maintains vector of readings for 24 hours.

    •	Automatic reboot: If WiFi or time sync fails beyond threshold.

________________________________________
The preceeding was prepared by CoPilot by reading the source and listing main points it considered interesting.  

The following was written by the author.  Between all of this, you should get the picture.
________________________________________

This project was started to let me learn about programming and operation of ESP-NOW as a transport mechanism and to end up with a remote sensing thermometer that would send from more than one sensor to central receiver.  I had been wanting an inside and outside thermometer for some time and I found out about ESP-NOW while deciding how to do the communications so I had a chance to use it and learn it.  It turns out to be a very simple protocol with a couple of variations that make it easy to send and receive messages between units. The protocol does not implement resends but you can do that fairly easily with the results of the sends as reported.  To get this information, you must send to a fixed MAC address.  If you want to broadcast to all receivers in the area, you lose the feedback about the success of the send.  This is only natural since there are multiple receivers listening and reporting the reception status of each one could be quite a bit of overhead.

It was designed to have one indoor sensor and two or more outdoor sensors. It was done in about a week.

The reason for multiple outdoor sensors is to mitigate the effect of sun/light heating of a single sensor.  If you take readings from both side of the house aligned with sun movement, you can then take the lower or lowest reading each sample period and use that number as your final number for that sample period.

While working on this project, I found that one could use ESP-NOW and WiFi at the same time.  It is believed that this is not possible but it surely is.  I was unsure if this was possible.  This is a example of it working just fine as there is no conflict as far as I have found.  I use WiFi to sNTP time on the receiver but not on the transmitter.  The transmitter does not need to know the time.  The receiver does to perform various housekeeping functions with the data.

The transmitter have a built-in jitter of 10 seconds.  That is to try to avoid transmission collisions on the WiFi signal between senders.  It cannot make up for heavy WiFi usage and, still, some collisions occur.  

The display interval is no more than 5 minutes, within a millisecond or so.  This timer is on the receiver and is gated with a timer but will also update the display after both outside sensors have sent in data.  Either/any of the outside transmitters sending data along with the inside sensor will allow the update to happen just a little bit early.  This is not terribly important but gives just a tiny bit fresher data for display.

The transmitters sleep during their waiting period between transmissions.  This reduces the current draw and allows the outside sensors to easily be run by batter for months or a combination of battery and solar charging.  I will present a setup for that later.

The other thing that was new to me on this project is the use of the C++ vector structure.  It has all of the overhead of any array being managed by software but the internals of the operations are hidden behind the interface.  It is easy to use by has depth of operations sufficient to allow almost any data structure to be maintained.  The vector is named TempReading. Probably not the best name but... there you go.  It can hold as much data as you have RAM available to hold it.  You will need 288 data elements for each outdoor sensor.  When it full, the oldest data is eliminated and then the new data is added at the end.  It is used to determin the high and low temperatures over the last 24 hours.  Two data elements are saved.  The temperature reading and the epoch time it was saves.  The epoch is what allows the oldest reading(s) to be deleted each cycle.

There is one unusual thing done in this program.  It was said that this does not work but, in fact, it does work on the Arduino IDE, both versions 1 and 2.  That is a relative include for a common file that the transmitter and receiver need.  I wanted a single copy so it could not get out of sync.  The file is located in the folder above the two folders for the transmitter and receiver code.  It is included thus:

#include "../CommonFile_v2.h".  

Nominally, this is done in a fake library in the libraries folder under the sketch folder name but that's additional installation overhead and I tried this and it works.

As mentioned earlier, the transmitters need to know the MAC address of the receiver.  They do not need to know any IP information to route the packets.  This is handled in the library and, to me, is just plain magic!  I don't understand how it is done and won't try to offer any explanation other than... it works!  The receiver does start up WiFi so it can get the time of day once each day.  The transmitter code does not activate WiFi.

Since the transmitters go into deep sleep between transmissions, some data has to be stored in the RTC memory.  It is limited but not that limited.  I put 5 variables there so that they will survive sleeping and revoot.  One of the four is reset by the system at each bootup.  The other four values are maintained through bootup.  They do not survive power interruption.  But that's not a worry. There is a switch stored in a variable called rtcMagic.  It tells whether some variable have been reset or not.  Silly, really.  It could have just been a boolean value.

There is a backup scheme for resends that extends to a maximum of 30 minutes so the channel will not be cluttered with resends.

At the moment, two different sensors are supported.  The ANT/AHT20 and the BME280.  If either of there are installed, then the values from this sensor will be sent.  If neither are present, fake data will be sent.  No programming change is require to switch between these two sensors.  It was not coded expecint change, just to keep from having to recomile depending on which was used.

For some reason, the receiver must be registered by ESP-NOW before sending.  That's done by the library routines named: esp_now_add_peer().

So the readings are obtained from whichever sensor is installed and the data is filled into the struct and send along.  There are some additional fields in the structure.  There is the packet version number.  As of this writing, it is at version 2.  Also a packet sequence number to avoid multiple entries for a duplicated packet.  And there are counters for total good and total bad sends.  There is a CRC8 (8 bit CRC) computed and send along, too.  That's is recomputed in the receiver and compared before the data is accepted.  Mismatched CRC8's cause the packet to be rejected.

That's the highlights of the send process.  Now, let's move on to the receiver.  It is a little more complex.

The receiver code has additional includes SNTP, TimeLib, WiFi and the C++ vector libraries.  Silly me, I was taught to start counting at 1 so I created all of my arrays with 3 elements insted of 2.  The 0th element is not used.  There is plenty of memory.  Don't worry.

We save the contents of the received packet in various variables.  I should have done it with a struct but with a small project like this, I just didn't do it.  There is an update backoff pair of times for low rate update start and low rate end.  As sent out, there are midnight and 10 am.  The ending time is the last hour to be in low rate.  Then 5 minutes display udpates will resume.

The "staleDelay" is the amount of time the program will keep the last reading before declaring the transmitter dead and clear it.  This causes "n/a" to be shown on the display for the locale (Inside or Outside).

There are two little macros that let you see if a value is in or out of a range of two other numbers.  This is used to determine if we are inside or outside of the slow display update window.  The macro does not care in what order you input the range numbers.  High, Low is just as good as Low, High.  Slick, right?!?!?!

Setup does all of the regular stuff including getting the display prepped, Wifi started and the epoch fetched.  

Loop is where the displaying up the results is done.  All of the variables are setup in Loop, then a display update routine is called to show it on whatever display is desired.  As delivered, it is an ePaper display on a LilyGo T5 v2.13.  To change to a different display, just replace this display formatting routine (last statement in Loop).

The Utilities tab is where all of the hard work is done.  deduceOffsets gets the offsets and other information from the time string as input.  Later, the offset is applied to the epoch to get standard time or the localTime routine is called.  addReading pushes the current reading onto the end of the outdoor vector.  Indoor temps of not saved, only displayed.  printVector is a utility to show the contenct of the readings vector.  OnDataRecv is called when a packet has been received for processing.  It checks the packet sequence number to be sure this is not a repeat and recalculates and compares the CRC8, rejecting the packet if either of these is not what they should be.  It also sets a boolean to show it is running to minimize the mixing of prints from this routine which can run at any time and the loop routine that runs at a varying schedule.  If you have SHOW_DETAILS defined, there is additional output and I prefer to keep them separate.

timtimeSyncCallback is run when a new time is received in response to a timed fetch by the sNTP library code.  It is there just to verify that a new time hack has been received.  IinitTime makes sure that the time is valid before exiting so that nothing will run with a wrong time epoch.  getMinTemp and getMaxTemp do exactly what they say.  The read through the readings vector and fine min and max temps.  Remember that the vector is trimmed to only have 24 hours or readings in it.  CONFIG_FOR_JOE changes the WAP credentials and displays temperature in F instead of C degrees.

It takes a very long time for the first compile pass of this code with all of the ePaper support code.  Maybe I will find something simpler but I don't think that will happen.  If you change to a different display, remove all of GXEDP2 stuff unless you need it for the display you are using.  That might make the compile faster.

Enjoy,
Mikey the Midnight Coder (and, sometimes, in the daytime, too!)
