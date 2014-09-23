// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/antibody/clusters/CDRClusterMatcher.fwd.hh
/// @brief 
/// @author Jared Adolf-Bryfogle (jadolfbr@gmail.com)

#ifndef INCLUDED_protocols_antibody_clusters_CDRCLUSTERMATCHER_FWD_HH
#define INCLUDED_protocols_antibody_clusters_CDRCLUSTERMATCHER_FWD_HH

// Utility headers
#include <utility/pointer/owning_ptr.hh>


namespace protocols {
namespace antibody {
namespace clusters {
	
// Forward
class CDRClusterMatcher;

typedef utility::pointer::shared_ptr< CDRClusterMatcher > CDRClusterMatcherOP;
typedef utility::pointer::shared_ptr< CDRClusterMatcher const > CDRClusterMatcherCOP;


} //namespace clusters
} //namespace antibody
} //namespace protocols

#endif	//#ifndef INCLUDED_protocols/antibody_design_CDRCLUSTERMATCHER_FWD_HH

