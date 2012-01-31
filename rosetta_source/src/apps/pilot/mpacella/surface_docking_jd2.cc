 // -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;
//      rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
// :noTabs=false:tabSize=4:indentSize=4:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
//          under license.
// (c) The Rosetta software is developed by the contributing members of the
//          Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions
//          about this can be
// (c) addressed to University of Washington UW TechTransfer,
//                                            email: license@u.washington.edu.

/// @file   surface_docking_jd2.cc
/// @brief
/// @author Robin A Thottungal (raugust1@jhu.edu)

// Unit header
#include <protocols/surface_docking/SurfaceDockingProtocol.hh>
#include <protocols/surface_docking/SurfaceDockingProtocol.fwd.hh>

// Project header
#include <protocols/jd2/JobDistributor.hh>
#include <devel/init.hh>
//#include <protocols/init/init.hh>

int
main( int argc, char * argv [] )
{
	using namespace protocols::surface_docking;
	using namespace protocols::jd2;

	// initialize option system, random number generators, and all factory-registrators
	devel::init(argc, argv);
	//protocols::init(argc, argv);

	SurfaceDockingProtocolOP dp = new SurfaceDockingProtocol();
	JobDistributor::get_instance()->go(dp);
}
