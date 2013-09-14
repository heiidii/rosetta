// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file swa_monte_carlo.cc
/// @author Rhiju Das (rhiju@stanford.edu)

// Most of following needs to be pushed to its own class...

// libRosetta headers
#include <core/types.hh>

// do we need all these?
#include <core/chemical/util.hh>
#include <core/scoring/ScoringManager.hh>
#include <core/scoring/ScoreFunctionFactory.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/rms_util.hh>
#include <core/init/init.hh>
#include <core/io/silent/util.hh>
#include <core/io/silent/SilentStruct.hh>
#include <core/io/silent/SilentFileData.hh>
#include <core/io/silent/BinaryRNASilentStruct.hh>
#include <core/pose/Pose.hh>
#include <core/pose/full_model_info/FullModelInfoUtil.hh>
#include <core/pose/full_model_info/FullModelInfo.hh>
#include <protocols/swa/StepWiseUtil.hh>
#include <protocols/swa/rna/StepWiseRNA_Util.hh>
#include <protocols/swa/monte_carlo/RNA_StepWiseMonteCarlo.hh>
#include <protocols/viewer/viewers.hh>

//////////////////////////////////////////////////
#include <basic/options/keys/score.OptionKeys.gen.hh>
#include <basic/options/option.hh>
#include <basic/options/keys/out.OptionKeys.gen.hh> // for option[ out::file::silent  ] and etc.
#include <basic/options/keys/in.OptionKeys.gen.hh> // for option[ in::file::tags ] and etc.
#include <basic/options/keys/rna.OptionKeys.gen.hh>
#include <basic/options/keys/OptionKeys.hh>
#include <basic/options/option_macros.hh>

#include <basic/Tracer.hh>
#include <utility/vector1.hh>

//////////////////////////////////////////////////////////
#include <ObjexxFCL/string.functions.hh>
#include <ObjexxFCL/format.hh>

// C++ headers
//#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <list>

using namespace protocols;
using namespace basic::options;
using namespace basic::options::OptionKeys;
using ObjexxFCL::lead_zero_string_of;
using utility::vector1;

static basic::Tracer TR( "apps.pilot.rhiju.stepwise_monte_carlo" );

OPT_KEY( Integer, cycles )
OPT_KEY( Real, minimize_single_res_frequency )
OPT_KEY( Real, switch_focus_frequency )
OPT_KEY( Boolean, allow_internal_moves )
OPT_KEY( Boolean, erraser )
OPT_KEY( Boolean, skip_deletions )
OPT_KEY( Boolean, verbose_scores )
OPT_KEY( Real, temperature )
OPT_KEY( Real, add_delete_frequency )
OPT_KEY( Real, just_min_after_mutation_frequency )
OPT_KEY( Integer, num_random_samples )
OPT_KEY( IntegerVector, sample_res )
OPT_KEY( IntegerVector, rmsd_res )

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// could go into a util or object -- or perhaps could use job distributor.
bool
get_out_tag( std::string & out_tag,
						 Size const & n,
						 std::string const & silent_file ){

  using namespace core::io::silent;
	std::map< std::string, bool > tag_is_done;

	static bool init( false );
	if ( !init ){
		tag_is_done = initialize_tag_is_done( silent_file );
		init = true;
	}

	out_tag = "S_"+lead_zero_string_of( n, 6 );
	if (tag_is_done[ out_tag ] ) {
		TR << "Already done: " << out_tag << std::endl;
		return false;
	}
	return true; //ok, not done, so do it.
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
output_to_silent_file( std::string const & out_tag,
											 std::string const & silent_file,
											 pose::Pose const & pose,
											 pose::PoseOP & native_pose ){

  using namespace core::io::silent;
  using namespace core::pose::full_model_info;
  using namespace protocols::swa;

	// output silent file.
	static SilentFileData const silent_file_data;

	//setup rmsd res as everything to be sampled.
	utility::vector1< Size > rmsd_res_list = option[ rmsd_res ]();
	if ( rmsd_res_list.size() == 0 ) rmsd_res_list = const_full_model_info_from_pose( pose ).moving_res_in_full_model();

	BinaryRNASilentStruct s( pose, out_tag );
	s.add_energy( "missing", get_number_missing_residues( pose ) );
	if ( native_pose ) 	s.add_energy( "rms", get_all_atom_rmsd( pose, *native_pose, rmsd_res_list ) );
	silent_file_data.write_silent_struct( s, silent_file, false /*score_only*/ );

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
stepwise_monte_carlo()
{
  using namespace core::pose;
  using namespace core::scoring;
  using namespace core::chemical;
  using namespace core::pose::full_model_info;
  using namespace protocols::swa;
  using namespace protocols::swa::monte_carlo;

	// Following could be generalized to fa_standard, after recent unification, but
	// probably should wait for on-the-fly residue type generation.
	ResidueTypeSetCAP rsd_set = core::chemical::ChemicalManager::get_instance()->residue_type_set( RNA );

	// native pose -- not in use currently?
	PoseOP native_pose;
	if ( option[ in::file::native ].user() ) native_pose = get_pdb_and_cleanup( option[ in::file::native ](), rsd_set );

	// Following could go to a FullModelSetup class.
	// read starting pose(s) from disk
	utility::vector1< std::string > const & input_files = option[ in::file::s ]();
	utility::vector1< PoseOP > input_poses;
	for ( Size n = 1; n <= input_files.size(); n++ ) 	input_poses.push_back( get_pdb_and_cleanup( input_files[ n ], rsd_set ) );
	//FullModelInfo (minimal object needed for add/delete)
	fill_full_model_info_from_command_line( input_poses );

	// scorefunction
	core::scoring::ScoreFunctionOP scorefxn;
	if ( option[ score::weights ].user() ) scorefxn = getScoreFunction();
	else  scorefxn = ScoreFunctionFactory::create_score_function( "rna/rna_res_level_energy.wts" );

	// a unit test specific for two helix test case.
	//	test_merge_and_slice_with_two_helix_test_case( input_poses, scorefxn ); exit( 0 );

	// actual pose to be sampled...
	pose::Pose & pose = *input_poses[ 1 ];
	protocols::viewer::add_conformation_viewer ( pose.conformation(), "current", 800, 800 );

	RNA_StepWiseMonteCarlo stepwise_rna_monte_carlo( scorefxn );
	stepwise_rna_monte_carlo.set_verbose_scores( option[ verbose_scores ]() );
	stepwise_rna_monte_carlo.set_use_phenix_geo( option[ basic::options::OptionKeys::rna::corrected_geo ]() );
	stepwise_rna_monte_carlo.set_skip_deletions( option[ skip_deletions ]() );
	stepwise_rna_monte_carlo.set_num_random_samples( option[ num_random_samples ]() );
	stepwise_rna_monte_carlo.set_erraser( option[ erraser ]() );
	stepwise_rna_monte_carlo.set_cycles( option[ cycles ]() );
	stepwise_rna_monte_carlo.set_add_delete_frequency( option[ add_delete_frequency ]() );
	stepwise_rna_monte_carlo.set_minimize_single_res_frequency( option[ minimize_single_res_frequency ]() );
	stepwise_rna_monte_carlo.set_switch_focus_frequency( option[ switch_focus_frequency ]() );
	stepwise_rna_monte_carlo.set_sample_res( option[ sample_res ]() );
	stepwise_rna_monte_carlo.set_just_min_after_mutation_frequency( option[ just_min_after_mutation_frequency ]() );

	// main loop
	std::string out_tag;
	std::string const silent_file = option[ out::file::silent ]();
	Pose start_pose = pose;
	for ( Size n = 1; n <= option[ out::nstruct ](); n++ ) {

		if ( !get_out_tag( out_tag, n, silent_file ) ) continue;

		pose = start_pose;
		stepwise_rna_monte_carlo.apply( pose );
		output_to_silent_file( out_tag, silent_file, pose, native_pose );
	}

}


///////////////////////////////////////////////////////////////
void*
my_main( void* )
{

	clock_t const my_main_time_start( clock() );

	stepwise_monte_carlo();

	protocols::viewer::clear_conformation_viewers();

	std::cout << "Total time to run " << static_cast<Real>( clock() - my_main_time_start ) / CLOCKS_PER_SEC << " seconds." << std::endl;

  exit( 0 );

}


///////////////////////////////////////////////////////////////////////////////
int
main( int argc, char * argv [] )
{

	try {

  using namespace basic::options;
	utility::vector1< Size > blank_size_vector;
	utility::vector1< std::string > blank_string_vector;

	NEW_OPT( verbose_scores, "Show all score components", false );
	NEW_OPT( skip_deletions, "no delete moves -- just for testing", false );
	NEW_OPT( erraser, "Use KIC sampling", true );
	NEW_OPT( allow_internal_moves, "Allow moves in which internal cutpoints are created to allow ERRASER rebuilds", false );
	NEW_OPT( num_random_samples, "Number of samples from swa residue sampler before minimizing best", 20 );
	NEW_OPT( cycles, "Number of Monte Carlo cycles", 50 );
	NEW_OPT( temperature, "Monte Carlo temperature", 1.0 );
	NEW_OPT( add_delete_frequency, "Frequency of add/delete vs. resampling", 0.5 );
	NEW_OPT( minimize_single_res_frequency, "Frequency with which to minimize the residue that just got rebuilt, instead of all", 0.0 );
	NEW_OPT( switch_focus_frequency, "Frequency with which to switch the sub-pose that is being modeled", 0.5 );
	NEW_OPT( just_min_after_mutation_frequency, "After a mutation, how often to just minimize (without further sampling the mutated residue)", 0.5 );
	NEW_OPT( sample_res, "specify particular residues that should be rebuilt (as opposed to all missing in starting PDB)", blank_size_vector );
	NEW_OPT( rmsd_res, "specify residues for which rmsd values should be computed", blank_size_vector );

  ////////////////////////////////////////////////////////////////////////////
  // setup
  ////////////////////////////////////////////////////////////////////////////
  core::init::init(argc, argv);


  ////////////////////////////////////////////////////////////////////////////
  // end of setup
  ////////////////////////////////////////////////////////////////////////////
  protocols::viewer::viewer_main( my_main );


	} catch ( utility::excn::EXCN_Base const & e ) {
		std::cout << "caught exception " << e.msg() << std::endl;
	}

}



