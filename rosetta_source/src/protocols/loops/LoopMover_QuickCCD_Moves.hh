// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file
/// @brief
/// @author Mike Tyka

#ifndef INCLUDED_protocols_loops_LoopMover_QuickCCD_Moves_hh
#define INCLUDED_protocols_loops_LoopMover_QuickCCD_Moves_hh


#include <protocols/loops/LoopMover_QuickCCD.hh>
#include <protocols/moves/Mover.hh>

#include <core/types.hh>
#include <core/scoring/ScoreFunction.fwd.hh>
#include <core/kinematics/MoveMap.fwd.hh>
#include <core/pose/Pose.fwd.hh>

#include <utility/vector1.hh>


// C++ Headers


///////////////////////////////////////////////////////////////////////////////
namespace protocols {
namespace loops {



////////////////////////////////////////////////////////////////////////////////////////
///@details The Old stuff from Looprelax
/////////////////////////////////////////////////////////////////////////////////////////
class LoopMover_Perturb_QuickCCD_Moves: public LoopMover_Perturb_QuickCCD{
public:

	LoopMover_Perturb_QuickCCD_Moves();

	LoopMover_Perturb_QuickCCD_Moves(
		protocols::loops::Loops  loops_in
	);

	LoopMover_Perturb_QuickCCD_Moves(
		protocols::loops::Loops  loops_in,
		core::scoring::ScoreFunctionOP  scorefxn
	);

	virtual std::string get_name() const;

	/// @brief Clone this object
	virtual protocols::moves::MoverOP clone() const {
		return new LoopMover_Perturb_QuickCCD_Moves(*this);
	}

	LoopResult model_loop(
		core::pose::Pose & pose,
		protocols::loops::Loop const & loop
	);

protected:
};




} //namespace loops
} //namespace protocols

#endif //INCLUDED_protocols_loops_LoopMover_QuickCCD_Moves_HH
