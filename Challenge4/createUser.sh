#!/bin/bash
#
username="$1"
pass="$2"

sudo useradd -s /bin/bash -G sudo,MayIAccounts "$username"
echo "$username:$pass" | sudo chpasswd
