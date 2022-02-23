#!/bin/bash

sudo /home/naposto/bin/srsran/bin/srsue /home/naposto/.config/srsran/ue.conf \
    --gw.netns=ue \
    --rf.device_name=zmq \
    --rf.device_args="fail_on_disconnect=true,tx_port=tcp://*:2101,rx_port=tcp://localhost:2100,id=ue,base_srate=23.04e6"
#    --log.phy_level=debug \
#    --log.rf_level=debug \
#    --channel.ul.enable=true \
#    --channel.ul.awgn.enable=true \
#    --channel.ul.awgn.signal_power=0 \
#    --channel.ul.awgn.snr=6 \