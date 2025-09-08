Quick Start
===========

1) Setup python virtual env using requirement.txt

2) Ensure you have the sshpass package installed

    sudo apt install sshpass

3) Add your fresh RAPI's IP to the inventory.ini file.

4) Activate / create Python venv to install ansible

    python3 -m venv venv
    . venv/bin/activate
    pip install -r requirements.txt

5) Verify you can ping it:

    ansible pycontrol -m ping -i inventory.ini -kK

6) Run the playbook

    ansible-playbook -i inventory.ini playbook.yaml -k

7) Run the playbook for things with a specific tag

    ansible-playbook -i inventory.ini playbook.yaml -k --tags wifi

8) To skip some cleaning tasks that you may have already run, specify tags to skip tasks with the clean tag:

    ansible-playbook -i inventory.ini playbook.yaml -k --skip-tags "clean"

Variables
=========

You can find various settings in the variable section of inventory.ini

```yaml

[all:vars]
wlan0_ip=192.168.26.1
dhcp_pool=192.168.26.100,192.168.26.120
ssid=PyControl
password=Spain2026
username=nhilton
```

