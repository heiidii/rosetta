// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// This file is part of the Rosetta software suite and is made available under license.
// The Rosetta software is developed by the contributing members of the Rosetta Commons consortium.
// (C) 199x-2009 Rosetta Commons participating institutions and developers.
// For more information, see http://www.rosettacommons.org/.

/// @file   core/scoring/symmetry/SymmetricEnergies.fwd.hh
/// @brief  Symmetric Energies class to store cached energies and track the residue
/// neighbor relationships
/// @author Ingemar Andre

#ifndef INCLUDED_core_scoring_symmetry_SymmetricEnergies_fwd_hh
#define INCLUDED_core_scoring_symmetry_SymmetricEnergies_fwd_hh

// Utility headers
#include <utility/pointer/owning_ptr.hh>

namespace core {
namespace scoring {
namespace symmetry {

// Forward
class SymmetricEnergies;

typedef utility::pointer::shared_ptr< SymmetricEnergies       > SymmetricEnergiesOP;
typedef utility::pointer::shared_ptr< SymmetricEnergies const > SymmetricEnergiesCOP;

}	// namespace symmetry
} // namespace pose
} // namespace core

#ifdef USEBOOSTSERIALIZE
#include <boost/serialization/base_object.hpp>
#endif

#endif // INCLUDED_core_pose_symmetry_SymmetricEnergies_FWD_HH
