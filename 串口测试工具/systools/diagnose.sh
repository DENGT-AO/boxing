#!/bin/sh

echo "---------------------------------------------"
echo "      * meminfo"
cat /proc/meminfo
echo "---------------------------------------------"
echo "      * uptime & loadavg"
uptime
echo "---------------------------------------------"
echo "      * usb devices"
cat /proc/bus/usb/devices
cat /sys/kernel/debug/usb/devices
#echo "---------------------------------------------"
#echo "      * config"
#cat /etc/inos.conf
echo "---------------------------------------------"
echo "      * messages"
cat /var/log/messages
echo "---------------------------------------------"
echo "      * old messages"
cat /var/log/messages.0
echo "---------------------------------------------"
echo "      * messages in flash"
cat /var/backups/messages
echo "---------------------------------------------"
echo "      * old messages in flash"
cat /var/backups/messages.0
echo "---------------------------------------------"
echo "      * alarms in flash"
cat /var/backups/alarm.log
echo "---------------------------------------------"
echo "      * ps"
ps
echo "---------------------------------------------"
echo "      * file-nr"
cat /proc/sys/fs/file-nr
echo "---------------------------------------------"
echo "      * fd"
ls -l /proc/*/fd
echo "---------------------------------------------"
echo "      * netstat"
netstat -anp
echo "---------------------------------------------"
echo "      * iptables -t filter"
iptables -L -n -v
echo "---------------------------------------------"
echo "      * iptables -t nat"
iptables -L -n -v -t nat
echo "---------------------------------------------"
echo "      * iptables -t mangle"
iptables -L -n -v -t mangle
echo "---------------------------------------------"
echo "      * arp"
arp -a
echo "---------------------------------------------"
echo "      * ifconfig"
ifconfig
echo "---------------------------------------------"
echo "      * ip link"
ip link
echo "---------------------------------------------"
echo "      * ip addr"
ip addr
echo "---------------------------------------------"
echo "      * ip route"
ip route
echo "---------------------------------------------"
echo "      * ip route show cache"
ip route show cache
echo "---------------------------------------------"
echo "      * conntrack -L"
conntrack -L
#echo "---------------------------------------------"
#echo "      * ip xfrm state"
#ip xfrm state
echo "---------------------------------------------"
echo "      * ip xfrm show policy"
ip xfrm policy show
echo "---------------------------------------------"
echo "      * ipsec status"
ipsec statusall
whack --status
echo "---------------------------------------------"
echo "      * ipsec cert status"
ipsec listcacerts
ipsec listcerts
whack --listall
echo "---------------------------------------------"
