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

private:
	const sched_cell_params_t* cc_cfg = nullptr;
	const char* fifo_out = "/tmp/actor_in";
	const char* fifo_in  = "/tmp/actor_out";

	int fd_to_ai_sched, fd_from_ai_sched;

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

	void sched_ul_retxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx);
	void sched_ul_newtxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx);
	void sched_dl_retxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx);
	void sched_dl_newtxs(sched_ue_list& ue_db, sf_sched* tti_sched, size_t prio_idx);
};
}


#endif /* SRSENB_HDR_STACK_MAC_SCHEDULERS_SCHED_AI_H_ */
