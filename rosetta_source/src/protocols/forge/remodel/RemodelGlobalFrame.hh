// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.
//
/// @file protocols/forge/remodel/RemodelGlobalFrame.hh
/// @brief
/// @author Possu Huang ( possu@uw.edu )
///

#ifndef INCLUDED_protocols_forge_remodel_RemodelGlobalFrame_hh
#define INCLUDED_protocols_forge_remodel_RemodelGlobalFrame_hh

//project headers
#include <core/kinematics/MoveMap.hh>
#include <core/pose/Pose.fwd.hh>
#include <core/scoring/ScoreFunction.fwd.hh>
#include <utility/vector1.hh>
#include <protocols/moves/Mover.hh>
#include <protocols/forge/remodel/RemodelData.hh>
#include <protocols/forge/remodel/RemodelWorkingSet.hh>
#include <core/scoring/constraints/ConstraintSet.fwd.hh>
#include <Eigen/Dense>

namespace protocols {
namespace forge {
namespace remodel {

class RemodelGlobalFrame: public protocols::moves::Mover {

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
  typedef core::scoring::constraints::ConstraintSetOP ConstraintSetOP;
  typedef core::scoring::constraints::ConstraintSet ConstraintSet;


public: //constructor/destructor

	RemodelGlobalFrame();

	RemodelGlobalFrame(RemodelData const & remodel_data, RemodelWorkingSet const & working_model, ScoreFunctionOP const & sfxn);
	
	RemodelGlobalFrame(Size segment_size);

	virtual
	~RemodelGlobalFrame();

public: // virtual constructors

	virtual
	MoverOP clone() ;

	virtual
	MoverOP fresh_instance() ;

public: // options

public:

	virtual void apply( Pose & pose);
	virtual std::string get_name() const;

	void get_helical_params( Pose & pose );
	void align_segment( Pose & pose );
	void setup_helical_constraint( Pose & pose );
	void matrix3f_to_xyzMatrix( Eigen::Matrix3f const & Re, numeric::xyzMatrix< core::Real> & R  );
	void identity_matrix( numeric::xyzMatrix< core::Real>  & R );
	void restore_original_cst( Pose & pose );
	void set_native_cst_set( ConstraintSet const & cst_set);
	void set_native_cst_set( Pose const & pose );
	void set_segment_size( Size segment_size );


private: // data

	RemodelData remodel_data_; // design mode determined in here
	RemodelWorkingSet working_model_; // data for the remodeling pose
	ScoreFunctionOP score_fxn_;

	Real radius_;
	Size seg_size;
	int left_handed_;
	ConstraintSetOP native_cst_set;

public: // accessors

	void scorefunction(ScoreFunctionOP const & sfxn);

//	MoveMap const & movemap() const;

//	PackerTask const & packertask() const;

};

} // remodel
} // forge
} // protocols

#endif /* INCLUDED_protocols_forge_remodel_RemodelGlobalFrame_HH */

