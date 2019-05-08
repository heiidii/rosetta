// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   protocols/jd3/pose_outputters/PoseOutputter.cc
/// @brief  Definition of the %PoseOutputter class's methods
/// @author Andrew Leaver-Fay (aleaverfay@gmail.com)

//unit headers
#include <protocols/jd3/pose_outputters/PoseOutputter.hh>

#ifdef    SERIALIZATION
// Utility serialization headers
#include <utility/serialization/serialization.hh>

// Cereal headers
#include <cereal/types/polymorphic.hpp>
#endif // SERIALIZATION

namespace protocols {
namespace jd3 {
namespace pose_outputters {

PoseOutputter::PoseOutputter() = default;
PoseOutputter::~PoseOutputter() = default;

} // namespace pose_outputters
} // namespace jd3
} // namespace protocols

#ifdef    SERIALIZATION

/// @brief Automatically generated serialization method
template< class Archive >
void
protocols::jd3::pose_outputters::PoseOutputter::save( Archive & arc ) const {
	arc( cereal::base_class< protocols::jd3::output::ResultOutputter >( this ) );
}

/// @brief Automatically generated deserialization method
template< class Archive >
void
protocols::jd3::pose_outputters::PoseOutputter::load( Archive & arc ) {
	arc( cereal::base_class< protocols::jd3::output::ResultOutputter >( this ) );
}

SAVE_AND_LOAD_SERIALIZABLE( protocols::jd3::pose_outputters::PoseOutputter );
CEREAL_REGISTER_TYPE( protocols::jd3::pose_outputters::PoseOutputter )

CEREAL_REGISTER_DYNAMIC_INIT( protocols_jd3_pose_outputters_PoseOutputter )
#endif // SERIALIZATION
