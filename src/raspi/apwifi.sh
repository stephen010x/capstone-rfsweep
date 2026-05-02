#!/bin/bash

# to reset wifi, delete the /root/key file

ssid="rfpi"
pass="pass"



fkey="/home/user/key/wifi"
profile="raspi-ap"
profile-static="raspi-static"
# wlan-ip="10.42.0.1/24"
#eth-ip="192.168.50.1/24"


mkdir -p $(dirname -- "$fkey")
chown -R user $(dirname -- "$fkey")


# dump wifi status
echo '======== wifi info ================================'
ip link show
echo '---------------------------------------------------'
nmcli device status
echo '---------------------------------------------------'
nmcli connection show
echo '---------------------------------------------------'
lsusb
echo '==================================================='



# if key file doesn't exist, then...
if [ ! -f "$fkey" ]; then

    # create new key file
    touch $fkey

    # delete existing access point profile
    nmcli con delete "$profile"
    nmcli con delete "$profile-static"

    # setup wifi access point
    nmcli con add con-name "$profile" ifname wlan0 type wifi ssid "$ssid"
    nmcli con modify "$profile" 802-11-wireless.mode ap 802-11-wireless.band bg \
        ipv4.method shared

    # setup password (or no password)
    # no password not working
    if [ -n "$pass" ]; then
        nmcli con modify "$profile" wifi-sec.key-mgmt wpa-psk wifi-sec.psk "$pass"
    else
        nmcli con modify "$profile" wifi-sec.key-mgmt none
        nmcli con modify "$profile" 802-11-wireless-security.key-mgmt none
        nmcli con modify "$profile" -802-11-wireless-security.psk \
            -802-11-wireless-security.wep-key0 -802-11-wireless-security.wep-key1 \
            -802-11-wireless-security.wep-key2 -802-11-wireless-security.wep-key3 \
            -802-11-wireless-security.auth-alg
    fi

    # setup ethernet access point
    nmcli con add con-name "$profile-static" type ethernet ifname eth0
    nmcli con modify "$profile-static" ipv4.method shared ipv6.method ignore

    sudo nmcli con modify "$profile" connection.autoconnect yes
    sudo nmcli con modify "$profile-static" connection.autoconnect yes
    
fi


# begin broadcasting access point
#nmcli connection up "$profile-static"
nmcli connection up "$profile"
