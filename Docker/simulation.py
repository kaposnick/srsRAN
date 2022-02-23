#!/usr/bin/python3

import subprocess
import signal
from time import sleep
from sys import stderr
import numpy as np

iperf_server_address = '192.168.1.29'
iperf_duration = '100' # in seconds

def run_simulation(noise_level, cpu_quota):
    print("=== Running simulation for CPU Quota: {}, Noise Level: {} ===".format(cpu_quota, noise_level))
    cfs_period = 500000
    cfs_quota = str(int(cfs_period * cpu_quota))

    # cmd = ['/usr/bin/python3', '/home/naposto/phd/srsRAN/Docker/channel_emulator.py']
    # proc_grc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    # print('Started GRC [{}]...'. format(proc_grc.pid))
    # proc_grc.communicate(input=b'0.02\n')
    # # to_send = '{}\n'.format(noise_level)
    # # proc_grc.stdin.write(b'0.02\n')
    # # proc_grc.stdin.flush()

    # while True:
    #     output = proc_grc.stdout.readline()



    proc_iperf_server = subprocess.Popen(['/usr/bin/iperf', '-s'], stdout=subprocess.DEVNULL)
    print('Started iperf server [{}]... '.format(proc_iperf_server.pid))

    with open('/sys/fs/cgroup/cpu/enb/cpu.cfs_quota_us', 'w') as outfile:
        subprocess.Popen(['/usr/bin/echo', cfs_quota], stdout=outfile)

    cmd = ['/home/naposto/bin/srsran/bin/srsepc']
    cmd.append('/home/naposto/.config/srsran/epc.conf')
    cmd.append('--hss.db_file')
    cmd.append('/home/naposto/.config/srsran/user_db.csv')
    proc_epc = subprocess.Popen(cmd, stdout=subprocess.PIPE, universal_newlines=True)
    print('Started srsepc [{}]... '.format(proc_epc.pid), end='')
    while True:
        output = proc_epc.stdout.readline()
        if ('SP-GW Initialized' in output):
            print('EPC Initialized successfully...')
            break

    cmd = ['/usr/bin/python3', '/home/naposto/phd/srsRAN/Docker/channel_emulator.py']
    proc_grc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    print('Started GRC [{}]...'. format(proc_grc.pid))

    cmd = ['/home/naposto/bin/srsran/bin/srsenb']
    cmd.append('/home/naposto/.config/srsran/enb.conf')
    cmd.append('--enb_files.sib_config')
    cmd.append('/home/naposto/.config/srsran/sib.conf')
    cmd.append('--enb_files.rr_config')
    cmd.append('/home/naposto/.config/srsran/rr.conf')
    cmd.append('--enb_files.rb_config')
    cmd.append('/home/naposto/.config/srsran/rb.conf')
    cmd.append('--log.phy_level=info')
    cmd.append('--log.filename=/home/naposto/phd/nokia/new_data/csv_2/cpu_{}_noise_{}.log'.format(cpu_quota, noise_level))
    cmd.append('--rf.device_name=zmq')
    cmd.append('--rf.device_args=fail_on_disconnect=true,tx_port=tcp://*:2101,rx_port=tcp://localhost:2100,id=enb,base_srate=23.04e6')

    proc_enb = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    print('Started srsenb [{}]... '.format(proc_enb.pid), end='')

    while True:
        output = proc_enb.stdout.readline()
        if ('==== eNodeB started' in output.strip()):
            with open('/sys/fs/cgroup/cpu/enb/cgroup.procs', 'w') as outfile:
                subprocess.Popen(['/usr/bin/echo', str(proc_enb.pid)], stdout=outfile)
            print('Process eNB successfully started...')
            break
        return_code = proc_enb.poll()
        if return_code is not None:
            print('RETURN CODE', return_code)
            # Process has finished, read rest of the output 
            for output in proc_enb.stdout.readlines():
                print(output.strip())
            break

    cmd = ['/home/naposto/bin/srsran/bin/srsue']
    cmd.append('/home/naposto/.config/srsran/ue.conf')
    cmd.append('--gw.netns=ue')
    cmd.append('--rf.device_name=zmq')
    cmd.append('--rf.device_args=fail_on_disconnect=true,tx_port=tcp://*:2001,rx_port=tcp://localhost:2000,id=ue,base_srate=23.04e6')
    proc_ue = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr = subprocess.PIPE, universal_newlines=True)

    print('Started srsue [{}]... '.format(proc_ue.pid), end='')
    while True:
        output = proc_ue.stdout.readline().strip()
        # print(output)
        if ('Network attach successful.' in output):
            print('srsUE connected to the eNB successfully')
            print('Set the noise level and starting the iperf test')
            if (noise_level != 0):
                print('Setting new noise level to {}'.format(noise_level))
                proc_grc.stdin.write('{}\n'.format(noise_level).encode("utf-8"))
                proc_grc.stdin.flush()
            subprocess.Popen(['/usr/sbin/ip', 'netns', 'exec', 'ue', '/usr/sbin/ip', 'route', 'add', 'default', 'dev', 'tun_srsue'])
            iperf_client_process = subprocess.Popen(['/usr/sbin/ip', 'netns', 'exec', 'ue', '/usr/bin/iperf', '-c', iperf_server_address, '-t', iperf_duration], stdout=subprocess.PIPE, stderr = subprocess.PIPE, universal_newlines=True)
            print('Started iperf client [{}]...'.format(iperf_client_process.pid))
            iperf_client_process.wait()
            print('Stopping iperf client')
            proc_iperf_server.send_signal(signal.SIGINT)
            proc_iperf_server.wait()
            sleep(10)
            print('Stopping iperf server')
            break

    print('Stopping srsue...')
    proc_ue.send_signal(signal.SIGINT)

    print('Stopping srsenb...')
    proc_enb.send_signal(signal.SIGINT)

    print('Killing GRC...')
    proc_grc.kill()

    print('Stopping srsepc...')
    proc_epc.send_signal(signal.SIGINT)

    proc_ue.wait()
    proc_enb.wait()
    proc_epc.wait()

arr_cpu_quota = [0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0]
arr_noise_level = ['0.000', '0.004', '0.008', '0.012', '0.016', '0.02', '0.04', '0.06', '0.08', '0.10', '0.13', '0.16', '0.20', '0.23', '0.26', '0.30']

# run_simulation(0.22, 1.0)

run_simulation('0.16', 1.4)
run_simulation('0.04', 1.4)

# for cpu_quota in arr_cpu_quota:
#     for noise_level in arr_noise_level:
#         run_simulation(noise_level, cpu_quota)







