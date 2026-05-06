# TODO

## FIXES
- [x] Fix recorded angle calculation to actual rather than ideal
- [x] Ensure that changing stepper_mode will not lose steps
- [x] Add assertion to stepper_mode that no steps are lost
- [x] Fix server crashing when disabling transmitter
- [x] Fine tune motor delay
- [x] Make file output replace rather than append
- [x] Fix process.py plots.png output

## TESTS
- [x] Test text file with process.py
- [ ] Test various transmitter script and rfsweep transmitter signal amplitude values
- [ ] Test all 4 run scripts with new transmit and error log features
- [ ] Make sure that running `rfsweep receive` won't return back to origin

## FEATURES
- [x] Remove --snap flag and features. Replace with --stepmode
- [x] Make rotation back to origin fullspeed
- [x] Update defaults help string
- [x] Add verbose option -v
- [x] Ethernet support
- [x] Setup server loop and log script
- [x] Make default for band_hz be 0.75% of srate_hz
- [x] Add --clock flag to enable receiver/transmitter clock
- [x] Add copyover for raspi systemd startup and other scripts
- [x] Add separate transmit script, and transmitter code to fastrun
- [x] Add to scripts to print last few lines of logs if error
- [x] Add cad models to a dir on the git repo (cleanup the kicad files)
- [x] Add to scripts to try for wlan first, then eth, then localhost
    - [ ] Find and add default eth IP address to scripts
- [ ] Write the actual readme/documentation
- [ ] Add version number to help prints

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
- [ ] Have --binary and --file=data.bin be default
- [ ] Have --clock flag fail when transmitter is either disabled or --clock flag wasn't 
        also specified for the transmitter
- [ ] Update shell scripts to match batch scripts
- [ ] Verify that the clockout is detected with hackrf_get_clkin_status
- [ ] Emit server error if the clockout for both hackrfs are detected
- [ ] Add interactive mode to rfsweep (have it be default, with a flag to disable)
- [ ] Rename --tx-ampl to --tx-mag or --magnitude
- [ ] GUI Interface (Not happening)
- [ ] Have default ip address be the wlan0 or eth0 connection
    - [ ] Have ip address default to eth0 then wlan0 then localhost
- [ ] Test with thread sanatizer (both client and server)
