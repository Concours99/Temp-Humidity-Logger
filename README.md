# Temp-Humidity-Logger
Arduino code for temp-humidty logging to ThingSpeak
Questions to geiserw@gmail.com

In addition to the TempHumid_logger.ino file with the Arduino code in it, you
will need to supply a file named SecretStuff.h.
In it will be two lines:

#define SSID "The name of your local WiFi network"
#define PASS "The password for your local WiFi network"

I did not include this file for obvious reasons.

TempHumid_logger CaseBottom.stl
TempHumid_logger CaseTop.stl

These files are for a case I designed and created on my 3d printer.
Obviously, you will need to layout the circuit board exactly as I have to
make the top fit.

Change log:
2023-Jan-05
	Corrected the problem of not being able to connect to WiFi when power
	was lost and returned.  The WiFi would take longer to come back than the
	arduino and the code to wait for the connection was broken.
	Also added a call to reset the arduino if there is an error updating the
	data as it probably means the WiFi was lost without losing power.