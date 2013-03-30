// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.
//
/// @file protocols/forge/remodel/RemodelDesignMover.hh
/// @brief
/// @author Possu Huang ( possu@uw.edu )
///

#ifndef INCLUDED_protocols_forge_remodel_RemodelDesignMover_hh
#define INCLUDED_protocols_forge_remodel_RemodelDesignMover_hh

//project headers
#include <core/kinematics/MoveMap.hh>
#include <core/pose/Pose.fwd.hh>
#include <core/scoring/ScoreFunction.fwd.hh>
#include <utility/vector1.hh>
#include <utility/options/IntegerVectorOption.hh>
#include <protocols/moves/Mover.hh>
#include <protocols/forge/remodel/RemodelData.hh>
#include <protocols/forge/remodel/RemodelWorkingSet.hh>

namespace protocols {
namespace forge {
namespace remodel {

class RemodelDesignMover: public protocols::moves::Mover {

private: // typedefs

	typedef protocols::moves::Mover Super;

public: // typedefs

	typedef core::Real Real;
	typedef core::Size Size;

	typedef core::kinematics::MoveMap MoveMap;
	typedef core::pose::Pose Pose;
	typedef core::scoring::ScoreFunctionOP ScoreFunctionOP;
	typedef core::scoring::ScoreFunction ScoreFunction;
	typedef core::pack::task::PackerTaskOP PackerTaskOP;
	typedef core::pack::task::PackerTask PackerTask;
	typedef protocols::moves::MoverOP MoverOP;
	typedef protocols::forge::remodel::RemodelData RemodelData;
	typedef protocols::forge::remodel::RemodelWorkingSet RemodelWorkingSet;


public: //constructor/destructor

	RemodelDesignMover();

	RemodelDesignMover(RemodelData const & remodel_data, RemodelWorkingSet const & working_model, ScoreFunctionOP const & sfxn);

	virtual
	~RemodelDesignMover();

public: // virtual constructors

	virtual
	MoverOP clone() const;

	virtual
	MoverOP fresh_instance() const;


public: // accessors

	core::pack::task::PackerTaskOP &  task();
	void scorefunction(ScoreFunctionOP const & sfxn);
	//MoveMap const & movemap() const;
	//PackerTask const & packertask() const;

public: // options

	void mode1_packertask(Pose & pose); // auto loop only
	void mode1_1_packertask(Pose & pose); // auto all 
	void mode2_packertask(Pose & pose); // auto loop with design neighbor
	void mode3_packertask(Pose & pose); // auto loop with repack neighbor
	void mode4_packertask(Pose & pose); // full manual
	void mode5_packertask(Pose & pose); // manual with auto design neighbor
	void mode6_packertask(Pose & pose); // manual with auto repack neighbor
	void reduce_task(Pose & pose, PackerTaskOP & task, bool core, bool boundary, bool surface);

	bool check_state();

	void set_state( std::string state_tag );

	bool find_disulfides_in_the_neighborhood( Pose & pose, utility::vector1<std::pair<Size, Size> > & disulf_partners );
	void make_disulfide( Pose & pose, utility::vector1<std::pair<Size, Size> > & disulf_partners, core::kinematics::MoveMapOP mm );

	virtual void apply( Pose & pose );
	virtual std::string get_name() const;
	
private:
	void run_calculator( core::pose::Pose const & pose, std::string const & calculator, std::string const & calculation, utility::vector1_bool & residues );

private: // data

	RemodelData remodel_data_; // design mode determined in here
	RemodelWorkingSet working_model_; // data for the remodeling pose
	PackerTaskOP archived_starting_task_;
	utility::vector1<bool> non_default_positions_;
	std::string state_;
//	PackerTaskOP packer_task_;
	ScoreFunctionOP score_fxn_;
//	MoveMap move_map_;

	// User specified options
	bool op_user_remodel_repeat_structure_;
	bool op_user_design_no_design_;
	bool op_user_design_find_neighbors_;
	bool op_user_design_design_neighbors_;
	bool op_user_design_design_all_;
	bool op_user_design_skip_partial_;
	bool op_user_remodel_resclass_by_sasa_;
	bool op_user_remodel_disulf_landing_range_;

	// Option values
	Size op_remodel_repeat_structure_;
	Size op_remodel_core_cutoff_;
	Size op_remodel_boundary_cutoff_;
	//utility::options::IntegerVectorOption op_remodel_disulf_landing_range_;
	utility::vector1<int>  op_remodel_disulf_landing_range_;
	Real op_remodel_match_rt_limit_;
};

} // remodel
} // forge
} // protocols

#endif /* INCLUDED_protocols_forge_remodel_RemodelDesignMover_HH */

