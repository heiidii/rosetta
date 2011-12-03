// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/loops/LoopMover_BackrubCreator.hh
/// @brief  Header for LoopMover_BackrubCreator
/// @author Matthew O'Meara

#ifndef INCLUDED_protocols_loops_LoopMover_BackrubCreator_hh
#define INCLUDED_protocols_loops_LoopMover_BackrubCreator_hh

// Unit Headers
#include <protocols/moves/MoverCreator.hh>
namespace protocols {
namespace loops {

/// @brief creator for the LoopMover_Refine_BackrubCreator class
class LoopMover_Refine_BackrubCreator : public moves::MoverCreator
{
public:
  virtual ~LoopMover_Refine_BackrubCreator();

  virtual moves::MoverOP create_mover() const;
  virtual std::string keyname() const;

};


} //namespace
} //namespace

#endif
