/*
 * sched_time_pf_ml.h
 *
 *  Created on: Apr 4, 2022
 *      Author: naposto
 */

#ifndef SRSENB_HDR_STACK_MAC_SCHEDULERS_SCHED_LEASCH_H_
#define SRSENB_HDR_STACK_MAC_SCHEDULERS_SCHED_LEASCH_H_

#include "sched_time_pf.h"

namespace srsenb {

class sched_leasch final : public sched_time_pf {

public:
	sched_leasch(const sched_cell_params_t& cell_params_,
			     const sched_interface::sched_args_t& sched_args);
	void sched_ul_users(sched_ue_list& ue_db, sf_sched* tti_sched) override;

private:
	const sched_cell_params_t* cc_cfg = nullptr;
};

}


#endif /* SRSENB_HDR_STACK_MAC_SCHEDULERS_SCHED_LEASCH_H_ */
