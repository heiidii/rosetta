// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file    test_RingConformationMover.cc
/// @brief   Pilot application source code for testing RingConformationMover.
/// @author  labonte

// Unit headers
#include <protocols/simple_moves/carbohydrates/RingConformationMover.hh>

// Project headers
#include <devel/init.hh>
#include <protocols/jd2/JobDistributor.hh>


int main(int argc, char *argv[])
{
	using namespace protocols;
	using namespace simple_moves::carbohydrates;
	using namespace jd2;

	// Initialize core.
	devel::init(argc, argv);

	// Distribute the mover.
	RingConformationMoverOP my_mover = new RingConformationMover();
	JobDistributor::get_instance()->go(my_mover);
}
