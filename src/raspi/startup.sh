#!/bin/bash

user="user"

# disable swap
swapoff -a

# log output file
flog="/home/$user/log.txt"

# make sure log file exists
touch $flog
chown $user $flog

# setup wifi, output stdout to log
source /home/$user/apwifi.sh &>$flog

# start rfsweep server
# loop infinitely in case of errors
while true; do
    echo >>$flog
    echo >>$flog
    /home/$user/capstone-rfsweep/bin/rfsweep server --log=$flog -v &>>$flog
done

#nmcli con down raspi-ap &>>$flog
#sleep 30
#nmcli con show &>>$flog
#nmcli con up netplan-wlan0-TurtlePhone &>>$flog
#nmcli device wifi list &>>$flog
