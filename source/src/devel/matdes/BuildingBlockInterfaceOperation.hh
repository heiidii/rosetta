// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   devel/matdes/BuildingBlockInterfaceOperation2.0.hh
/// @brief  Restrict design to only residues at inter-building block interfaces
/// @author Will Sheffler (willsheffler@gmail.com) Jacob Bale (balej@uw.edu)

#ifndef INCLUDED_devel_matdes_BuildingBlockInterfaceOperation_hh
#define INCLUDED_devel_matdes_BuildingBlockInterfaceOperation_hh

// Unit Headers
#include <devel/matdes/BuildingBlockInterfaceOperation.fwd.hh>
#include <devel/matdes/BuildingBlockInterfaceOperationCreator.hh>
#include <core/pack/task/operation/TaskOperation.hh>

// Project Headers
#include <core/pose/Pose.fwd.hh>
#include <utility/tag/Tag.fwd.hh>
#include <utility/vector1.hh>
#include <core/types.hh>

// Utility Headers
#include <string>

// C++ Headers

namespace devel { 
namespace matdes {

class BuildingBlockInterfaceOperation : public core::pack::task::operation::TaskOperation {
public:
	BuildingBlockInterfaceOperation(
		core::Size nsub_bblock = 1,
		std::string sym_dof_names  = "",
		core::Real contact_dist = 10,
		core::Real bblock_dist = 5,
		core::Real fa_rep_cut = 3.0,
		bool filter_intrabb = true,
		bool intrabb_only = false,
		bool multicomponent = false );

	virtual ~BuildingBlockInterfaceOperation();

	virtual core::pack::task::operation::TaskOperationOP clone() const;

	virtual
	void
	apply( core::pose::Pose const &, core::pack::task::PackerTask & ) const;

	void parse_tag( TagCOP tag , DataMap & );
	void parse_def( utility::lua::LuaObject const & def );

private:

	core::Size nsub_bblock_;
	std::string sym_dof_names_;
 	core::Real contact_dist_; 
	core::Real bblock_dist_; 
	core::Real fa_rep_cut_;
	bool filter_intrabb_;
	bool intrabb_only_;
	bool multicomponent_;
};

} //namespace matdes 
} //namespace devel

#endif 
