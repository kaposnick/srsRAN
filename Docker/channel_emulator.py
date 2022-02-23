#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: Intra Handover Flowgraph
# GNU Radio version: 3.8.1.0

from distutils.version import StrictVersion

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print("Warning: failed to XInitThreads()")

from gnuradio import channels
from gnuradio.filter import firdes
from gnuradio import gr
import sys
import signal
from PyQt5 import Qt
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
from gnuradio import zeromq
from gnuradio.qtgui import Range, RangeWidget
from gnuradio import qtgui

class intra_enb(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Intra Handover Flowgraph")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Intra Handover Flowgraph")
        qtgui.util.check_set_qss()
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except:
            pass
        self.top_scroll_layout = Qt.QVBoxLayout()
        self.setLayout(self.top_scroll_layout)
        self.top_scroll = Qt.QScrollArea()
        self.top_scroll.setFrameStyle(Qt.QFrame.NoFrame)
        self.top_scroll_layout.addWidget(self.top_scroll)
        self.top_scroll.setWidgetResizable(True)
        self.top_widget = Qt.QWidget()
        self.top_scroll.setWidget(self.top_widget)
        self.top_layout = Qt.QVBoxLayout(self.top_widget)
        self.top_grid_layout = Qt.QGridLayout()
        self.top_layout.addLayout(self.top_grid_layout)

        self.settings = Qt.QSettings("GNU Radio", "intra_enb")

        try:
            if StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
                self.restoreGeometry(self.settings.value("geometry").toByteArray())
            else:
                self.restoreGeometry(self.settings.value("geometry"))
        except:
            pass

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 1.92e6
        self.noise_level = noise_level = 0
        self.base_rate = base_rate = 23040000

        ##################################################
        # Blocks
        ##################################################
        self._noise_level_range = Range(0, 0.3, 0.001, 0, 200)
        self._noise_level_win = RangeWidget(self._noise_level_range, self.set_noise_level, 'noise_level', "counter_slider", float)
        self.top_grid_layout.addWidget(self._noise_level_win)
        self.zeromq_req_source_1_0 = zeromq.req_source(gr.sizeof_gr_complex, 1, 'tcp://localhost:2101', 100, False, -1)
        self.zeromq_req_source_1 = zeromq.req_source(gr.sizeof_gr_complex, 1, 'tcp://localhost:2001', 100, False, -1)
        self.zeromq_rep_sink_1_0 = zeromq.rep_sink(gr.sizeof_gr_complex, 1, 'tcp://*:2000', 100, False, -1)
        self.zeromq_rep_sink_1 = zeromq.rep_sink(gr.sizeof_gr_complex, 1, 'tcp://*:2100', 100, False, -1)
        self.channels_channel_model_0_0 = channels.channel_model(
            noise_voltage=noise_level,
            frequency_offset=0.0,
            epsilon=1.0,
            taps=[1.0 + 1.0j],
            noise_seed=0,
            block_tags=False)
        self.channels_channel_model_0 = channels.channel_model(
            noise_voltage=noise_level,
            frequency_offset=0.0,
            epsilon=1.0,
            taps=[1.0 + 1.0j],
            noise_seed=0,
            block_tags=False)



        ##################################################
        # Connections
        ##################################################
        self.connect((self.channels_channel_model_0, 0), (self.zeromq_rep_sink_1_0, 0))
        self.connect((self.channels_channel_model_0_0, 0), (self.zeromq_rep_sink_1, 0))
        self.connect((self.zeromq_req_source_1, 0), (self.channels_channel_model_0_0, 0))
        self.connect((self.zeromq_req_source_1_0, 0), (self.channels_channel_model_0, 0))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "intra_enb")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate

    def get_noise_level(self):
        return self.noise_level

    def set_noise_level(self, noise_level):
        print('Noise level: ' + str(noise_level))
        self.noise_level = noise_level
        self.channels_channel_model_0.set_noise_voltage(self.noise_level)
        self.channels_channel_model_0_0.set_noise_voltage(self.noise_level)

    def get_base_rate(self):
        return self.base_rate

    def set_base_rate(self, base_rate):
        self.base_rate = base_rate

import sys

def input_thread(tb):
    for line in sys.stdin:
        noise_level = line
        tb.set_noise_level(float(noise_level))

import threading

def main(top_block_cls=intra_enb, options=None):

    if StrictVersion("4.5.0") <= StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()
    tb.start()
    tb.show()
    

    def sig_handler(sig=None, frame=None):
        Qt.QApplication.quit()

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    timer = Qt.QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)

    threading.Thread(target=input_thread(tb)).start()


    def quitting():
        tb.stop()
        tb.wait()
    qapp.aboutToQuit.connect(quitting)
    qapp.exec_()



if __name__ == '__main__':
    main()
