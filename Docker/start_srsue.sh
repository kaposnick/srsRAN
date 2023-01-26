#!/bin/bash

sudo /home/naposto/tools/srsRAN/bin/srsue /home/naposto/.config/srsran/ue.conf \
    --gw.netns=ue \
    --log.phy_level=warning \
    --log.phy_lib_level=warning \
    --log.filename='/tmp/ue.log' \
    --rf.device_name=zmq \
    --rf.device_args="fail_on_disconnect=true,tx_port=tcp://*:2001,rx_port=tcp://localhost:2000,id=ue,base_srate=23.04e6"
#    --log.phy_level=debug \
#    --log.rf_level=debug \
#    --channel.ul.enable=true \
#    --channel.ul.awgn.enable=true \
#    --channel.ul.awgn.signal_power=0 \
#    --channel.ul.awgn.snr=6 \
