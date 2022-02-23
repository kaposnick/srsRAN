/**
 * Copyright 2013-2021 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "srsenb/hdr/metrics_csv.h"
#include "srsran/phy/utils/vector.h"

#include <float.h>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>

#include <stdio.h>

using namespace std;

namespace srsenb {

metrics_csv::metrics_csv(std::string filename) : n_reports(0), metrics_report_period(1.0), enb(NULL)
{
  file.open(filename.c_str(), std::ios_base::out);
}

metrics_csv::~metrics_csv()
{
  stop();
}

void metrics_csv::set_handle(enb_metrics_interface* enb_)
{
  enb = enb_;
}

void metrics_csv::stop()
{
  if (file.is_open()) {
    file << "#eof\n";
    file.flush();
    file.close();
  }
}

//static double current_get_timestamp() {
//	auto tp = std::chrono::system_clock::now().time_since_epoch();
//	return std::chrono::duration_cast<std::chrono::milliseconds>(tp).count() * 1e-3;
//}

static bool has_valid_metric_ranges(const enb_metrics_t& m, unsigned index)
{
  if (index >= m.phy.size()) {
    return false;
  }
  if (index >= m.stack.mac.ues.size()) {
    return false;
  }
  if (index >= m.stack.rlc.ues.size()) {
    return false;
  }
  if (index >= m.stack.pdcp.ues.size()) {
    return false;
  }

  return true;
}

void metrics_csv::append_phy_metr(const phy_metrics_t& phy) {
	file << phy.rnti << "|";
	file << phy.ul.pusch_sinr << "|";
	file << phy.ul.pucch_sinr << "|";
	file << phy.ul.rssi << "|";
	file << phy.ul.turbo_iters << "|";
	file << phy.ul.dec_rt << "|";
	file << phy.ul.dec_cpu << "|";
	file << phy.ul.mcs << "|";
	file << phy.ul.n_samples << "|";
	file << phy.ul.n_samples_pucch << "|";
	file << phy.ul.n_samples << "|";
	file << phy.dl.mcs << "|";
	file << phy.dl.n_samples << "|";
}

void metrics_csv::append_mac_metr(const mac_ue_metrics_t& mac) {
	file << mac.nof_tti << "|";
	file << mac.dl_cqi << "|";
	file << mac.tx_brate << "|";
	file << mac.tx_pkts << "|";
	file << mac.tx_errors << "|";
	file << mac.dl_buffer << "|";
	file << mac.rx_brate << "|";
	file << mac.rx_pkts << "|";
	file << mac.rx_errors << "|";
	file << mac.ul_buffer << "|";
	file << mac.phr ;
}

void metrics_csv::set_metrics(const enb_metrics_t& metrics, const uint32_t period_usec)
{
  if (file.is_open() && enb != NULL) {
    if (n_reports == 0) {
      file << "report|period|cpu_usage|pci";
      file << "|rnti|pusch_sinr|pucch_sinr|rssi|turbo_iters|dec_rt|dec_cpu|ul_mcs|ul_nsamples_pusch|ul_nsamples_pucch|ul_nsamples|dl_mcs|dl_nsamples_pdsch";
      file << "|ttis|dl_cqi|dl_bits|dl_pkts|dl_errors|dl_pend_bytes|ul_bits|ul_pkts|ul_errors|ul_pend_bytes|ul_phr";
      file << "\n";
    }

    if (metrics.stack.rrc.ues.size() > 0) {
		uint16_t num_ccs = metrics.stack.mac.cc_info.size();
    	for (unsigned cc_idx = 0; cc_idx != num_ccs; ++cc_idx) {
    		for (unsigned i = 0; i != metrics.stack.rrc.ues.size(); i++) {
    	    		if (!has_valid_metric_ranges(metrics, i)) {
    	    			continue;
    	    		}

    	    		if (metrics.stack.mac.ues[i].cc_idx != cc_idx) {
    	    			continue;
    	    		}

    	    		for(auto it = metrics.phy.begin(); it != metrics.phy.end(); ++it) {
    	    			auto& phy_metr = *it;
    	    			if (phy_metr.rnti == metrics.stack.mac.ues[i].rnti) {
    	    				file << n_reports << "|";
    	    				file << (period_usec * 1e-3) << "|";
    	    				file << metrics.sys.current_cpu_usage << "|";
    	        	    	file << metrics.stack.mac.cc_info[cc_idx].pci << "|";
    	    				append_phy_metr(phy_metr);
    	    				append_mac_metr(metrics.stack.mac.ues[i]);
							file << "\n";
							file.flush();
    	    				break;
    	    			}
    	    		}
    	    	}
    	    }
    }
    n_reports++;

  } else {
    std::cout << "Error, couldn't write CSV file." << std::endl;
  }
}

std::string metrics_csv::float_to_string(float f, int digits, bool add_semicolon)
{
  std::ostringstream os;
  const int          precision = (f == 0.0) ? digits - 1 : digits - log10f(fabs(f)) - 2 * DBL_EPSILON;
  os << std::fixed << std::setprecision(precision) << f;
  if (add_semicolon)
    os << ';';
  return os.str();
}



} // namespace srsenb
