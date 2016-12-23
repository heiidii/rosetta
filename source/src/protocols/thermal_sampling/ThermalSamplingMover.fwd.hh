// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/thermal_sampling/ThermalSamplingMover.fwd.hh
/// @brief Use a simulated tempering simulation to refine a pose
/// @author Andy Watkins (amw579@nyu.edu)


#ifndef INCLUDED_protocols_thermal_sampling_ThermalSamplingMover_fwd_hh
#define INCLUDED_protocols_thermal_sampling_ThermalSamplingMover_fwd_hh

// Utility headers
#include <utility/pointer/owning_ptr.hh>



// Forward
namespace protocols {
namespace thermal_sampling {

class ThermalSamplingMover;

typedef utility::pointer::shared_ptr< ThermalSamplingMover > ThermalSamplingMoverOP;
typedef utility::pointer::shared_ptr< ThermalSamplingMover const > ThermalSamplingMoverCOP;



} //thermal_sampling
} //protocols


#endif //INCLUDED_protocols_thermal_sampling_ThermalSamplingMover_fwd_hh




