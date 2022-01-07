#!/bin/bash

sudo /home/naposto/bin/srsran/bin/srsenb /home/naposto/.config/srsran/enb.conf \
    --enb_files.sib_config /home/naposto/.config/srsran/sib.conf \
    --enb_files.rr_config /home/naposto/.config/srsran/rr.conf \
    --enb_files.rb_config /home/naposto/.config/srsran/rb.conf \
    --expert.metrics_http_scrape_enable=true \
    --expert.metrics_http_scrape_port=8080 \
    --rf.device_name=zmq \
    --rf.device_args="fail_on_disconnect=true,tx_port=tcp://*:2000,rx_port=tcp://localhost:2001,id=enb,base_srate=23.04e6" 