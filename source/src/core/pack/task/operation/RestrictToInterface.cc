// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file RestrictToInterface.cc
/// @brief When passed to a PackerTask, pack/design is limited to the protein interface
///        NOTE: This class is deprecated (and not even compiled).  Use the task operation in
///        protocols/toolbox/task_operations instead.
/// @author ashworth

#include <core/pack/task/operation/RestrictToInterface.hh>
#include <core/pack/task/operation/RestrictToInterfaceCreator.hh>

#include <core/conformation/Residue.hh>
#include <core/kinematics/FoldTree.hh>
#include <core/pack/task/PackerTask.hh>
#include <core/pose/Pose.hh>
#include <core/scoring/ScoreFunction.hh>

#include <core/conformation/Conformation.hh>
#include <core/conformation/symmetry/SymmetricConformation.hh>
#include <core/conformation/symmetry/SymmetryInfo.hh>
#include <core/conformation/symmetry/util.hh>
// AUTO-REMOVED #include <core/scoring/ScoreFunctionFactory.hh>
#include <utility/tag/Tag.hh>
#include <utility/vector1.hh>
#include <core/conformation/Interface.hh>


namespace core {
namespace pack {
namespace task {
namespace operation {

using namespace scoring;

//RestrictTaskForDocking::RestrictTaskForDocking()
//	: parent(),
//		scorefxn_( 0 ),
//		rb_jump_( 0 ),
//		include_current_( true ),
//		distance_( 0 )
//{}
//
//RestrictTaskForDocking::RestrictTaskForDocking(
//	ScoreFunctionCOP scorefxn,
//	core::Size rb_jump,
//	bool include_current,
//	core::Real distance
//) : parent(),
//		scorefxn_( scorefxn ),
//		rb_jump_( rb_jump ),
//		include_current_( include_current ),
//		distance_( distance )
//{}
//
//RestrictTaskForDocking::~RestrictTaskForDocking(){}
//
//
//task::operation::TaskOperationOP RestrictTaskForDocking::clone() const
//{
//	return new RestrictTaskForDocking( *this );
//}
//
//void
//RestrictTaskForDocking::apply(
//	pose::Pose const & pose,
//	task::PackerTask & task
//) const
//{
//	task.initialize_from_command_line().restrict_to_repacking().or_include_current( include_current_ );
//
//	runtime_assert( scorefxn_ != 0 );
//	runtime_assert( rb_jump_ );
//	runtime_assert( distance_ );
//	// (existing comment) /// why is this still necessary???
////	(*scorefxn_)(pose);
////	scorefxn_->accumulate_residue_total_energies( pose );
//
//	core::conformation::Interface interface( rb_jump_ );
//	interface.distance( distance_ );
//	interface.calculate( pose );
//	interface.set_pack( pose, &task );
//}

DockingNoRepack1::DockingNoRepack1()
	: parent(), rb_jump_(1)
{}

DockingNoRepack1::DockingNoRepack1( core::Size rb_jump_in )
	: parent(), rb_jump_(rb_jump_in)
{}

DockingNoRepack1::~DockingNoRepack1(){}

task::operation::TaskOperationOP DockingNoRepack1::clone() const
{
	return new DockingNoRepack1( *this );
}

void
DockingNoRepack1::apply(
	pose::Pose const & pose,
	task::PackerTask & task
) const
{
Size cutpoint ( pose.fold_tree().cutpoint_by_jump( rb_jump_ ) );
		for ( Size ii = 1 ; ii <= cutpoint; ++ii ) {
			 task.nonconst_residue_task( ii ).prevent_repacking();
		}
}

DockingNoRepack2::DockingNoRepack2()
	: parent(), rb_jump_(1)
{}

DockingNoRepack2::DockingNoRepack2( core::Size rb_jump_in )
	: parent(), rb_jump_(rb_jump_in)
{}

DockingNoRepack2::~DockingNoRepack2(){}

task::operation::TaskOperationOP DockingNoRepack2::clone() const
{
	return new DockingNoRepack2( *this );
}

void
DockingNoRepack2::apply(
	pose::Pose const & pose,
	task::PackerTask & task
) const
{
Size cutpoint ( pose.fold_tree().cutpoint_by_jump( rb_jump_ ) );
		for ( Size ii = cutpoint+1 ; ii <= pose.total_residue(); ++ii ) {
			 task.nonconst_residue_task( ii ).prevent_repacking();
		}
}

RestrictToInterface::~RestrictToInterface(){}

RestrictToInterface::RestrictToInterface(utility::vector1<bool> loop_residues):
	parent(), distance_( 8 ), loopy_interface_( true ) {
	ObjexxFCL::FArray1D_bool hack_loop_residues( loop_residues.size(), false );
	for( core::Size ii = 1; ii <= loop_residues.size(); ii++ ) {
		if( loop_residues[ii] )
			hack_loop_residues(ii) = true;
	}
	loop_residues_ = hack_loop_residues;
}

TaskOperationOP RestrictToInterfaceCreator::create_task_operation() const
{
	return new RestrictToInterface;
}

task::operation::TaskOperationOP RestrictToInterface::clone() const
{
	return new RestrictToInterface( *this );
}

void
RestrictToInterface::apply(
	pose::Pose const & pose,
	task::PackerTask & task
) const
{
	utility::vector1<bool> is_interface( pose.total_residue(), false );

	core::Size num_jump_ = rb_jump_.size();
	for( Size jj=1; jj<=num_jump_; jj++ ) {
		core::conformation::Interface interface( rb_jump_[jj] );
		interface.distance( distance_ );
		interface.calculate( pose );

		for ( Size ii=1; ii<=pose.total_residue(); ++ii )
			if( interface.is_interface(ii) )
				is_interface[ii] = true;
	}

	if( loopy_interface_ ) {
		for ( Size ii=1; ii<=pose.total_residue(); ++ii )
			if( loop_residues_(ii) )
				is_interface[ii] = true;
	}

	for ( Size ii=1; ii<=pose.total_residue(); ++ii ) {
		if ( !is_interface[ii] ) //|| pose.residue(ii).is_ligand() )
			task.nonconst_residue_task( ii ).prevent_repacking();
		}
	if ( core::conformation::symmetry::is_symmetric( pose ) ) symmetric_task( pose, task );
}

void RestrictToInterface::symmetric_task( core::pose::Pose const & pose, task::PackerTask & task ) const
{
	using namespace conformation::symmetry;

	SymmetricConformation const & SymmConf (
	dynamic_cast< SymmetricConformation const &> ( pose.conformation()) );

	for ( Size i = 1; i <= pose.total_residue(); ++i ) {
		if ( !SymmConf.Symmetry_Info()->chi_is_independent(i) ) {
			task.nonconst_residue_task( i ).prevent_repacking();
		}
	}
}

void RestrictToInterface::rb_jump( core::Size const jump_in ) {

	rb_jump_.push_back( jump_in );
}

void RestrictToInterface::distance( core::Real const distance_in ) {
	distance_ = distance_in;
}

void
RestrictToInterface::parse_tag( utility::tag::TagCOP tag )
{
  rb_jump_.push_back( ( tag->getOption< core::Size >( "jump", 1 ) ) );
  distance_ = tag->getOption< core::Real >( "distance", 8 )  ;
}

} // namespace operation
} // namespace task
} // namespace pack
} // namespace core

