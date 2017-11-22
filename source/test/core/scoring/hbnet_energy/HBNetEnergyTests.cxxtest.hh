// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file  core/scoring/hbnet_energy/HBNetEnergyTests.cxxtest.hh
/// @brief  Unit tests for the hydrogen bond network score term, a packer-compatible, non-pairwise-decomposible score
/// term that gives a bonus dependenent on the size of hydrogen bond networks (nonlinearly ramping as a function of
/// network size).
/// @author Vikram K. Mulligan (vmullig@u.washington.edu)


// Test headers
#include <test/UMoverTest.hh>
#include <test/UTracer.hh>
#include <cxxtest/TestSuite.h>
#include <test/util/pose_funcs.hh>
#include <test/core/init_util.hh>

// Project Headers
#include <core/scoring/hbnet_energy/HBNetEnergy.hh>

// Core Headers
#include <core/pose/Pose.hh>
#include <core/import_pose/import_pose.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/symmetry/SymmetricScoreFunction.hh>

// Protocols Headers
#include <protocols/cyclic_peptide/SymmetricCycpepAlign.hh>
#include <protocols/simple_moves/symmetry/SetupForSymmetryMover.hh>
#include <protocols/cyclic_peptide/DeclareBond.hh>

// Utility, etc Headers
#include <basic/Tracer.hh>

static basic::Tracer TR("HBNetEnergyTests");

class HBNetEnergyTests : public CxxTest::TestSuite {
	//Define Variables

public:

	void setUp(){
		core_init();

	}

	void tearDown(){

	}

	void test_basic_and_symmetric_scoring() {
		core::pose::Pose pose;
		core::import_pose::pose_from_file( pose, "core/scoring/hbnet_energy/hbnet_energy_test1.pdb", false, core::import_pose::PDB_file );
		protocols::cyclic_peptide::DeclareBond decbond;
		decbond.set( pose.total_residue(), "C", 1, "N", false, false, 0, 0, false );
		decbond.apply(pose);
		pose.update_residue_neighbors();
		//pose.dump_pdb( "hbnet_test1.pdb" ); //DELETE ME
		core::scoring::ScoreFunctionOP sfxn( new core::scoring::ScoreFunction );
		sfxn->set_weight( core::scoring::hbnet, 1.0 );

		core::Real const asymm_score( (*sfxn)(pose) );

		TR << "ASYMM_SCORE =\t" << asymm_score << std::endl;


		//Symmetry test:
		core::pose::Pose pose2(pose);
		protocols::cyclic_peptide::SymmetricCycpepAlign symmalign;
		symmalign.set_angle_threshold(10);
		symmalign.set_auto_detect_symmetry(false);
		symmalign.set_invert(true);
		symmalign.set_symmetry(4, true);
		symmalign.set_trim_info(true, 1);
		symmalign.apply(pose2);
		protocols::simple_moves::symmetry::SetupForSymmetryMover setupsymm;
		setupsymm.process_symmdef_file( "core/scoring/hbnet_energy/S4.symm" );
		setupsymm.apply(pose2);
		protocols::cyclic_peptide::DeclareBond decbond2;
		decbond2.set( 24, "C", 1, "N", false, false, 0, 0, false );
		decbond2.apply(pose2);
		pose2.update_residue_neighbors();
		//pose2.dump_pdb( "hbnet_test2.pdb" ); //DELETE ME

		core::scoring::ScoreFunctionOP sfxn2( core::scoring::symmetry::SymmetricScoreFunctionOP( new core::scoring::symmetry::SymmetricScoreFunction ) );
		sfxn2->set_weight( core::scoring::hbnet, 1.0 );

		core::Real const symm_score( (*sfxn2)(pose2) );

		TR << " SYMM_SCORE =\t" << symm_score << std::endl;

		TS_ASSERT_DELTA( symm_score, asymm_score, 0.000001 );
	}

};