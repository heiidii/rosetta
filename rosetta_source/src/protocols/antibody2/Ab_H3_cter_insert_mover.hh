// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer, email:license@u.washington.edu

/// @file protocols/antibody2/Ab_H3_cter_insert_mover.hh
/// @brief Build a homology model of an antibody2
/// @detailed
///
///
/// @author Jianqing Xu ( xubest@gmail.com )




#ifndef INCLUDED_protocols_antibody2_Ab_H3_cter_insert_mover_hh
#define INCLUDED_protocols_antibody2_Ab_H3_cter_insert_mover_hh


#include <protocols/antibody2/Ab_H3_cter_insert_mover.fwd.hh>
#include <core/fragment/FragData.hh>
#include <protocols/antibody2/AntibodyInfo.fwd.hh>
#include <protocols/loops/Loop.hh>
#include <protocols/moves/Mover.hh>
#include <protocols/moves/PyMolMover.fwd.hh>

#ifdef PYROSETTA
	#include <protocols/moves/PyMolMover.hh>
#endif


using namespace core;
namespace protocols {
namespace antibody2 {





//////////////////////////////////////////////////////////////////////////
/// @brief H3 CDR, Fragment Insertion and CCD
/// @details
class Ab_H3_cter_insert_mover : public protocols::moves::Mover {

public:
    /// @brief default constructor
	Ab_H3_cter_insert_mover();

	/// @brief constructor with arguments
	Ab_H3_cter_insert_mover(antibody2::AntibodyInfoOP antibody_info, bool camelid );


	/// @brief default destructor
	~Ab_H3_cter_insert_mover();

    void set_default();

	virtual void apply(pose::Pose & pose );
    virtual std::string get_name() const;

    // read CDR H3 C-terminal fragments (size: 4)
    void read_H3_cter_fragment( bool is_camelid);

    void turn_on_and_pass_the_pymol(moves::PyMolMoverOP pymol){
        use_pymol_diy_ = true;
        pymol_ = pymol;
    }

private:

    // CDR H3 C-terminal fragments
	utility::vector1< core::fragment::FragData > H3_base_library_;

    AntibodyInfoOP ab_info_;

    bool user_defined_;

    bool use_pymol_diy_;
    moves::PyMolMoverOP pymol_;

    /// @brief benchmark flag
	bool benchmark_;

    /// @brief is camelid antibody without light chain
	bool is_camelid_;


    void init(AntibodyInfoOP antibody_info, bool camelid, bool benchmark);
//    void setup_objects();
//    void finalize_setup( core::pose::Pose & pose );


    std::string H3_ter_library_filename_;

};






}//antibody2
}//protocols

#endif

