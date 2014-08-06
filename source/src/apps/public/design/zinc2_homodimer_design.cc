// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file    apps/public/design/zinc2_homodimer_design.cc
/// @brief   Starts with an undesigned symmetric dimer with two interface zinc sites as created by zinc2_homodimer_design_setup, and designs the interface with sc, bb, and jump minimization.
/// @details Prior to running this protocol, 'make_NCS.pl' must be applied to the symmetric zinc dimer to define the symmetry.  The consequence is that the input PDB is actually the monomer generated by 'make_NCS.pl' along with the symmetry definition file.  Reconstituting the dimer is the first step in this protocol.  Next, the metal binding sites are located, an appropriate taskfactory, movemaps, and movers are setup, and finally the dimer is iteratively designed with packrotamers and minimization.  Constraints are applied to the two metal sites, allowing minimization while respecting metal coordination geometry.
/// @author Bryan Der

#include <devel/init.hh>
#include <protocols/metal_interface/MetalSiteResidue.hh>
#include <protocols/metal_interface/ZincSiteFinder.hh>
#include <protocols/metal_interface/AddZincSiteConstraints.hh>

#include <protocols/jd2/JobDistributor.hh>
#include <protocols/moves/Mover.hh>
#include <protocols/simple_moves/PackRotamersMover.hh>
#include <protocols/simple_moves/MinMover.hh>
#include <protocols/moves/MonteCarlo.hh>
#include <protocols/toolbox/task_operations/RestrictToInterfaceOperation.hh>
#include <core/kinematics/MoveMap.hh>
#include <core/pack/task/TaskFactory.hh>
#include <core/pack/task/operation/TaskOperations.hh>
#include <core/pose/Pose.hh>
#include <core/pose/PDBInfo.hh>
#include <core/scoring/ScoreFunctionFactory.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/constraints/ResidueTypeConstraint.hh>
#include <numeric/xyzMatrix.hh>
#include <numeric/xyzVector.hh>
#include <utility/file/FileName.hh>
#include <utility/vector1.hh>
#include <basic/options/option.hh>
#include <basic/options/keys/packing.OptionKeys.gen.hh>
#include <basic/Tracer.hh>

//SYMMETRY
#include <protocols/simple_moves/symmetry/SetupForSymmetryMover.hh> //create symmetric homodimer from input monomer via symmetry:symmetry_definition option
#include <protocols/simple_moves/symmetry/SymPackRotamersMover.hh>
//#include <protocols/simple_moves/symmetry/SymRotamerTrialsMover.hh>
#include <protocols/simple_moves/symmetry/SymMinMover.hh>
#include <core/scoring/symmetry/SymmetricScoreFunction.hh>
//#include <core/conformation/symmetry/util.hh>
//#include <core/conformation/symmetry/SymmetricConformation.hh>

#include <sstream>

#include <core/conformation/Residue.hh>
#include <core/kinematics/Jump.hh>
#include <utility/vector0.hh>




//tracers
using basic::Error;
using basic::Warning;
using basic::T;
static basic::Tracer TR("apps.pilot.bder.zinc2_homodimer_design");

typedef numeric::xyzVector<core::Real> point;
typedef point axis;
typedef core::pose::Pose Pose;

using namespace core;

//define local options
basic::options::IntegerOptionKey const repackmin_iterations( "repackmin_iterations" );
basic::options::RealOptionKey const fav_nat_bonus("fav_nat_bonus");
basic::options::IntegerOptionKey const nstruct_iterations("nstruct_iterations");



///@brief
class zinc2_homodimer_design : public protocols::moves::Mover {
public:
  zinc2_homodimer_design()
  {

		TR << "//////////////////////////////////////////////////////////////////////////////////////////////" << std::endl << std::endl;

		TR << "/// @file    apps/pilot/bder/zinc2_homodimer_design.cc" << std::endl;
		TR << "/// @brief   Starts with an undesigned symmetric dimer with two interface zinc sites as created by zinc2_homodimer_design_setup, and designs the interface with sc, bb, and jump minimization." << std::endl;
		TR << "/// @details Prior to running this protocol, 'make_NCS.pl' must be applied to the symmetric zinc dimer to define the symmetry.  The consequence is that the input PDB is actually the monomer generated by 'make_NCS.pl' along with the symmetry definition file.  Reconstituting the dimer is the first step in this protocol.  Next, the metal binding sites are located, an appropriate taskfactory, movemaps, and movers are setup, and finally the dimer is iteratively designed with packrotamers and minimization.  Constraints are applied to the two metal sites, allowing minimization while respecting metal coordination geometry." << std::endl << std::endl;

		TR << "Options used in this protocol:" << std::endl;
		TR << "  -symmetry:symmetry_definition symmdef_file" << std::endl;
		TR << "  -s monomer.pdb" << std::endl;
		TR << "  -repackmin_iterations 3  // an inner loop" << std::endl;
		TR << "  -nstruct_iterations 5    // an outer loop" << std::endl;
		TR << "  -favor_native_residue_bonus 1.5  //during design, we want to limit the number of mutations" << std::endl;
		TR << "  -jd2:no_output   // output is done manually within the protocol via dump_pdb" << std::endl;

		TR << "Steps in the protocol:" << std::endl;
		TR <<   "STEP 1: use symmetry_definition_file to make symmetric pose" << std::endl;
		TR <<   "STEP 2: find the two zinc binding sites, add zinc constraints to pose" << std::endl;
		TR <<   "STEP 3: make taskfactory, scorefunction, and movers" << std::endl;
		TR <<   "STEP 4: iterate between packrotamers and minimization (sc, bb, jump)" << std::endl << std::endl;

		TR << "//////////////////////////////////////////////////////////////////////////////////////////////" << std::endl << std::endl;

  }
  virtual ~zinc2_homodimer_design(){};

  virtual
  void
  apply( Pose & pose ){

		setup( pose );
		design_symmetric_homodimer_metal_interface( pose );

    return;
  }


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////// Setup  ///////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//setup
	void
	setup( Pose & pose ){

		utility::file::FileName pdbfilename( pose.pdb_info()->name() );
		pdbname_base_ = pdbfilename.base();

		// make symmetric pose according to symm definition file included as an option
		protocols::simple_moves::symmetry::SetupForSymmetryMoverOP make_monomeric_input_pose_symmetrical = new protocols::simple_moves::symmetry::SetupForSymmetryMover();
		make_monomeric_input_pose_symmetrical->apply( pose );

		pose.dump_pdb("sym.pdb");

		setup_metalsite( pose );
		setup_taskfactory_scorefunction_movers( pose );

		return;
	}


	//setup
	void
	setup_metalsite( Pose & pose ){

		TR << "Finding zinc sites 1 and 2: " << std::endl;

		Size zn1_res( 0 );
		Size zn2_res( 0 );
		bool found_zn1( false );
		bool found_zn2( false );

		for( Size i=1; i<=pose.total_residue(); i++ ) {
			if(pose.residue(i).name3() == " ZN") {
				if( ! found_zn1 ) {
					zn1_res = i;
					found_zn1 = true;
				}
				else if ( ! found_zn2 ) {
					zn2_res = i;
					found_zn2 = true;
				}
			}
		}

		if(zn1_res == 0 || zn2_res == 0 ) {
			set_last_move_status(protocols::moves::FAIL_DO_NOT_RETRY);
			TR << "DID NOT FIND TWO ZINC's.  SET STATUS FAIL DO NOT RETRY." << std::endl;
			return;
		}

		//will find the zincs and the coordinating residues, storing the relevant info such as restype, resnum, atom xyz's
		protocols::metal_interface::ZincSiteFinderOP find_zinc1 = new protocols::metal_interface::ZincSiteFinder( zn1_res );
		protocols::metal_interface::ZincSiteFinderOP find_zinc2 = new protocols::metal_interface::ZincSiteFinder( zn2_res );
		msr1_ = find_zinc1->find_zinc_site(pose); // msr is of type utility::vector1< protocols::metal_interface::MetalSiteResidueOP >
		msr2_ = find_zinc2->find_zinc_site(pose);

		protocols::metal_interface::AddZincSiteConstraintsOP constraints_adder1 = new protocols::metal_interface::AddZincSiteConstraints( msr1_ );
		protocols::metal_interface::AddZincSiteConstraintsOP constraints_adder2 = new protocols::metal_interface::AddZincSiteConstraints( msr2_ );
		constraints_adder1->add_constraints( pose );
		constraints_adder2->add_constraints( pose );

		return;
	}



	//setup
	void
	setup_taskfactory_scorefunction_movers( Pose & pose ){

		using namespace core::pack::task;
		using namespace basic::options;
		using namespace core::scoring;
		using namespace core::scoring::constraints;


		//TASKFACTORY --> resfile
		TaskFactoryOP task_factory = new TaskFactory();
		task_factory->push_back(new operation::InitializeFromCommandline());
		if ( option[ OptionKeys::packing::resfile ].user() ) { // resfile enables one-sided interface design
			task_factory->push_back( new operation::ReadResfile ); //NATAA resfile
		}
		//TASKFACTORY --> prevent repack
		operation::PreventRepackingOP prevent_repack = new operation::PreventRepacking();
		TR << "Preventing repacking of residues ";
		for(core::Size i(2); i <= 5; ++i) {
			prevent_repack->include_residue( msr1_[i]->get_seqpos() );
			prevent_repack->include_residue( msr2_[i]->get_seqpos() );
		}
		TR << std::endl;
		task_factory->push_back(prevent_repack);

		//TASKFACTORY --> restrict to interface
		task_factory->push_back(new protocols::toolbox::task_operations::RestrictToInterfaceOperation(1, 2));
		taskfactory_ = task_factory;


		//SCOREFUNCTION --> Favor native residue
		for ( Size i=1; i<= pose.total_residue();  ++i ) {
			pose.add_constraint( new ResidueTypeConstraint( pose, i, basic::options::option[fav_nat_bonus].value()) );
		}
		//SCOREFUNCTION --> add constraints
		fa_metal_scorefxn_ = get_score_function();
		fa_metal_scorefxn_->set_weight( res_type_constraint, 1.0 );
		fa_metal_scorefxn_->set_weight( atom_pair_constraint, 1.0 );
		fa_metal_scorefxn_->set_weight( dihedral_constraint, 1.0 );
		fa_metal_scorefxn_->set_weight( angle_constraint, 1.0 );

		//SCOREFUNCTION --> make symmetric
		fa_metal_sym_scorefxn_ = core::scoring::symmetry::symmetrize_scorefunction( *fa_metal_scorefxn_ );
		TR << "fullatom + metal scorefunction: " << *fa_metal_sym_scorefxn_ << std::endl;


		//MOVERS --> sym_pack_mover
		TR << "Generating sym pack mover..." << std::endl;
		sym_pack_mover_ = new protocols::simple_moves::symmetry::SymPackRotamersMover;
		sym_pack_mover_->task_factory( taskfactory_ );
		sym_pack_mover_->score_function( fa_metal_scorefxn_ );

		//MOVERS --> minmover
		movemap_sc_ = new kinematics::MoveMap;
		movemap_sc_->set_chi( true );
		movemap_sc_->set_bb( false );
		movemap_sc_->set_jump( false ); //first and second jumps are to zinc from chain A

		movemap_bb_ = new kinematics::MoveMap;
		movemap_bb_->set_chi( false );
		movemap_bb_->set_bb( true );
		movemap_bb_->set_jump( true ); //first and second jumps are to zinc from chain A

		movemap_ = new kinematics::MoveMap;
		movemap_->set_chi( true );
		movemap_->set_bb( true );
		movemap_->set_jump( true ); //first and second jumps are to zinc from chain A


		sym_minmover_sc_ = new protocols::simple_moves::symmetry::SymMinMover( movemap_sc_, fa_metal_scorefxn_, "dfpmin_armijo", 0.01, true );
		sym_minmover_bb_ = new protocols::simple_moves::symmetry::SymMinMover( movemap_bb_, fa_metal_scorefxn_, "dfpmin_armijo", 0.01, true );
		sym_minmover_ = new protocols::simple_moves::symmetry::SymMinMover( movemap_, fa_metal_scorefxn_, "dfpmin_armijo", 0.01, true );



		return;
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////   END SETUP   ///////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////



	//////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////   BEGIN PROTOCOL   //////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////


	//protocol
	void
	design_symmetric_homodimer_metal_interface( Pose & pose ){ //i, j, k only needed for pdb naming

    Pose const designable_start_pose( pose );

		Size nstruct_it = (Size) basic::options::option[nstruct_iterations];
    for (Size n(1); n <= nstruct_it; n++) { //home-made nstruct
      std::stringstream ss;
      ss << n;
      std::string n_string;
      if(n < 10) { n_string = "000" + ss.str(); }
      else if (n < 100) { n_string = "00" + ss.str(); }
      else if (n < 1000) { n_string = "0" + ss.str(); }
      else { n_string = ss.str(); }

      protocols::moves::MonteCarloOP mc = new protocols::moves::MonteCarlo( pose , *fa_metal_sym_scorefxn_ , 0.6 );

			Size repackmin_it = (Size) basic::options::option[ repackmin_iterations ];
      for( Size i(1); i <= repackmin_it; ++i ) {
				TR << "Repackmin cycle " << i << " out of " << repackmin_it << std::endl;

				//DESIGN
				sym_pack_mover_->apply( pose );
				TR << "Score after packing: " << fa_metal_sym_scorefxn_->score( pose ) << std::endl;

				//MINIMIZE
				sym_minmover_sc_->apply( pose );
				TR << "Score after SC minimization: " << fa_metal_sym_scorefxn_->score( pose ) << std::endl;
				mc->boltzmann( pose );

				sym_minmover_bb_->apply( pose );
				TR << "Score after BB minimization: " << fa_metal_sym_scorefxn_->score( pose ) << std::endl;
				mc->boltzmann( pose );

				sym_minmover_->apply( pose );
				TR << "Score after ALL minimization: " << fa_metal_sym_scorefxn_->score( pose ) << std::endl;
				mc->boltzmann( pose );

      }//repackmin_iterations

      mc->recover_low( pose );

      std::string nstruct_dump_name = pdbname_base_ + "_" + n_string + ".pdb";
      pose.dump_scored_pdb(nstruct_dump_name, *fa_metal_sym_scorefxn_);

      TR << "FINISHED " << nstruct_dump_name << std::endl;

      pose = designable_start_pose;

    }//nstruct

		return;
	}//design_symmetric_homodimer_metal_interface


	virtual
	std::string
	get_name() const { return "zinc2_homodimer_design"; }


private:

	std::string pdbname_base_;

	utility::vector1< protocols::metal_interface::MetalSiteResidueOP > msr1_;
	utility::vector1< protocols::metal_interface::MetalSiteResidueOP > msr2_;

	core::pack::task::TaskFactoryOP taskfactory_;
	core::scoring::ScoreFunctionOP fa_metal_scorefxn_;
  core::scoring::symmetry::SymmetricScoreFunctionOP fa_metal_sym_scorefxn_;

  protocols::simple_moves::symmetry::SymPackRotamersMoverOP sym_pack_mover_;
  protocols::simple_moves::symmetry::SymMinMoverOP sym_minmover_;
  protocols::simple_moves::symmetry::SymMinMoverOP sym_minmover_sc_;
  protocols::simple_moves::symmetry::SymMinMoverOP sym_minmover_bb_;

	core::kinematics::MoveMapOP movemap_;
	core::kinematics::MoveMapOP movemap_sc_;
	core::kinematics::MoveMapOP movemap_bb_;

};


typedef utility::pointer::owning_ptr< zinc2_homodimer_design > zinc2_homodimer_designOP;

int main( int argc, char* argv[] )
{
	try {
	using basic::options::option;
	option.add( repackmin_iterations, "number of repack and minimization cycles" ).def(2);
	option.add( fav_nat_bonus, "favor native residue" ).def(1.5);
	option.add( nstruct_iterations, "home-made nstruct" ).def(3);


  devel::init(argc, argv);
  protocols::jd2::JobDistributor::get_instance()->go(new zinc2_homodimer_design);

  TR << "************************d**o**n**e**************************************" << std::endl;

  } catch ( utility::excn::EXCN_Base const & e ) {
		std::cout << "caught exception " << e.msg() << std::endl;
		return -1;
  }

  return 0;
}

