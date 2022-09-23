/*
 * sched_ai.h
 *
 *  Created on: Apr 12, 2022
 *      Author: naposto
 */

#ifndef SRSENB_HDR_STACK_MAC_SCHEDULERS_SCHED_AI_H_
#define SRSENB_HDR_STACK_MAC_SCHEDULERS_SCHED_AI_H_

#include "sched_base.h"

namespace srsenb {

class sched_ai : public sched_base {

public:
	sched_ai(const sched_cell_params_t& cell_params_, const sched_interface::sched_args_t& sched_args);
	void sched_dl_users(sched_ue_list& ue_db, sf_sched* tti_sched) override;
	void sched_ul_users(sched_ue_list& ue_db, sf_sched* tti_sched) override;
	void sched_update_beta_factor(uint32_t beta_factor) override;

private:
	const sched_cell_params_t* cc_cfg = nullptr;
	uint32_t beta_factor = -1;
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
	} sched_ai_tx;

	typedef struct {
		uint16_t tti;
		uint16_t rnti;
		uint32_t decoding_time;
		char result;
		uint32_t bits;
	} shced_ai_result;

	typedef struct {
		uint8_t mcs;
		uint8_t num_rbs;
	} sched_ai_rcv;

	char sched_tx_buffer[sizeof(sched_ai_tx)];
	char sched_rcv_buffer[sizeof(sched_ai_rcv)];
	char sched_vrf_action_buffer[sizeof(sched_verify_action)];

	void sched_ul_retxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx);
	void sched_ul_newtxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx);
	void sched_dl_retxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx);
	void sched_dl_newtxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx);
	void verify_action(uint32_t verify_action);

};
}


#endif /* SRSENB_HDR_STACK_MAC_SCHEDULERS_SCHED_AI_H_ */
