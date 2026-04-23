# TODO

## FIXES
- [x] Fix recorded angle calculation to actual rather than ideal
- [x] Ensure that changing stepper_mode will not lose steps
- [x] Add assertion to stepper_mode that no steps are lost
- [x] Fix server crashing when disabling transmitter
- [-] Fine tune motor delay

## TESTS
- [ ] Test with thread sanatizer (both client and server)
- [ ] Test various transmitter script and rfsweep transmitter signal amplitude values

## FEATURES
- [x] Remove --snap flag and features. Replace with --stepmode
- [x] Make rotation back to origin fullspeed
- [x] Update defaults help string
- [x] Add verbose option -v
- [ ] Ethernet support
- [ ] Setup server loop and log script
- [ ] Add cad models to a dir on the git repo (cleanup the kicad files)
- [ ] Write the actual readme/documentation
- [ ] Make default for band_hz be 0.75% of srate_hz
- [ ] Add --clock flag to enable receiver/transmitter clock
- [ ] Have scripts poll server for error logs when something doesn't work
- [ ] Add copyover for raspi systemd startup and other scripts

---

## LOW-PRIORITY (probably will not add)
- [ ] Command to update the project's git remotely
- [ ] Return direct error messages to the client
- [ ] More robust receiver/transmitter hackrf selection
- [ ] Consider making file output default and add --stdout flag
- [ ] Optionally concise flags like -aFq -s -S etc.
- [ ] Hackrf support on windows (if you wanted to connect a transmitter from windows)
- [ ] Add a way to reduce or increase the bytes read for each angle
- [ ] Add motor speed setting to rotate and measure mode
- [ ] Modify makefile to output binary into the tmp folder, and copy to bin.
- [ ] Have motors run continuously, uninterupted by samples, and just space out samples
        by a time, and record the actual angle for measurements.
- [ ] Optimize motor delays such that smaller delays are ignored, allowing the motor
        to rotate faster.
- [ ] Consider flags for measure command to enable transmitter for measurements
- [ ] Add --no-amplify and --ascii flags that do nothing
