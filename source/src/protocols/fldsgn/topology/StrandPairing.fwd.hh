 // -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// This file is part of the Rosetta software suite and is made available under license.
// The Rosetta software is developed by the contributing members of the Rosetta Commons consortium.
// Copyright in the Rosetta software belongs to the developers and their institutions.
// For more information, see www.rosettacommons.org.

/// @file   ./src/protocols/fldsgn/topology/StrandPairing.fwd.hh
/// @brief
/// @author Nobuyasu Koga ( nobuyasu@u.washington.edu )

#ifndef INCLUDED_protocols_fldsgn_topology_StrandPairing_fwd_hh
#define INCLUDED_protocols_fldsgn_topology_StrandPairing_fwd_hh

// utitlity headers
#include <utility/pointer/owning_ptr.hh>
#include <utility/vector1.hh>

namespace protocols {
namespace fldsgn {
namespace topology {

	class StrandPairing;
	class StrandPairingSet;
	class Residues;

	typedef utility::pointer::shared_ptr< StrandPairing > StrandPairingOP;
	typedef utility::pointer::shared_ptr< StrandPairingSet > StrandPairingSetOP;
	typedef utility::pointer::shared_ptr< StrandPairing const > StrandPairingCOP;
	typedef utility::pointer::shared_ptr< StrandPairingSet const > StrandPairingSetCOP;
	typedef utility::vector1< StrandPairingOP > StrandPairings;

	// typedef utility::pointer::owning_ptr< Residues > ResiduesOP;
	//typedef utility::pointer::owning_ptr< StrandPairing const > StrandPairingCOP;
	//typedef utility::pointer::owning_ptr< StrandPairingSet const > StrandPairingSetCOP;

} // namespace topology
} // namespace fldsgn
} // namespace protocols

#endif
