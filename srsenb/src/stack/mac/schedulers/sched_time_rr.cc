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

#include "srsenb/hdr/stack/mac/schedulers/sched_time_rr.h"
#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cstring>

namespace srsenb {

sched_time_rr::sched_time_rr(const sched_cell_params_t& cell_params_, const sched_interface::sched_args_t& sched_args)
{
  cc_cfg = &cell_params_;

  // create the pipe
	mkfifo(fifo_out, 0666);
	fd_to_ai_sched = open(fifo_out, O_CREAT | O_WRONLY, 0666);
	if (fd_to_ai_sched < 0) {
		std::cerr << "Error opening " << fd_to_ai_sched << std::endl;
	}

	mkfifo(fifo_in, 0666);
	fd_from_ai_sched = open(fifo_in, O_RDONLY);
	if (fd_from_ai_sched < 0) {
		std::cerr << "Error opening " << fd_from_ai_sched << std::endl;
	}
}

void sched_time_rr::sched_update_beta_factor(uint32_t beta_factor) {
	std::cout << "SCHED AI Updating beta factor " << beta_factor << std::endl;
	this->beta_factor = beta_factor;
}

/*****************************************************************
 *                         Dowlink
 *****************************************************************/

void sched_time_rr::sched_dl_users(sched_ue_list& ue_db, sf_sched* tti_sched)
{
  if (ue_db.empty()) {
    return;
  }

  // give priority in a time-domain RR basis.
  uint32_t priority_idx = tti_sched->get_tti_tx_dl().to_uint() % (uint32_t)ue_db.size();
  sched_dl_retxs(ue_db, tti_sched, priority_idx);
  sched_dl_newtxs(ue_db, tti_sched, priority_idx);
}

void sched_time_rr::sched_dl_retxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx)
{
  auto iter = ue_db.begin();
  std::advance(iter, prio_idx);
  for (uint32_t ue_count = 0; ue_count < ue_db.size(); ++iter, ++ue_count) {
    if (iter == ue_db.end()) {
      iter = ue_db.begin(); // wrap around
    }
    sched_ue&           user = *iter->second;
    const dl_harq_proc* h    = get_dl_retx_harq(user, tti_sched);
    // Check if there is a pending retx
    if (h == nullptr) {
      continue;
    }
    try_dl_retx_alloc(*tti_sched, user, *h);
  }
}

void sched_time_rr::sched_dl_newtxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx)
{
  auto iter = ue_db.begin();
  std::advance(iter, prio_idx);
  for (uint32_t ue_count = 0; ue_count < ue_db.size(); ++iter, ++ue_count) {
    if (iter == ue_db.end()) {
      iter = ue_db.begin(); // wrap around
    }
    sched_ue& user = *iter->second;
    if (user.enb_to_ue_cc_idx(cc_cfg->enb_cc_idx) < 0) {
      continue;
    }
    const dl_harq_proc* h = get_dl_newtx_harq(user, tti_sched);
    // Check if there is an empty harq for the newtx
    if (h == nullptr) {
      continue;
    }
    if (try_dl_newtx_alloc_greedy(*tti_sched, user, *h) == alloc_result::no_cch_space) {
      logger.info("SCHED: Couldn't find space in PDCCH/PUCCH for DL tx for rnti=0x%x", user.get_rnti());
    }
  }
}

/*****************************************************************
 *                         Uplink
 *****************************************************************/

void sched_time_rr::sched_ul_users(sched_ue_list& ue_db, sf_sched* tti_sched)
{
  if (ue_db.empty()) {
    return;
  }
  // give priority in a time-domain RR basis.
  uint32_t priority_idx = tti_sched->get_tti_tx_ul().to_uint() % (uint32_t)ue_db.size();
  sched_ul_retxs(ue_db, tti_sched, priority_idx);
  sched_ul_newtxs(ue_db, tti_sched, priority_idx);
}

void sched_time_rr::sched_ul_retxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx)
{
  auto iter = ue_db.begin();
  std::advance(iter, prio_idx);
  for (uint32_t ue_count = 0; ue_count < ue_db.size(); ++iter, ++ue_count) {
    if (iter == ue_db.end()) {
      iter = ue_db.begin(); // wrap around
    }
    sched_ue&           user = *iter->second;
    const ul_harq_proc* h    = get_ul_retx_harq(user, tti_sched);
    // Check if there is a pending retx
    if (h == nullptr) {
      continue;
    }
    alloc_result code = try_ul_retx_alloc(*tti_sched, user, *h);
    if (code == alloc_result::no_cch_space) {
      logger.debug("SCHED: Couldn't find space in PDCCH for UL retx of rnti=0x%x", user.get_rnti());
    }
  }
}

void sched_time_rr::sched_ul_newtxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx)
{
  auto iter = ue_db.begin();
  std::advance(iter, prio_idx);
  for (uint32_t ue_count = 0; ue_count < ue_db.size(); ++iter, ++ue_count) {
    if (iter == ue_db.end()) {
      iter = ue_db.begin(); // wrap around
    }
    sched_ue&           user = *iter->second;
    const ul_harq_proc* h    = get_ul_newtx_harq(user, tti_sched);
    // Check if there is a empty harq
    if (h == nullptr) {
      continue;
    }

    /*
      We do this as to resemble the way sched_ai works. In return,
      we receive a fake decision (mcs, prb) will is discarded.
    */
    srsenb::tti_point tti_tx_ul = tti_sched->get_tti_tx_ul();
    uint32_t pending_data = user.get_pending_ul_new_data(tti_tx_ul, cc_cfg->enb_cc_idx);
    // Check if there is a empty harq, and data to transmit
    if (pending_data == 0) {
      continue;
    }

    sched_ai_tx ai_tx = {
			(uint16_t) tti_tx_ul.to_uint(),
			(uint16_t) user.get_rnti(),
			(uint32_t) pending_data,
			(int32_t) user.find_ue_carrier(cc_cfg->enb_cc_idx)->tpc_fsm.get_ul_snr_estim(),
      (uint16_t) this->beta_factor };
	  memcpy(sched_tx_buffer, &ai_tx, sizeof(ai_tx));
	  int bytes_write = write(fd_to_ai_sched, sched_tx_buffer, sizeof(sched_tx_buffer)); 
	  int bytes_read = read(fd_from_ai_sched, sched_rcv_buffer, sizeof(sched_rcv_buffer));
	  if (bytes_read < 0) {
		  std::cerr << "pipe read error" << std::endl;
	  }
    uint32_t     pending_rb = user.get_required_prb_ul(cc_cfg->enb_cc_idx, pending_data);
    prb_interval alloc      = find_contiguous_ul_prbs(pending_rb, tti_sched->get_ul_mask());
    if (alloc.empty()) {
      continue;
    }
    alloc_result ret = tti_sched->alloc_ul_user(&user, alloc);
    if (ret == alloc_result::no_cch_space) {
      logger.info(
          "SCHED: rnti=0x%x, cc=%d, Couldn't find space in PDCCH for UL tx", user.get_rnti(), cc_cfg->enb_cc_idx);
    }
  }
}

} // namespace srsenb
