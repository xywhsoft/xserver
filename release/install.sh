#!/bin/bash

ln -s /home/www/xs.service /etc/systemd/system/xs.service
sudo systemctl daemon-reload
sudo systemctl enable xs.service
sudo systemctl start xs.service
