// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/forge/remodel/RemodelLoopMover.cc
/// @brief  Loop modeling protocol based on routines from Remodel and EpiGraft
///         packages in Rosetta++.
/// @author Possu Huang (possu@u.washington.edu)

// unit headers
#include <protocols/forge/remodel/RemodelAccumulator.hh>
#include <protocols/forge/methods/util.hh>

// package headers

// project headers
#include <basic/Tracer.hh>
#include <core/scoring/ScoreFunctionFactory.hh>
#include <basic/options/keys/remodel.OptionKeys.gen.hh>
#include <basic/options/keys/cluster.OptionKeys.gen.hh>
#include <basic/options/keys/constraints.OptionKeys.gen.hh>
#include <protocols/simple_moves/ConstraintSetMover.hh>
#include <core/scoring/constraints/ResidueTypeLinkingConstraint.hh>
#include <basic/options/option.hh>
#include <fstream>
#include <sys/stat.h>
// AUTO-REMOVED #include <ObjexxFCL/format.hh>
// AUTO-REMOVED #include <core/io/pdb/pose_io.hh>
#include <core/import_pose/import_pose.hh>
// AUTO-REMOVED #include <protocols/forge/build/BuildInstruction.hh> // REQUIRED FOR WINDOWS

#include <utility/vector1.hh>

//Auto Headers
#include <protocols/simple_filters/ScoreTypeFilter.hh>


// numeric headers

// boost headers

// C++ headers
//#include <string>

using namespace basic::options;

namespace protocols {
namespace forge {
namespace remodel{

// Tracer instance for this file
// Named after the original location of this code
static basic::Tracer TR( "protocols.forge.remodel.RemodelAccumulator" );

// RNG
//static numeric::random::RandomGenerator RG( 9788221 ); // magic number, don't change


/// @brief default constructor
RemodelAccumulator::RemodelAccumulator()
	: op_user_remodel_repeat_structure_(option[OptionKeys::remodel::repeat_structure].user()),
		op_user_RemodelLoopMover_cyclic_peptide_(option[OptionKeys::remodel::RemodelLoopMover::cyclic_peptide].user()),
		op_user_constraints_cst_file_(option[OptionKeys::constraints::cst_file].user()),
		op_remodel_use_clusters_(option[OptionKeys::remodel::use_clusters]),
		op_remodel_save_top_(option[OptionKeys::remodel::save_top]),
		op_remodel_cluster_on_entire_pose_(option[OptionKeys::remodel::cluster_on_entire_pose]),
		op_remodel_repeat_structure_(option[OptionKeys::remodel::repeat_structure]),
		op_remodel_cluster_radius_(option[OptionKeys::remodel::cluster_radius]),
		op_cluster_max_total_cluster_(option[OptionKeys::cluster::max_total_cluster])
{
	cluster_switch_ = op_remodel_use_clusters_;
}

/// @brief value constructor
RemodelAccumulator::RemodelAccumulator( RemodelWorkingSet & working_model)
	: op_user_remodel_repeat_structure_(option[OptionKeys::remodel::repeat_structure].user()),
		op_user_RemodelLoopMover_cyclic_peptide_(option[OptionKeys::remodel::RemodelLoopMover::cyclic_peptide].user()),
		op_user_constraints_cst_file_(option[OptionKeys::constraints::cst_file].user()),
		op_remodel_use_clusters_(option[OptionKeys::remodel::use_clusters]),
		op_remodel_save_top_(option[OptionKeys::remodel::save_top]),
		op_remodel_cluster_on_entire_pose_(option[OptionKeys::remodel::cluster_on_entire_pose]),
		op_remodel_repeat_structure_(option[OptionKeys::remodel::repeat_structure]),
		op_remodel_cluster_radius_(option[OptionKeys::remodel::cluster_radius]),
		op_cluster_max_total_cluster_(option[OptionKeys::cluster::max_total_cluster])
{
	//check clustering info
	cluster_switch_ = op_remodel_use_clusters_;

	//remodel_data_ = remodeldata;
	working_model_ = working_model;

}

/// @brief copy constructor

/// @brief default destructor
RemodelAccumulator::~RemodelAccumulator(){}

/// @brief clone this object
RemodelAccumulator::MoverOP RemodelAccumulator::clone() const {
	return new RemodelAccumulator( *this );
}


/// @brief create this type of object
RemodelAccumulator::MoverOP RemodelAccumulator::fresh_instance() const {
	return new RemodelAccumulator();
}

void RemodelAccumulator::apply( Pose & pose ){
	using namespace core::scoring;
	using namespace protocols::simple_filters;
	using namespace basic::options;

//make the object's own collection of poses
	core::pose::PoseOP pose_pt = new core::pose::Pose( pose );

	ScoreFunctionOP score12( ScoreFunctionFactory::create_score_function(STANDARD_WTS, SCORE12_PATCH));
	sfxn_ = score12;

	ScoreTypeFilter const  pose_total_score( score12, total_score, 100 );
	core::Real score(pose_total_score.compute( *pose_pt ));
	pose_store_.insert(std::pair<core::Real, core::pose::PoseOP>(score, pose_pt));
	keep_top_pose( op_remodel_save_top_ );

	if (cluster_switch_){
		if (op_remodel_cluster_on_entire_pose_){
			cluster_pose();
		} else { //default uses only loops -- otherwise large structures averages out changes
			cluster_loop();
		}

		cluster_->sort_each_group_by_energy();
		cluster_->sort_groups_by_energy();

		// take the lowest energy in clusters and refine it.
		//std::vector< core::pose::Pose > results = cluster_->get_pose_list();
	}

}

std::string
RemodelAccumulator::get_name() const {
	return "RemodelAccumulator";
}



void RemodelAccumulator::keep_top_pose( core::Size num_to_keep)
{
 // TR << "sorted size " << pose_store_.size() << std::endl;
  if ( pose_store_.size() > num_to_keep )  {
    std::multimap<core::Real, core::pose::PoseOP>::iterator worst_structure = --pose_store_.end();
    //delete (*worst_structure).second; // deletes Pose allocated on heap
    pose_store_.erase( worst_structure );
 // TR << "sorted size " << pose_store_.size() << std::endl;

  }
}

std::vector<core::pose::PoseOP> RemodelAccumulator::contents_in_pose_store(){
	std::vector<core::pose::PoseOP> dummy_return;
	for (std::multimap<core::Real, core::pose::PoseOP>::iterator it= pose_store_.begin(), end = pose_store_.end(); it != end; it++){
		dummy_return.push_back(it->second);
	}
	return dummy_return;
}


void RemodelAccumulator::write_checkpoint(core::Size progress_point){

		std::ofstream cpfile;
		cpfile.open("checkpoint.txt", std::ios::trunc);
		cpfile << progress_point << std::endl;
		cpfile.close();

		core::Size num_report = op_remodel_save_top_;

		core::Size filecount = 1;
	  for (std::multimap<core::Real, core::pose::PoseOP>::iterator it= pose_store_.begin(), end = pose_store_.end(); it != end; it++){
			if (filecount <= num_report){
							std::stringstream sstream;
							std::string num(ObjexxFCL::lead_zero_string_of(filecount,3));
							sstream << "ck_" << num << ".pdb";

							it->second->dump_pdb(sstream.str());
			}
			filecount++;
  	}
}





core::Size RemodelAccumulator::recover_checkpoint()
{
		using namespace core::scoring;
		using namespace core::scoring::constraints;
		using core::import_pose::pose_from_pdb;
		using namespace protocols::simple_filters;
		using namespace basic::options;

		//if reading checkpoint, make sure there's no info stored
		pose_store_.clear();
		core::Size num_report = op_remodel_save_top_;
    for (core::Size i = 1; i<= num_report; i++){
      std::string number(ObjexxFCL::lead_zero_string_of(i, 3));

      //std::string filename = flags.prefix + "_" + number + ".pdb";
      std::string filename = "ck_" + number + ".pdb";
      std::cout << "checking checkpointed file: " << filename << std::endl;
      struct stat stFileInfo;
      bool boolReturn;
      int intStat;

      intStat = stat(filename.c_str(), &stFileInfo);
      if (intStat == 0){
        //file exists
        boolReturn = true;
      } else {
        boolReturn = false;
				continue;
      }

      if (boolReturn == true){
        core::pose::Pose dummyPose;
        core::import_pose::pose_from_pdb( dummyPose, filename.c_str(),false); //fullatom, ideal, readAllChain

				//RemodelDesignMover designMover(remodel_data_, working_model_);
				//designMover.set_state("stage");
				//designMover.apply(dummyPose);

				//special case for repeat proteins.  they need to carry constraints
				//from the start.  Build constraints are set in RemodelLoopMover, a bit
				//different from everything else

				if (op_user_remodel_repeat_structure_){
					if (op_user_constraints_cst_file_){
						protocols::simple_moves::ConstraintSetMoverOP repeat_constraint = new protocols::simple_moves::ConstraintSetMover();
						repeat_constraint->apply( dummyPose );
					}

					// ResidueTypeLinkingConstraints
					Size repeat_number = op_remodel_repeat_structure_;
					Size count=0;
					Real bonus = 10;
					Size segment_length = (dummyPose.n_residue())/repeat_number;
					for (Size rep = 1; rep < repeat_number; rep++ ){ // from 1 since first segment don't need self-linking
						for (Size res = 1; res <= segment_length; res++){
							dummyPose.add_constraint( new ResidueTypeLinkingConstraint(dummyPose, res, res+(segment_length*rep), bonus));
							count++;
						}
					}
					std::cout << "linking " << count << " residue pairs" << std::endl;
				}

				if (op_user_RemodelLoopMover_cyclic_peptide_){ //all processes/constraints has to be applied to checkpointed files
		      protocols::forge::methods::cyclize_pose(dummyPose);
		    }

				this->apply(dummyPose);

					ScoreFunctionOP score12( ScoreFunctionFactory::create_score_function(STANDARD_WTS, SCORE12_PATCH));
					sfxn_ = score12;

					ScoreTypeFilter const  pose_total_score( score12, total_score, 100 );
					core::Real score(pose_total_score.compute( dummyPose ));


        std::cout << "checkpointed_pose " << filename << " score = " << score << std::endl;
      }
    }
    int ck=0;
    std::ifstream trajCount("checkpoint.txt");
    trajCount >> ck;

    std::cout << "checkpoint at " << ck << std::endl;

    return ck;
}






void RemodelAccumulator::cluster_pose(){
	runtime_assert(cluster_switch_);
	cluster_ = new protocols::cluster::ClusterPhilStyle();
	run_cluster();
}

void RemodelAccumulator::cluster_loop(){
	runtime_assert(cluster_switch_);
	cluster_ = new protocols::cluster::ClusterPhilStyle_Loop(working_model_.loops);
	//debug
TR << "loops to build " << working_model_.loops << std::endl;
	run_cluster();
}

void RemodelAccumulator::run_cluster(){

 	runtime_assert(cluster_switch_);

//set cluster size from command line
	core::Real radius = op_remodel_cluster_radius_;


	cluster_->set_score_function( sfxn_ );
	cluster_->set_cluster_radius( radius );

	for ( std::multimap<core::Real, core::pose::PoseOP>::iterator it = pose_store_.begin(), end = pose_store_.end(); it != end ; it++){
		cluster_->apply((*(*it).second));
	}

	cluster_->do_clustering(op_cluster_max_total_cluster_);
	cluster_->do_redistribution();

}

bool RemodelAccumulator::cluster_switch(){
	return cluster_switch_;
}

std::vector<core::pose::PoseOP> RemodelAccumulator::clustered_top_poses(core::Size count){
	runtime_assert(cluster_switch_);
	return	cluster_->return_top_poses_in_clusters(count);
}

void RemodelAccumulator::shrink_cluster(core::Size num_top){
	cluster_->limit_groupsize( (int)num_top ); // keep only cluster center and best energy
}

std::vector<core::pose::PoseOP>  RemodelAccumulator::clustered_best_poses(){
	runtime_assert(cluster_switch_);
	return cluster_->return_lowest_poses_in_clusters();
}




} // remodel
} // forge
} // protocol
