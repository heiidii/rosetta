// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file RBInMover.fwd.hh
/// @brief switch the chain order
/// @author

#ifndef INCLUDED_devel_splice_RBInMover_fwd_hh
#define INCLUDED_devel_splice_RBInMover_fwd_hh

#include <utility/pointer/owning_ptr.hh>

namespace devel{
    namespace splice{
        class RBInMover;
        typedef utility::pointer::shared_ptr< RBInMover > RBInMoverOP;
        typedef utility::pointer::shared_ptr< RBInMover const > RBInMoverCOP;
    } // simple_moves
} // protocols

#endif // INCLUDED_protocols_simple_moves_RBInMover_fwd_hh
