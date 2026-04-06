# lilygo-t-hmi

- 3D Model for LilyGo T-HMI Enclosure. I guess this must be a niche device as I could not find an existing model anywhere so had to create one. It has cut outs for the buttons, sd slot and socket pin things, and plenty of space for a fat USB-C cable.
- Test app, shows a joke of the day. Refresh (or wake) by pressing the screen. The screen will sleep after 10 minutes.

# Flashing the device
```
pio run -e HMI -t upload
```
(or use the Arduino IDE)
