/*
 * sched_leasch.cc
 *
 *  Created on: Apr 5, 2022
 *      Author: naposto
 */

#include "srsenb/hdr/stack/mac/schedulers/sched_leasch.h"
#include <vector>

namespace srsenb {

using srsran::tti_point;

sched_leasch::sched_leasch(const sched_cell_params_t& cell_params_,
						   const sched_interface::sched_args_t& sched_args) : sched_time_pf(cell_params_, sched_args) {}

void sched_leasch::sched_ul_users(sched_ue_list& ue_db, sf_sched* tti_sched) {
	srsran::tti_point tti_rx{tti_sched->get_tti_rx()};
	if (current_tti_rx != tti_rx) {
		new_tti(ue_db, tti_sched);
	}
}

} // namespace srsenb


