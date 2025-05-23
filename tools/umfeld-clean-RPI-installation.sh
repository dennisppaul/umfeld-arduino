$USER_HOME=/home/umfeld
sudo bash -c '
rm -f $USER_HOME/.bash_history $USER_HOME/.zsh_history /root/.bash_history /root/.zsh_history
rm -f /etc/ssh/ssh_host_*
rm -f /etc/wpa_supplicant/wpa_supplicant.conf
rm -f /var/lib/dhcp/* /etc/udev/rules.d/70-persistent-net.rules
rm -rf $USER_HOME/.cache/* $USER_HOME/.local/share/* /tmp/* /var/tmp/*
journalctl --rotate
journalctl --vacuum-time=1s
rm -rf /var/log/*.gz /var/log/*.1 /var/log/*.old
truncate -s 0 /var/log/*.log
apt clean
rm -f $USER_HOME/.ssh/authorized_keys
unset HISTFILE
history -c
sync
echo "System cleaned. You can now shut down and image the SD card."
'