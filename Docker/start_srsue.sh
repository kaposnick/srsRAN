#!/bin/bash

sudo /home/naposto/bin/srsran/bin/srsue /home/naposto/.config/srsran/ue.conf \
    --rf.device_name=zmq \
    --rf.device_args="fail_on_disconnect=true,tx_port=tcp://*:2001,rx_port=tcp://localhost:2000,id=enb,base_srate=23.04e6" 