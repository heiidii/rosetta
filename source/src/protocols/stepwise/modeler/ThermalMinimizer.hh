// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/stepwise/modeler/ThermalMinimizer.hh
/// @brief Use a simulated tempering simulation to refine a pose
/// @author Andy Watkins (amw579@nyu.edu)

#ifndef INCLUDED_protocols_stepwise_modeler_ThermalMinimizer_hh
#define INCLUDED_protocols_stepwise_modeler_ThermalMinimizer_hh

// Unit headers
#include <protocols/stepwise/modeler/ThermalMinimizer.fwd.hh>
#include <protocols/moves/Mover.hh>

// Protocol headers
#include <protocols/filters/Filter.fwd.hh>

// Core headers
#include <core/pose/Pose.fwd.hh>
#include <core/scoring/ScoreFunction.fwd.hh>
#include <core/kinematics/MoveMap.fwd.hh>

// Basic/Utility headers
#include <basic/options/option.hh>
#include <basic/options/keys/rna.OptionKeys.gen.hh>
#include <basic/datacache/DataMap.fwd.hh>

namespace protocols {
namespace stepwise {
namespace modeler {

///@brief Use a simulated tempering simulation to refine a pose
class ThermalMinimizer : public protocols::moves::Mover {

	typedef core::Size Size;
	typedef core::Real Real;

public:

	/////////////////////
	/// Constructors  ///
	/////////////////////

	/// @brief Default constructor
	ThermalMinimizer();
	
	/// @brief Copy constructor (not needed unless you need deep copies)
	ThermalMinimizer( ThermalMinimizer const & src );

	/// @brief Destructor (important for properly forward-declaring smart-pointer members)
	virtual ~ThermalMinimizer();

	/////////////////////
	/// Mover Methods ///
	/////////////////////

public:
	/// @brief Apply the mover
	virtual void
	apply( core::pose::Pose & pose );

	/// @brief Show the contents of the Mover
	static std::string
	class_name();

	virtual void
	show( std::ostream & output = std::cout ) const;

	/// @brief Get the name of the Mover
	virtual std::string
	get_name() const;

	///////////////////////////////
	/// Rosetta Scripts Support ///
	///////////////////////////////

	/// @brief parse XML tag (to use this Mover in Rosetta Scripts)
	virtual void
	parse_my_tag(
		utility::tag::TagCOP tag,
		basic::datacache::DataMap & data,
		protocols::filters::Filters_map const & filters,
		protocols::moves::Movers_map const & movers,
		core::pose::Pose const & pose );

	//ThermalMinimizer & operator=( ThermalMinimizer const & src );

	/// @brief required in the context of the parser/scripting scheme
	virtual protocols::moves::MoverOP
	fresh_instance() const;

	/// @brief required in the context of the parser/scripting scheme
	virtual protocols::moves::MoverOP
	clone() const;
	
	void set_n_cycle( Size const setting ) { n_cycle_ = setting; } 
	void set_temp( Real const temp ) { temp_ = temp; }
	void set_scorefxn( core::scoring::ScoreFunctionOP scorefxn ) { score_fxn_ = scorefxn; }
	void set_mm( core::kinematics::MoveMapOP mm ) { mm_ = mm; }

	//void set_residue_sampling_from_pose_and_movemap( core::pose::Pose const & pose, core::kinematics::MoveMap const & mm );

private: // methods

private: // data
	Size n_cycle_ = 100;
	Real angle_range_chi_ = 180;
	Real angle_range_bb_ = 60;
	Real temp_ = 1.0;
	core::scoring::ScoreFunctionOP score_fxn_;
	core::kinematics::MoveMapOP mm_;
};

std::ostream &
operator<<( std::ostream & os, ThermalMinimizer const & mover );

} //protocols
} //stepwise
} //modeler

#endif //protocols/stepwise/modeler_ThermalMinimizer_hh
