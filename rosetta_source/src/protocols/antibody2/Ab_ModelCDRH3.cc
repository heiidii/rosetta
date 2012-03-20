// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/antibody2/Ab_ModelCDRH3.cc
/// @brief Build a homology model of an antibody2
/// @detailed
///
///
/// @author Jianqing Xu ( xubest@gmail.com )

#include <protocols/jobdist/JobDistributors.hh> // SJF Keep first for mpi

#include <core/chemical/ChemicalManager.hh>
#include <core/chemical/ResidueSelector.hh>
#include <core/chemical/VariantType.hh>
#include <core/io/pdb/pose_io.hh>
#include <core/io/silent/SilentStruct.hh>
#include <core/io/silent/SilentStructFactory.hh>
#include <core/kinematics/FoldTree.hh>
#include <core/kinematics/MoveMap.hh>
#include <core/pack/rotamer_set/UnboundRotamersOperation.hh>
#include <core/pack/task/PackerTask.hh>
#include <core/pack/task/TaskFactory.hh>
#include <core/pack/dunbrack/RotamerConstraint.hh>
#include <core/pose/PDBInfo.hh>
#include <core/pose/util.hh>
#include <core/pose/datacache/CacheableDataType.hh>
#include <core/scoring/Energies.hh>
#include <core/scoring/ScoreType.hh>
#include <core/scoring/rms_util.tmpl.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/ScoreFunctionFactory.hh>
#include <core/scoring/constraints/ConstraintFactory.hh>
#include <core/scoring/constraints/ConstraintIO.hh>
#include <core/import_pose/import_pose.hh>
#include <core/pose/util.hh>

#include <basic/options/option.hh>
#include <basic/options/keys/antibody.OptionKeys.gen.hh>
#include <basic/options/keys/constraints.OptionKeys.gen.hh>
#include <basic/options/keys/in.OptionKeys.gen.hh>
#include <basic/options/keys/out.OptionKeys.gen.hh>
#include <basic/options/keys/run.OptionKeys.gen.hh>
#include <basic/prof.hh>
#include <basic/Tracer.hh>
#include <basic/datacache/BasicDataCache.hh>
#include <basic/datacache/DiagnosticData.hh>

#include <protocols/toolbox/task_operations/RestrictToInterface.hh>
#include <protocols/jd2/ScoreMap.hh>
#include <protocols/jd2/JobDistributor.hh>
#include <protocols/jd2/Job.hh>
#include <protocols/jd2/JobOutputter.hh>
#include <protocols/loops/loops_main.hh>
#include <protocols/loops/Loop.hh>
#include <protocols/loops/Loops.hh>
#include <protocols/simple_moves/ConstraintSetMover.hh>
#include <protocols/simple_moves/MinMover.hh>
#include <protocols/simple_moves/PackRotamersMover.hh>
#include <protocols/simple_moves/RotamerTrialsMover.hh>
#include <protocols/simple_moves/RotamerTrialsMinMover.hh>
#include <protocols/moves/MoverContainer.hh>
#include <protocols/moves/PyMolMover.hh>
#include <protocols/rigid/RB_geometry.hh>
#include <protocols/rigid/RigidBodyMover.hh>

#include <protocols/antibody2/Ab_util.hh>
#include <protocols/antibody2/Ab_Info.hh>
#include <protocols/antibody2/Ab_ModelCDRH3.hh>
#include <protocols/antibody2/CDRH3Modeler2.hh>
#include <protocols/antibody2/Ab_LH_RepulsiveRamp_Mover.hh>
#include <protocols/antibody2/Ab_LH_SnugFit_Mover.hh>
#include <protocols/antibody2/Ab_Relax_a_CDR_FullAtom.hh>

#include <ObjexxFCL/format.hh>
#include <ObjexxFCL/string.functions.hh>
using namespace ObjexxFCL::fmt;




using basic::T;
using basic::Error;
using basic::Warning;

static basic::Tracer TR("protocols.antibody2.Ab_ModelCDRH3");
using namespace core;

namespace protocols {
namespace antibody2 {

// default constructor
Ab_ModelCDRH3::Ab_ModelCDRH3() : Mover() {
	user_defined_ = false;
	init();
}

// default destructor
Ab_ModelCDRH3::~Ab_ModelCDRH3() {}

//clone
protocols::moves::MoverOP
Ab_ModelCDRH3::clone() const {
	return( new Ab_ModelCDRH3() );
}

    
    
void Ab_ModelCDRH3::init() 
{
	Mover::type( "Ab_ModelCDRH3" );

	// setup all the booleans with default values
	// they will get overwritten by the options and/or passed values
    
	set_default();
    
	register_options();
    
	init_from_options();

    scorefxn_ = core::scoring::ScoreFunctionFactory::create_score_function( "standard", "score12" );
    scorefxn_->set_weight( core::scoring::chainbreak, 1.0 );
    scorefxn_->set_weight( core::scoring::overlap_chainbreak, 10./3. );
    scorefxn_->set_weight( core::scoring::atom_pair_constraint, 1.00 );

	setup_objects();

}

    
    
void Ab_ModelCDRH3::set_default()
{
	TR <<  "Setting up default settings to all FALSE" << std::endl;
	snugfit_   = false;
	benchmark_ = false;
	camelid_   = false;
	camelid_constraints_ = false;
    cst_weight_ = 0.0;
    high_cst_ = 100.0; // if changed here, please change at the end of AntibodyModeler as well

}

    
    
void Ab_ModelCDRH3::register_options()
{
	using namespace basic::options;

	option.add_relevant( OptionKeys::antibody::snugfit );
    option.add_relevant( OptionKeys::run::benchmark );
	option.add_relevant( OptionKeys::antibody::camelid );
	option.add_relevant( OptionKeys::antibody::camelid_constraints );
	option.add_relevant( OptionKeys::constraints::cst_weight );
	option.add_relevant( OptionKeys::in::file::native );
}

    
    
    
    
void Ab_ModelCDRH3::init_from_options() 
{
	using namespace basic::options;
	using namespace basic::options::OptionKeys;
	TR <<  "Reading Options" << std::endl;

	if ( option[ OptionKeys::antibody::snugfit ].user() )
                set_snugfit( option[ OptionKeys::antibody::snugfit ]() );
	if ( option[ OptionKeys::antibody::camelid ].user() )
                set_camelid( option[ OptionKeys::antibody::camelid ]() );
	if ( option[ OptionKeys::antibody::camelid_constraints ].user() )
                set_camelid_constraints( option[ OptionKeys::antibody::camelid_constraints ]() );
	if ( option[ OptionKeys::run::benchmark ].user() )
                set_benchmark( option[ OptionKeys::run::benchmark ]() );

	//set native pose if asked for
	if ( option[ OptionKeys::in::file::native ].user() ) {
		core::pose::PoseOP native_pose = new core::pose::Pose();
		core::import_pose::pose_from_pdb( *native_pose, option[ OptionKeys::in::file::native ]() );
		set_native_pose( native_pose );
	}
	else{
		set_native_pose(NULL);
	}
    
	//cst_weight_ = option[ OptionKeys::constraints::cst_weight ]();
    //JQX: why this will read 1 ?
    
	if( camelid_ ) {
		snugfit_ = false;
	}
    
    
    highres_scorefxn_ = scoring::ScoreFunctionFactory::create_score_function("standard", "score12" );
	highres_scorefxn_->set_weight( scoring::chainbreak, 1.0 );
	highres_scorefxn_->set_weight( scoring::overlap_chainbreak, 10./3. );
    // adding constraints
	highres_scorefxn_->set_weight( scoring::atom_pair_constraint, high_cst_ );
}


    
    
void
Ab_ModelCDRH3::setup_objects() {
    
	sync_objects_with_flags();

}
    
void Ab_ModelCDRH3::sync_objects_with_flags() 
{
	using namespace protocols::moves;

	// add movers to sequence mover depending on the flags that were set


    
	flags_and_objects_are_in_sync_ = true;
	first_apply_with_current_setup_ = true;
}


std::string Ab_ModelCDRH3::get_name() const 
{        
    return "Ab_ModelCDRH3";
}

    
    
    
    
    

    

void
Ab_ModelCDRH3::finalize_setup( pose::Pose & frame_pose ) 
{
	TR<<"AAAAAAAA     cst_weight: "<<cst_weight_<<std::endl;
	if(  cst_weight_ != 0.00  ) {
		simple_moves::ConstraintSetMoverOP cdr_constraint = new simple_moves::ConstraintSetMover();
		cdr_constraint->apply( frame_pose );
	}

	// check for native and input pose
	if ( !get_input_pose() ) {
		pose::PoseOP input_pose = new pose::Pose(frame_pose);  //JQX: QUESTION: why owning pointer here
		set_input_pose( input_pose );   // JQX: pass the input_pose to the mover.input_pose_
	}


	pose::PoseOP native_pose;
	if ( !get_native_pose() ) {
		TR << "Danger Will Robinson! Native is an impostor!" << std::endl;
        TR << "   'native_pose' is just a copy of the 'input_pose'    " << std::endl;
        TR << "    since you didn't sepcifiy the native pdb name"<<std::endl;
		native_pose = new pose::Pose(frame_pose);
	} else {
		native_pose = new pose::Pose( *get_native_pose() );
	}

	pose::set_ss_from_phipsi( *native_pose ); // JQX: this is the secondary structure from the native pose

	set_native_pose( native_pose ); // pass the native pose to the mover.native_pose_

    ab_info_ = new Ab_Info(frame_pose,camelid_);
            TR<< " Check ab_info object !!!!!    "<<std::endl;
            TR<<*ab_info_<<std::endl;
    
    model_cdrh3_ = new CDRH3Modeler2(true, true, camelid_, benchmark_, ab_info_ );
    model_cdrh3_->set_native_pose( get_native_pose() );	
}



//APPLY
void Ab_ModelCDRH3::apply( pose::Pose & frame_pose ) {

    using namespace chemical;
    using namespace id;
    using namespace scoring;
    using namespace core::scoring::constraints;
    using namespace protocols::moves;


    // the default inital secstruct is all "L" loop!
    start_pose_ = frame_pose;
    
    protocols::moves::PyMolMover pymol;
    if ( !flags_and_objects_are_in_sync_ ){ 
       sync_objects_with_flags(); 
    }
    
    if ( first_apply_with_current_setup_ ){ 
        finalize_setup(frame_pose);  
        first_apply_with_current_setup_=false; 
    }



	basic::prof_reset();
	protocols::jd2::JobOP job( protocols::jd2::JobDistributor::get_instance()->current_job() );
	// utility::exit( EXIT_FAILURE, __FILE__, __LINE__);

	pose::set_ss_from_phipsi( frame_pose );
    
	pymol.apply( frame_pose );
	pymol.send_energy( frame_pose );

	// display constraints and return
	if( camelid_constraints_ ) {
		display_constraint_residues( frame_pose );
		return;
	}



    //####################################################
    // Step 1: model the cdr h3: 
    //  1). insert the c_terminal
    //  2). frag_ccd the h3 loop
    //  notes: pay attention to the way it treats the stems
    //####################################################
    model_cdrh3_->apply( frame_pose );
    pymol.apply( frame_pose );
    pymol.send_energy( frame_pose );
    
    
    
    
    
    exit(-1);
    
    
    
    //####################################################
    // Step 2: packing the CDRs
    //####################################################
    relax_cdrs( frame_pose );
    pymol.apply( frame_pose );
    pymol.send_energy( frame_pose );


    
    
    //####################################################
	// Step 3: SnugFit: remove the clashes between L-H
    //####################################################
	if ( !camelid_ && snugfit_ ) {
		all_cdr_VL_VH_fold_tree( frame_pose, ab_info_->all_cdr_loops_ );
        
        //$$$$$$$$$$$$$$$$$$$$$$$$
        Ab_LH_RepulsiveRamp_Mover ab_lh_repulsiveramp_mover (ab_info_->all_cdr_loops_);
                // TODO: JQX: should just pass the pointer of ab_info_ into the SnugFit class
        ab_lh_repulsiveramp_mover.apply(frame_pose);
		//repulsive_ramp ( frame_pose, ab_info_.all_cdr_loops_ );
		pymol.apply( frame_pose );
		pymol.send_energy( frame_pose );
        // turn on repulsive
        //$$$$$$$$$$$$$$$$$$$$$$$$$
        Ab_LH_SnugFit_Mover ab_lh_snugfit_mover(ab_info_->all_cdr_loops_); 
                // TODO: JQX: should just pass the pointer of ab_info_ into the SnugFit class
        ab_lh_snugfit_mover.apply(frame_pose);
		//snugfit_mcm_protocol ( frame_pose, ab_info_.all_cdr_loops_ );
		pymol.apply( frame_pose );
		pymol.send_energy( frame_pose );
        // turn off repulsive
        //$$$$$$$$$$$$$$$$$$$$$$$$$$

        //TODO: 
        //JQX: need to read creafully these two functions, maybe they should be merged into one class
        // but this will be in planned phase 2
        

		// align pose to native pose
		pose::Pose native_pose = *get_native_pose();
		antibody2::Ab_Info native_ab( native_pose, camelid_ );
//		ab_info_.align_to_native( pose, native_ab, native_pose );
	}
	pymol.apply( frame_pose );
	pymol.send_energy( frame_pose );

    
    
    
    //####################################################
	// Step 4: Full Atom Relax 
    //####################################################

        //$$$$$$$$$$$$$$$$$$$$$$$$
        Ab_Relax_a_CDR_FullAtom relax_a_cdr_high_res(true/*current_loop_is_H3_*/, true/*H3_filter_*/, ab_info_); 
        relax_a_cdr_high_res.pass_start_pose(start_pose_);
        relax_a_cdr_high_res.apply(frame_pose);
        //build_fullatom_loop( pose_in );
        //$$$$$$$$$$$$$$$$$$$$$$$$$
        if( !benchmark_ ) 
        {
            
            Size repack_cycles(1);
            if( antibody_refine_ && !snugfit_ ){repack_cycles = 3;}
            protocols::simple_moves::PackRotamersMoverOP packer;
            packer = new protocols::simple_moves::PackRotamersMover( highres_scorefxn_ );
            packer->task_factory(tf_);
            packer->nloop( repack_cycles );
            packer->apply( frame_pose );
        }
    
    
        // Minimize CDR H2 loop if this is a camelid
    
        if( is_camelid_ ) {
            //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
            Ab_Relax_a_CDR_FullAtom relax_a_cdr_high_res(false, false, is_camelid_, ab_info_); // because of h2
            relax_a_cdr_high_res.apply(frame_pose);
            //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
        
            //JQX: remove the duplicated code, camelid H2 will be automatically taken care of
            //     see the code in Ab_Relax_a_CDR_FullAtom file
        }
    
    
    
    
    
    
    
    
    //####################################################
	// Step 5: Store the homolgy models
    //####################################################
    
	// remove cutpoints variants for all cdrs
	// "true" forces removal of variants even from non-cutpoints
	loops::remove_cutpoint_variants( frame_pose, true );

	// Define CDR H3 loop
	Size frag_size   = (ab_info_->get_CDR_loop("h3")->stop()  - ab_info_->get_CDR_loop("h3")->start()) + 3;
	Size cutpoint    =  ab_info_->get_CDR_loop("h3")->start() + int( frag_size / 2 );
	loops::Loop cdr_h3( ab_info_->get_CDR_loop("h3")->start(), ab_info_->get_CDR_loop("h3")->stop(), cutpoint, 0, false );

	// Fold Tree
	antibody2::simple_one_loop_fold_tree( frame_pose, cdr_h3 );

	// Redefining CDR H3 cutpoint variants
	loops::add_single_cutpoint_variant( frame_pose, cdr_h3 );

	// add scores to map for outputting constraint score
	( *scorefxn_ )( frame_pose );

	Real constraint_score = frame_pose.energies().total_energies()[ core::scoring::atom_pair_constraint ];

	// removing constraint score
	scorefxn_->set_weight( core::scoring::atom_pair_constraint, 0.00 );
	// add scores to map for output
	( *scorefxn_ )( frame_pose );

	job->add_string_real_pair("AA_H3", global_loop_rmsd( frame_pose, *get_native_pose(), "h3" ));
	job->add_string_real_pair("AB_H2", global_loop_rmsd( frame_pose, *get_native_pose(), "h2" ));
	job->add_string_real_pair("AC_H1", global_loop_rmsd( frame_pose, *get_native_pose(), "h1" ));
	if( !camelid_ ) {
		job->add_string_real_pair("AC_L3", global_loop_rmsd( frame_pose, *get_native_pose(), "l3" ));
		job->add_string_real_pair("AD_L2", global_loop_rmsd( frame_pose, *get_native_pose(), "l2" ));
		job->add_string_real_pair("AE_L1", global_loop_rmsd( frame_pose, *get_native_pose(), "l1" ));
	}
	job->add_string_real_pair("AF_constraint", constraint_score);

	set_last_move_status( protocols::moves::MS_SUCCESS );   

	basic::prof_show();


}// end apply









///////////////////////////////////////////////////////////////////////////
/// @begin relax_cdrs     //JQX: packing, minimization, and mintrial
///
/// @brief relaxes all cdrs simultaneously
///
/// @detailed based on the all_cdrs loop definiton, minimizes only those
///           regions. A standard dfpmin is utilized with score12 and chain
///           -break and chain-overlap set. The allow_bb/chi arrays are
///           changed accordingly but then are reset to their initial
///           states before exiting the routine. Similarly the fold tree
///           and jump movements are restored to their initial states
///
/// @param[out]
///
/// @global_read
///
/// @global_write
///
/// @remarks
///
/// @references
///
/// @authors Aroop 02/15/2010
///
/// @last_modified 02/15/2010
///////////////////////////////////////////////////////////////////////////
void Ab_ModelCDRH3::relax_cdrs( core::pose::Pose & pose )
{
	using namespace pack;
	using namespace pack::task;
	using namespace pack::task::operation;
	using namespace protocols;
	using namespace protocols::toolbox::task_operations;
	using namespace protocols::moves;
	// Storing initial fold tree
	kinematics::FoldTree const input_tree( pose.fold_tree() );

	// changing to all cdr fold tree
	ab_info_->all_cdr_fold_tree( pose );

	// adding cutpoint variants for chainbreak score computation
	loops::add_cutpoint_variants( pose );

	Size const nres = pose.total_residue();

	//setting MoveMap
	kinematics::MoveMapOP allcdr_map;
	allcdr_map = new kinematics::MoveMap();
	allcdr_map->clear();
	allcdr_map->set_chi( false );
	allcdr_map->set_bb( false );
	utility::vector1< bool> is_flexible( nres, false );
	bool include_neighbors( false );
	select_loop_residues( pose, ab_info_->all_cdr_loops_, include_neighbors, is_flexible );
	allcdr_map->set_bb( is_flexible );
	include_neighbors = true;
	select_loop_residues( pose, ab_info_->all_cdr_loops_, include_neighbors, is_flexible );
	allcdr_map->set_chi( is_flexible );
	for( Size ii = 1; ii <= ab_info_->all_cdr_loops_.num_loop(); ii++ )
		allcdr_map->set_jump( ii, false );

    // score functions
	core::scoring::ScoreFunctionOP scorefxn;
	scorefxn = core::scoring::ScoreFunctionFactory::create_score_function( "standard", "score12" );
    scorefxn->set_weight( core::scoring::chainbreak, 10. / 3. );
    scorefxn->set_weight( core::scoring::overlap_chainbreak, 10. / 3. );

	Real min_tolerance = 0.1;
	if( benchmark_ ) min_tolerance = 1.0;
	std::string min_type = std::string( "dfpmin_armijo_nonmonotone" );
	bool nb_list = true;
    simple_moves::MinMoverOP all_cdr_min_moves = new simple_moves::MinMover( allcdr_map,
                                                                    scorefxn, min_type, min_tolerance, nb_list );
    all_cdr_min_moves->apply( pose );

    if( !benchmark_ ) {
        simple_moves::PackRotamersMoverOP repack=new simple_moves::PackRotamersMover( scorefxn );
        setup_packer_task( pose, tf_ );
        ( *scorefxn )( pose );
        tf_->push_back( new RestrictToInterface( is_flexible ) );
        repack->task_factory( tf_ );
        repack->apply( pose );

        simple_moves::RotamerTrialsMinMoverOP rtmin = new simple_moves::RotamerTrialsMinMover( scorefxn, tf_ );
        rtmin->apply( pose );
    }

    // Restoring pose fold tree
    pose.fold_tree( input_tree );
} // relax_cdrs

    
    
    
    
    
    
	///////////////////////////////////////////////////////////////////////////
	/// @begin all_cdr_VL_VH_fold_tree
	///
	/// @brief change to all CDR and VL-VH dock fold tree
	///
	/// @detailed
	///
	/// @param[out]
	///
	/// @global_read
	///
	/// @global_write
	///
	/// @remarks
	///
	/// @references
	///
	/// @authors Aroop 07/13/2010
	///
	/// @last_modified 07/13/2010
	///////////////////////////////////////////////////////////////////////////
	void Ab_ModelCDRH3::all_cdr_VL_VH_fold_tree( pose::Pose & pose_in, const loops::Loops & loops_in ) 
    {

		using namespace kinematics;

		Size nres = pose_in.total_residue();
		core::pose::PDBInfoCOP pdb_info = pose_in.pdb_info();
		char second_chain = 'H';
		Size rb_cutpoint(0);

		for ( Size i = 1; i <= nres; ++i ) {
			if( pdb_info->chain( i ) == second_chain) {
				rb_cutpoint = i-1;
				break;
			}
		}

		Size jump_pos1 ( geometry::residue_center_of_mass( pose_in, 1, rb_cutpoint ) );
		Size jump_pos2 ( geometry::residue_center_of_mass( pose_in,rb_cutpoint+1, nres ) );

		// make sure rb jumps do not reside in the loop region
		for( loops::Loops::const_iterator it= loops_in.begin(), it_end = loops_in.end(); it != it_end; ++it ) {
			if ( jump_pos1 >= ( it->start() - 1 ) &&
					 jump_pos1 <= ( it->stop() + 1) )
				jump_pos1 = it->stop() + 2;
			if ( jump_pos2 >= ( it->start() - 1 ) &&
					 jump_pos2 <= ( it->stop() + 1) )
				jump_pos2 = it->start() - 2;
		}

		// make a simple rigid-body jump first
		setup_simple_fold_tree(jump_pos1,rb_cutpoint,jump_pos2,nres, pose_in );

		// add the loop jump into the current tree,
		// delete some old edge accordingly
		FoldTree f( pose_in.fold_tree() );

		for( loops::Loops::const_iterator it=loops_in.begin(),
					 it_end=loops_in.end(); it != it_end; ++it ) {
			Size const loop_start ( it->start() );
			Size const loop_stop ( it->stop() );
			Size const loop_cutpoint ( it->cut() );
			Size edge_start(0), edge_stop(0);
			bool edge_found = false;
			const FoldTree & f_const = f;
			Size const num_jump = f_const.num_jump();
			for( FoldTree::const_iterator it2=f_const.begin(),
						 it2_end=f_const.end(); it2 !=it2_end; ++it2 ) {
				edge_start = std::min( it2->start(), it2->stop() );
				edge_stop = std::max( it2->start(), it2->stop() );
				if ( ! it2->is_jump() && loop_start > edge_start
						 && loop_stop < edge_stop ) {
					edge_found = true;
					break;
				}
			}

			f.delete_unordered_edge( edge_start, edge_stop, Edge::PEPTIDE);
			f.add_edge( loop_start-1, loop_stop+1, num_jump+1 );
			f.add_edge( edge_start, loop_start-1, Edge::PEPTIDE );
			f.add_edge( loop_start-1, loop_cutpoint, Edge::PEPTIDE );
			f.add_edge( loop_cutpoint+1, loop_stop+1, Edge::PEPTIDE );
			f.add_edge( loop_stop+1, edge_stop, Edge::PEPTIDE );
		}

		f.reorder(1);
		pose_in.fold_tree(f);
	} // all_cdr_VL_VH_fold_tree







Real Ab_ModelCDRH3::global_loop_rmsd (
    const pose::Pose & pose_in,
    const pose::Pose & native_pose,
    std::string cdr_type ) 
{
    using namespace scoring;

    loops::LoopOP current_loop = ab_info_->get_CDR_loop( cdr_type );
    Size loop_start = current_loop->start();
    Size loop_end = current_loop->stop();

    using ObjexxFCL::FArray1D_bool;
    FArray1D_bool superpos_partner ( pose_in.total_residue(), false );

    for ( Size i = loop_start; i <= loop_end; ++i ) superpos_partner(i) = true;

    using namespace core::scoring;
    Real rmsG = rmsd_no_super_subset( native_pose, pose_in, superpos_partner, is_protein_CA );
    return ( rmsG );
} // global_loop_rmsd
    
    
    
    
    
    
    

void Ab_ModelCDRH3::display_constraint_residues( core::pose::Pose & pose ) 
{

    // Detecting di-sulfide bond

    Size H1_Cys(0), H3_Cys(0);

    if(      pose.residue( pose.pdb_info()->pdb2pose('H',32 ) ).name3() == "CYS" ){
        H1_Cys = pose.pdb_info()->pdb2pose( 'H', 32 );
    }
    else if( pose.residue( pose.pdb_info()->pdb2pose('H',33 ) ).name3() == "CYS" ){
        H1_Cys = pose.pdb_info()->pdb2pose( 'H', 33 );
    }

    for( Size ii = ab_info_->get_CDR_loop("h3")->start(); ii <= ab_info_->get_CDR_loop("h3")->stop(); ii++ ){
        if( pose.residue(ii).name3() == "CYS" ) {
            H3_Cys = ii;
        }
    }

    if( ( H1_Cys != 0 ) && ( H3_Cys != 0 ) ){
        TR << "CONSTRAINTS: "<< "AtomPair CA " << H1_Cys << " CA " << H3_Cys
           << " BOUNDED 4.0 6.1 0.6 BOND; mean 5.6 sd 0.6" << std::endl;
    }

    // Specifying extended kink

    Size hfr_46(0), h3_closest(0);
    hfr_46 = pose.pdb_info()->pdb2pose( 'H', 46 );
    if( ab_info_->is_extended() ) h3_closest = ab_info_->get_CDR_loop("h3")->stop() - 5;
    if( h3_closest != 0 ) {
        TR << "CONSTRAINTS: " << "AtomPair CA " << hfr_46 << " CA " << h3_closest
           << " BOUNDED 6.5 9.1 0.7 DISTANCE; mean 8.0 sd 0.7" << std::endl;
    }

    return;
} // display_constraint_residues

    
    
    
    
    
    
    
    
    
    
/// @details  Show the complete setup of the antibody modeler protocol
void Ab_ModelCDRH3::show( std::ostream & out ) {
    if ( !flags_and_objects_are_in_sync_ ){
        sync_objects_with_flags();
    }
    out << *this;
}
    
std::ostream & operator<<(std::ostream& out, const Ab_ModelCDRH3 & ab_m_2 ){
    using namespace ObjexxFCL::fmt;
        
    // All output will be 80 characters - 80 is a nice number, don't you think?
    std::string line_marker = "///";
    out << "////////////////////////////////////////////////////////////////////////////////" << std::endl;
    out << line_marker << A( 47, "Rosetta 3 Antibody Modeler" ) << space( 27 ) << line_marker << std::endl;
    out << line_marker << space( 74 ) << line_marker << std::endl;

    // Display the state of the antibody modeler protocol that will be used
    out << line_marker << "  camelid:  " << ab_m_2.camelid_ <<std::endl;
    out << line_marker << "  snugfit:  " << ab_m_2.snugfit_ <<std::endl;
        
    // Close the box I have drawn
    out << "////////////////////////////////////////////////////////////////////////////////" << std::endl;
    return out;
}
    


    
    
    
    
    
    
    
    
    
    

} // end antibody2
} // end protocols

