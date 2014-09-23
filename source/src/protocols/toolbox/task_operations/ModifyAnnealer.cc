// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet;
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file ModifyAnnealer.cc
///
/// @brief Task operation to set high and low temps for annealer as well as whether or not to do a quench step
/// @author Tim Jacobs

//Unit Headers
#include <protocols/toolbox/task_operations/ModifyAnnealer.hh>
#include <protocols/toolbox/task_operations/ModifyAnnealerCreator.hh>

//Core Headers
#include <core/pack/task/PackerTask.hh>

//Utility Headers
#include <utility/tag/Tag.hh>


namespace protocols {
namespace toolbox {
namespace task_operations {

//initialize to default packer settings
ModifyAnnealer::ModifyAnnealer():
disallow_quench_(false),
high_temp_(100.0),
low_temp_(0.3)
{}

ModifyAnnealer::ModifyAnnealer(bool disallow_quench, core::Real high_temp, core::Real low_temp):
disallow_quench_(disallow_quench),
high_temp_(high_temp),
low_temp_(low_temp)
{}

ModifyAnnealer::~ModifyAnnealer(){}

core::pack::task::operation::TaskOperationOP ModifyAnnealer::clone() const{
	return core::pack::task::operation::TaskOperationOP( new ModifyAnnealer( *this ) );
}

void ModifyAnnealer::apply( core::pose::Pose const &, core::pack::task::PackerTask & task ) const{
	task.disallow_quench(disallow_quench_);
	task.high_temp(high_temp_);
	task.low_temp(low_temp_);
}

void ModifyAnnealer::parse_tag( utility::tag::TagCOP tag, basic::datacache::DataMap & ){
	disallow_quench_ = tag->getOption< bool >("disallow_quench", false);
	high_temp_ = tag->getOption< core::Real >("high_temp", 100.0);
	low_temp_ = tag->getOption< core::Real >("low_temp", 0.3);
}

core::pack::task::operation::TaskOperationOP ModifyAnnealerCreator::create_task_operation() const
{
	return core::pack::task::operation::TaskOperationOP( new ModifyAnnealer );
}

} //task_operations
} //toolbox
} //protocols
