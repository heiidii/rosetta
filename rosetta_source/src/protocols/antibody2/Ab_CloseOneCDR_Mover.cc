// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/antibody2/Ab_CloseOneCDR_Mover.cc
/// @brief grafts a cdr onto the template of an antibody framework
/// @detailed
/// @author Jianqing Xu (xubest@gmail.com)



#include <protocols/antibody2/Ab_CloseOneCDR_Mover.hh>

#include <core/pose/Pose.hh>
#include <core/kinematics/MoveMap.hh>
#include <protocols/moves/PyMolMover.hh>
#include <protocols/loops/loops_main.hh>
#include <protocols/loops/loop_closure/ccd/CcdLoopClosureMover.hh>
//#include <protocols/loops/LoopMover.fwd.hh>
//#include <protocols/loops/LoopMover.hh>
#include <core/conformation/Conformation.hh>
#include <core/kinematics/FoldTree.hh>
#include <numeric/xyz.functions.hh>
#include <numeric/xyz.io.hh>
#include <protocols/antibody2/Ab_H3_Model_CDR_H3.hh>
#include <protocols/antibody2/Ab_util.hh>

#include <basic/Tracer.hh>
static basic::Tracer TRC("protocols.antibody2.Ab_CloseOneCDR_Mover");

namespace protocols {
namespace antibody2 {
using namespace core;


Ab_CloseOneCDR_Mover::Ab_CloseOneCDR_Mover( ) : Mover( "Ab_CloseOneCDR_Mover" ) {
	set_default();
	cdr_loop_start_ = 0;
	cdr_loop_end_   = 0;
	loop_start_ = 0-flanking_residues_;
	loop_end_   = 0+flanking_residues_;
} // Ab_CloseOneCDR_Mover default constructor

    

Ab_CloseOneCDR_Mover::Ab_CloseOneCDR_Mover( Size query_start, Size query_end ) : Mover( "Ab_CloseOneCDR_Mover" ) {
	set_default();
	cdr_loop_start_ = query_start;
	cdr_loop_end_   = query_end;
	loop_start_ = query_start-flanking_residues_;
	loop_end_   = query_end+flanking_residues_;
} // Ab_CloseOneCDR_Mover default constructor

    
    
    
// Ab_CloseOneCDR_Mover default destructor
Ab_CloseOneCDR_Mover::~Ab_CloseOneCDR_Mover() {}

    
    
    
    
    
void Ab_CloseOneCDR_Mover::set_default()
{
	allowed_separation_ = 1.9;
	flanking_residues_ = 5; // default 5;
	movemap_ = new kinematics::MoveMap();
	movemap_->set_chi( false );
	movemap_->set_bb( false );
	pymol_ = new protocols::moves::PyMolMover();
} // Ab_CloseOneCDR_Mover::set_default

    
    
    
std::string
Ab_CloseOneCDR_Mover::get_name() const { return "Ab_CloseOneCDR_Mover"; }

    
    
void Ab_CloseOneCDR_Mover::set_pymol( protocols::moves::PyMolMoverOP pymol )
{
    pymol_ = pymol;
}

    
    
    
    

void Ab_CloseOneCDR_Mover::apply( pose::Pose & pose_in )
{
    using loops::loop_closure::ccd::CcdMover;
    using loops::loop_closure::ccd::CcdMoverOP;
    
	Size const N ( 1 ); // N atom
	Size const C ( 3 ); // C atom

	// Coordinates of the C and N atoms at stem
	numeric::xyzVector_float peptide_C, peptide_N;
    
	// N-terminal
	peptide_C = pose_in.residue( cdr_loop_start_ - 1 ).xyz( C );
	peptide_N = pose_in.residue( cdr_loop_start_ ).xyz( N );

	// C-terminal
	peptide_C = pose_in.residue( cdr_loop_end_ ).xyz( C );
	peptide_N = pose_in.residue( cdr_loop_end_ + 1 ).xyz( N );

	// calculate separation at ends to see if it needs to be closed
//	Real nter_separation=distance(peptide_C, peptide_N);
//	Real cter_separation=distance(peptide_C, peptide_N);
	Real nter_separation=peptide_C.distance(peptide_N);
	Real cter_separation=peptide_C.distance(peptide_N);

    
	// save the starting foldtree
	core::kinematics::FoldTree f( pose_in.fold_tree() );

    
	// setup movemap to only loop residues
	utility::vector1< bool> allow_bb_move( pose_in.total_residue(), false );
	for ( Size i=loop_start_; i<= loop_end_; ++i )
		allow_bb_move[ i ] = true;
	movemap_->set_bb( allow_bb_move );
	movemap_->set_jump( 1, false );

	pymol_->apply( pose_in );

	if( nter_separation > allowed_separation_ ) {
		loops::Loop one_loop( loop_start_, cdr_loop_start_, cdr_loop_start_-1, 0, false );
		simple_one_loop_fold_tree( pose_in, one_loop );
		CcdMoverOP ccd_moves = new CcdMover( one_loop, movemap_ );
		ccd_moves->apply( pose_in );
		pymol_->apply( pose_in );
	}

	if( cter_separation > allowed_separation_ ) {
		loops::Loop one_loop( cdr_loop_end_, loop_end_, cdr_loop_end_+1, 0, false );
		simple_one_loop_fold_tree( pose_in, one_loop );
		CcdMoverOP ccd_moves = new CcdMover( one_loop, movemap_ );
		ccd_moves->apply( pose_in );
		pymol_->apply( pose_in );
	}

	Real separation = 0.00;
	for( Size ii = loop_start_; ii <= loop_end_; ii++ ) {
		peptide_C = pose_in.residue( ii ).xyz( C );
		peptide_N = pose_in.residue( ii + 1 ).xyz( N );
		separation=peptide_C.distance(peptide_N);
//		separation=distance(peptide_C, peptide_N);
		if( separation > allowed_separation_ ) {
			Size cutpoint = ii;
			loops::Loop one_loop( loop_start_, loop_end_, cutpoint, 0, false );
			CcdMoverOP ccd_moves = new CcdMover( one_loop, movemap_ );
			ccd_moves->apply( pose_in );
			pymol_->apply( pose_in );
		}
	}

	// reset to original foldtree
	pose_in.fold_tree( f );
} // Ab_CloseOneCDR_Mover::apply




}  // namespace antibody2
}  // namespace protocols
