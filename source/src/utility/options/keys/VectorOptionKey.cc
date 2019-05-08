// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW CoMotion, email: license@uw.edu.

/// @file   utility/options/keys/VectorOptionKey.cc
/// @brief  Auto-generated serialization template functions
/// @author Andrew Leaver-Fay (aleavefay@gmail.com)

// Unit headers
#include <utility/options/keys/VectorOptionKey.hh>


#ifdef    SERIALIZATION
// Utility serialization headers
#include <utility/serialization/serialization.hh>

// Cereal headers
#include <cereal/access.hpp>
#include <cereal/types/polymorphic.hpp>

/// @brief Automatically generated serialization method
template< class Archive >
void
utility::options::VectorOptionKey::save( Archive & arc ) const {
	arc( cereal::base_class< utility::options::OptionKey >( this ) );
}

/// @brief Automatically generated deserialization method
template< class Archive >
void
utility::options::VectorOptionKey::load( Archive & arc ) {
	arc( cereal::base_class< utility::options::OptionKey >( this ) );
}

SAVE_AND_LOAD_SERIALIZABLE( utility::options::VectorOptionKey );
CEREAL_REGISTER_TYPE( utility::options::VectorOptionKey )

CEREAL_REGISTER_DYNAMIC_INIT( utility_options_keys_VectorOptionKey )

#endif // SERIALIZATION
