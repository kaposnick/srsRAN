#!/bin/bash


# echo 500000 > /sys/fs/cgroup/cpu,cpuacct/enb/cpu.cfs_period_us
# echo 375000 > /sys/fs/cgroup/cpu,cpuacct/enb/cpu.cfs_quota_us

# cpu=0.75
# noise=0.40

sudo /home/naposto/tools/srsRAN/bin/srsenb /home/naposto/.config/srsran/enb.conf \
    --enb_files.sib_config /home/naposto/.config/srsran/sib.conf \
    --enb_files.rr_config /home/naposto/.config/srsran/rr.conf \
    --enb_files.rb_config /home/naposto/.config/srsran/rb.conf \
    --expert.pusch_beta_factor=1 \
    --log.all_level=warning \
    --log.filename='/tmp/enb.log' \
    --rf.device_name=zmq \
    --rf.device_args="fail_on_disconnect=true,tx_port=tcp://*:2001,rx_port=tcp://localhost:2000,id=enb,base_srate=23.04e6" 

    # --scheduler.policy=time_sched_ai \
    # --rf.device_args="fail_on_disconnect=true,tx_port=tcp://*:2000,rx_port=tcp://localhost:2001,id=enb,base_srate=23.04e6"     
    # --scheduler.pusch_max_mcs=19 \
    # --log.phy_level=warning \
    # --log.mac_level=warning \
    # --log.phy_level=info \

## //    --expert.metrics_csv_enable=true \
##    --expert.metrics_period_secs=1.0 \
##    --scheduler.pdsch_mcs=25 \
##    --expert.metrics_http_scrape_enable=true \
##    --expert.metrics_http_scrape_port=8080 \
##    --log.phy_level=info \
##    --log.filename=/home/naposto/phd/nokia/data/enb.log \
# enb_pid=$!
# echo ${enb_pid} > /sys/fs/cgroup/cpu,cpuacct/enb/cgroup.procs

# while true; do read; done


# trap "trap - SIGTERM && kill -- -$$" SIGINT SIGTERM EXIT

#    --log.phy_lib_level=debug \
#    --log.mac_level=debug \
#   --expert.pusch_max_its 20 \
