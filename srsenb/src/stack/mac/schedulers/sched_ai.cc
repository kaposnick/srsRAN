#include "srsenb/hdr/stack/mac/schedulers/sched_ai.h"
#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cstring>

namespace srsenb {

sched_ai::sched_ai(const sched_cell_params_t& cell_params_, const sched_interface::sched_args_t& sched_args) {
	cc_cfg = &cell_params_;

	// create the pipe
	mkfifo(fifo_out, 0666);
	fd_to_ai_sched = open(fifo_out, O_CREAT | O_WRONLY, 0666);
	if (fd_to_ai_sched < 0) {
		std::cerr << "Error opening " << fd_to_ai_sched << std::endl;
	}

	if (report_to_orchestrator) {
		mkfifo(orchestrator, 0666);
		fd_to_orchestrator = open(orchestrator, O_CREAT | O_WRONLY, 0666);
		if (fd_to_orchestrator < 0) {
			std::cerr << "Error opening " << fd_to_orchestrator << std::endl;
		}	
	}

	mkfifo(fifo_verify_action, 0666);
	fd_verify_action = open(fifo_verify_action, O_CREAT | O_WRONLY, 0666);
	if (fd_verify_action < 0) {
		std::cerr << "Error opening " << fd_verify_action << std::endl;
	}

	mkfifo(fifo_in, 0666);
	fd_from_ai_sched = open(fifo_in, O_RDONLY);
	if (fd_from_ai_sched < 0) {
		std::cerr << "Error opening " << fd_from_ai_sched << std::endl;
	}
}

void sched_ai::sched_update_beta_factor(uint32_t beta_factor, uint16_t gain) {
	std::cout << "SCHED AI Updating beta factor " << beta_factor << " gain " << gain << std::endl;
	this->beta_factor = beta_factor;
	this->gain        = gain;
}

void sched_ai::sched_dl_users(sched_ue_list& ue_db, sf_sched* tti_sched) {
	if (ue_db.empty()) {
	    return;
	  }

	  // give priority in a time-domain RR basis.
	  uint32_t priority_idx = tti_sched->get_tti_tx_dl().to_uint() % (uint32_t)ue_db.size();
	  sched_dl_retxs(ue_db, tti_sched, priority_idx);
	  sched_dl_newtxs(ue_db, tti_sched, priority_idx);
}

void sched_ai::sched_dl_retxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx)
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

void sched_ai::sched_dl_newtxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx)
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

void sched_ai::sched_ul_users(sched_ue_list& ue_db, sf_sched* tti_sched) {
	if (ue_db.empty()) {
		return;
		// do not schedule any users in the uplink
	}
	uint32_t priority_idx = tti_sched->get_tti_tx_ul().to_uint() % (uint32_t) ue_db.size();
	sched_ul_retxs(ue_db, tti_sched, priority_idx);
	sched_ul_newtxs(ue_db, tti_sched, priority_idx);
}

void sched_ai::sched_ul_retxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx) {
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

void sched_ai::sched_ul_newtxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx) {
	auto iter = ue_db.begin();
	std::advance(iter, prio_idx);

	// schedule only one UE
	sched_ue& user = *iter->second;
	const ul_harq_proc* h = get_ul_newtx_harq(user, tti_sched);
	if ( h == nullptr) {
		return;
	}
	uint32_t pending_data = user.get_pending_ul_new_data(tti_sched->get_tti_tx_ul(), cc_cfg->enb_cc_idx);
	if (pending_data == 0) {
		return;
	}

	sched_ue_cell* ue_cell = user.find_ue_carrier(cc_cfg->enb_cc_idx);
	float snr = ue_cell->tpc_fsm.get_ul_snr_custom_estim() * 1000;

	if (report_to_orchestrator && snr >= 18000 && snr <= 42000) {
		sched_to_orch orch_ = {
			(uint32_t) tti_sched->get_tti_tx_ul().to_uint(),
			(int32_t) snr
		};
		memcpy(sched_tx_orch_buffer, &orch_, sizeof(sched_to_orch));	
		int bytes_write = write(fd_to_orchestrator, sched_tx_orch_buffer, sizeof(sched_tx_orch_buffer));
	}
	sched_ai_tx ai_tx = {
			(uint16_t) tti_sched->get_tti_tx_ul().to_uint(),
			(uint16_t) user.get_rnti(),
			(uint32_t) pending_data,
			(int32_t) snr,
	        (uint16_t) this->beta_factor,
			(uint16_t) this->gain };
	memcpy(sched_tx_buffer, &ai_tx, sizeof(ai_tx));
	int bytes_write = write(fd_to_ai_sched, sched_tx_buffer, sizeof(sched_tx_buffer)); 
	int bytes_read = read(fd_from_ai_sched, sched_rcv_buffer, sizeof(sched_rcv_buffer));
	if (bytes_read < 0) {
		std::cerr << "pipe read error" << std::endl;
	}

	sched_ai_rcv ai_decision;
	memcpy(&ai_decision, sched_rcv_buffer, sizeof(ai_decision));
	uint8_t pending_rb = ai_decision.num_rbs;
	if (pending_rb <= 0) {
		verify_action(0);
		return;
	}
	prb_interval alloc = find_contiguous_ul_prbs(pending_rb, tti_sched->get_ul_mask());
	if (alloc.empty()) {
		verify_action(0);
		return;
	}
	uint8_t mcs = ai_decision.mcs;
	alloc_result ret = tti_sched->alloc_ul_user(&user, alloc, mcs);
	if (ret == alloc_result::no_cch_space) {
		logger.info(
		          "SCHED: rnti=0x%x, cc=%d, Couldn't find space in PDCCH for UL tx", user.get_rnti(), cc_cfg->enb_cc_idx);
		verify_action(0);
		return;
	}
	verify_action(1);
}

void sched_ai::verify_action(uint32_t verify_action_int) {
	sched_verify_action verify_action_struct = {verify_action_int};
	memcpy(sched_vrf_action_buffer, &verify_action_struct, sizeof(verify_action_struct));
	int bytes_write = write(fd_verify_action, sched_vrf_action_buffer, sizeof(sched_vrf_action_buffer));
}

}
