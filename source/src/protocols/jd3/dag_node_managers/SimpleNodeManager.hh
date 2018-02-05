// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/jd3/dag_node_managers/SimpleNodeManager.hh
/// @brief Node Manager that does not discriminate between job results. All results are added to the same pool.
/// @detailed See here for more info: https://www.rosettacommons.org/docs/latest/development_documentation/tutorials/jd3_derived_jq/classes/node_manager
/// @author Jack Maguire, jack@med.unc.edu


#ifndef INCLUDED_protocols_jd3_dag_node_managers_SimpleNodeManager_HH
#define INCLUDED_protocols_jd3_dag_node_managers_SimpleNodeManager_HH

#include <protocols/jd3/dag_node_managers/SimpleNodeManager.fwd.hh>
#include <protocols/jd3/dag_node_managers/EvenlyPartitionedNodeManager.hh>

namespace protocols {
namespace jd3 {
namespace dag_node_managers {

class SimpleNodeManager : public EvenlyPartitionedNodeManager {
public:

	///@param job_offset The node manager can only represent nodes where the jobids form a continuous range. That range should start at job_offset+1.
	///@param num_jobs_total The range mentioned previous should end with job_offset+num_jobs_total (inclusive)
	///@param num_results_to_keep maximum number of results you want to keep
	///@param result_threshold If you want to have a result threshold, define it here. We will stop submitting once we receive this number of results.
	SimpleNodeManager(
		core::Size job_offset,
		core::Size num_jobs_total,
		core::Size num_results_to_keep,
		core::Size result_threshold = 0
	) :
		EvenlyPartitionedNodeManager (
		job_offset,
		num_jobs_total,
		num_results_to_keep,
		1,
		result_threshold
		)
	{}

	///@brief override of NodeManager::register_result() so that the partition argument will always be 1.
	void register_result( core::Size global_job_id, core::Size local_result_id, core::Real score, core::Size ) override {
		NodeManager::register_result( global_job_id, local_result_id, score, 1 );
	}

	void register_result( core::Size global_job_id, core::Size local_result_id, core::Real score ) {
		register_result( global_job_id, local_result_id, score, 1 );
	}


	/*
	Other useful methods:
	*/

};

} //dag_node_managers
} //jd3
} //protocols

#endif