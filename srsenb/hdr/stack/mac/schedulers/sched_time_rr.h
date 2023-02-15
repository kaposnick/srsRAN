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

#ifndef SRSENB_SCHEDULER_METRIC_H
#define SRSENB_SCHEDULER_METRIC_H

#include "sched_base.h"
#include "sched_ai.h"

namespace srsenb {

class sched_time_rr final : public sched_base
{
  const static int MAX_RBG = 25;

public:
  sched_time_rr(const sched_cell_params_t& cell_params_, const sched_interface::sched_args_t& sched_args);
  void sched_dl_users(sched_ue_list& ue_db, sf_sched* tti_sched) override;
  void sched_ul_users(sched_ue_list& ue_db, sf_sched* tti_sched) override;
  void sched_update_beta_factor(uint32_t beta_factor, uint16_t gain) override;

private:
  uint32_t beta_factor = -1;
  uint16_t gain = -1;
  const char* fifo_out = "/tmp/actor_in";
	const char* fifo_in  = "/tmp/actor_out";
  const char* fifo_verify_action = "/tmp/verify_action";

  int fd_to_ai_sched, fd_from_ai_sched, fd_verify_action;

  typedef struct {
		uint32_t verify_action;
	} sched_verify_action;

  typedef struct {
		uint16_t tti;
		uint16_t rnti;
		uint32_t bytes;
		int32_t noise;
		uint16_t beta;
    uint16_t gain;
	} sched_ai_tx;

  typedef struct {
		uint8_t mcs;
		uint8_t num_rbs;
	} sched_ai_rcv;

  char sched_tx_buffer[sizeof(sched_ai_tx)];
	char sched_rcv_buffer[sizeof(sched_ai_rcv)];
	char sched_vrf_action_buffer[sizeof(sched_verify_action)];


  void sched_dl_retxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx);
  void sched_dl_newtxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx);
  void sched_ul_retxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx);
  void sched_ul_newtxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx);
	void verify_action(uint32_t verify_action);


  const sched_cell_params_t* cc_cfg = nullptr;
};

} // namespace srsenb

#endif // SRSENB_SCHEDULER_METRIC_H
