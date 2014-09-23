// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/antibody_design/InterfaceFeatures.fwd.hh
/// @brief 
/// @author Jared Adolf-Bryfogle (jadolfbr@gmail.com)

#ifndef INCLUDED_protocols_features_INTERFACEFEATURES_FWD_HH
#define INCLUDED_protocols_features_INTERFACEFEATURES_FWD_HH

#include <utility/pointer/owning_ptr.hh>

namespace protocols {
namespace features {
		
class InterfaceFeatures;

	typedef utility::pointer::shared_ptr< protocols::features::InterfaceFeatures > InterfaceFeaturesOP;
	typedef utility::pointer::shared_ptr< InterfaceFeatures const > InterfaceFeaturesCOP;

}
}


#endif	//#ifndef INCLUDED_protocols/antibody_design_INTERFACEFEATURES_FWD_HH

