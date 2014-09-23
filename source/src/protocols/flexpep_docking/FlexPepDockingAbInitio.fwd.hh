// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (C) 199x-2007 The Hebrew University, Jerusalem
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file
/// @brief
/// @author Barak Raveh

#ifndef INCLUDED_protocols_flexpep_docking_FlexPepDockingAbInitio_fwd_hh
#define INCLUDED_protocols_flexpep_docking_FlexPepDockingAbInitio_fwd_hh

#include <utility/pointer/owning_ptr.hh>

namespace protocols {
namespace flexpep_docking {

  class FlexPepDockingAbInitio;
  typedef utility::pointer::shared_ptr< FlexPepDockingAbInitio > FlexPepDockingAbInitioOP;
  typedef utility::pointer::shared_ptr< FlexPepDockingAbInitio const > FlexPepDockingAbInitioCOP;

} // moves
} // protocols

#endif
