#!/bin/bash

sudo /home/naposto/bin/srsran/bin/srsue /home/naposto/.config/srsran/ue.conf \
    --gw.netns=ue2 \
    --rf.device_name=zmq \
    --usim.algo=mil \
    --usim.k=00112233445566778899aabbccddeeff \
    --usim.opc=63bfa50ee6523365ff14c1f45f88737d \
    --usim.imsi=001010123456780 \
    --usim.imei=353490069873320 \
    --rf.device_args="fail_on_disconnect=true,tx_port=tcp://*:2201,rx_port=tcp://localhost:2200,id=ue,base_srate=23.04e6"
#    --log.phy_level=debug \
#    --log.rf_level=debug \
#    --channel.ul.enable=true \
#    --channel.ul.awgn.enable=true \
#    --channel.ul.awgn.signal_power=0 \
#    --channel.ul.awgn.snr=6 \