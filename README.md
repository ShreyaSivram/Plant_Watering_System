# Plant_Watering_System
This project integrates an ESP32 microcontroller with an OLED display and a relay to create an automated plant watering system controlled via a web interface. The system connects to a WiFi network, retrieves time updates from an NTP server, and allows users to set watering times through a web page. The web interface includes functionalities to toggle the relay state and update watering schedules dynamically. Real-time status updates are displayed on the OLED screen, showing current time, scheduled watering times, and the state of the relay. This setup provides a versatile platform for remotely managing and automating plant watering tasks based on user-defined schedules.
Connect the circuit in the following way:
ESP32                   Breadboard
3.3v-                      OLED VCC
GND-                     OLED GND
D22-	             OLED SCL
D21- 	             OLED SCA
3.3v-                      Relay VCC
GND- 	             Relay GND
D33- 	             Relay IN


Make the necessary connections and upload the code onto the ESP32 board.
After uploading, check the serial monitor to see if the board is connected to the WIFI.
Now, check if the OLED is displaying the correct time
If yes, go on to copy and paste the IP address generated onto any internet browser.
Set the time and use the toggle to control the relay, thereby the motor.
Thus, your project is ready.



