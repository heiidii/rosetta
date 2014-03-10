// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// This file is part of the Rosetta software suite and is made available under license.
// The Rosetta software is developed by the contributing members of the Rosetta Commons consortium.
// (C) 199x-2009 Rosetta Commons participating institutions and developers.
// For more information, see http://www.rosettacommons.org/.

/// @file
/// @brief
/// @author Ingemar Andre


#ifndef INCLUDED_protocols_symmetric_docking_SymFoldAndDockMoveRbJumpMover_hh
#define INCLUDED_protocols_symmetric_docking_SymFoldAndDockMoveRbJumpMover_hh

// Unit headers
#include <protocols/symmetric_docking/SymFoldandDockMoveRbJumpMover.fwd.hh>
#include <protocols/moves/Mover.hh>
#include <core/pose/Pose.fwd.hh>

#include <utility/vector1.hh>
#include <utility/tag/Tag.fwd.hh>


// Utility Headers

namespace protocols {
namespace symmetric_docking {
///////////////////////////////////////////////////////////////////////////////
class SymFoldandDockMoveRbJumpMover : public moves::Mover
{
public:

	// default constructor

	SymFoldandDockMoveRbJumpMover();

	~SymFoldandDockMoveRbJumpMover(){}

	void apply( core::pose::Pose & pose );
	virtual std::string get_name() const;

	void parse_my_tag(
			utility::tag::TagCOP tag,
			basic::datacache::DataMap &,
			protocols::filters::Filters_map const &,
			protocols::moves::Movers_map const &,
			core::pose::Pose const & );
};

} // symmetric_docking
} // rosetta
#endif
