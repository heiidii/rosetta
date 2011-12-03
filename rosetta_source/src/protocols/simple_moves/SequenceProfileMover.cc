// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/simple_moves/SequenceProfileMover.cc
/// @brief  BS mover to get around a stupid "mover" that was embedded in the parser
/// @author Brian Weitzner brian.weitzner@gmail.com, Steven Lewis smlewi@gmail.com
/// @date   Rebased to next year.

// Unit Headers
#include <protocols/simple_moves/SequenceProfileMover.hh>
#include <protocols/simple_moves/SequenceProfileMoverCreator.hh>


// Project headers
#include <core/types.hh>
#include <core/pose/Pose.hh>

#include <protocols/filters/Filter.hh>
#include <protocols/filters/FilterFactory.hh>
#include <protocols/filters/BasicFilters.hh>
#include <utility/tag/Tag.hh>
#include <core/scoring/ScoreType.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/ScoreFunctionFactory.hh>

#include <protocols/jd2/JobDistributor.hh>
#include <protocols/jd2/Job.hh>
#include <core/scoring/constraints/SequenceProfileConstraint.hh>
#include <core/sequence/SequenceProfile.hh> 
#include <utility/file/FileName.hh>

#include <protocols/moves/DataMap.hh>

#include <basic/Tracer.hh>
#include <string>

static basic::Tracer TR( "protocols.simple_moves.SequenceProfileMover" );

namespace protocols {
namespace simple_moves {

SequenceProfileMover::SequenceProfileMover() :
	Mover( "SequenceProfileMover" )
{
}

SequenceProfileMover::~SequenceProfileMover() {}

void
SequenceProfileMover::apply( core::pose::Pose & pose )
{
    using namespace core::sequence;
    
    SequenceProfileOP profile = new SequenceProfile;
    profile->read_from_checkpoint( cst_file_name_ );
    for( core::Size seqpos( 1 ), end( pose.total_residue() ); seqpos <= end; ++seqpos )
        pose.add_constraint( new core::scoring::constraints::SequenceProfileConstraint( pose, seqpos, profile ) );
    
    TR << "Added sequence profile constraints specified in file " << cst_file_name_ << "." << std::endl;
    
    TR << "This code is included in the test!" << std::endl;
}

std::string
SequenceProfileMover::get_name() const {
	return SequenceProfileMoverCreator::mover_name();
}

protocols::moves::MoverOP
SequenceProfileMover::clone() const{
	return protocols::moves::MoverOP( new SequenceProfileMover( *this ) );
}

protocols::moves::MoverOP
SequenceProfileMover::fresh_instance() const{
	return protocols::moves::MoverOP( new SequenceProfileMover );
}

void
SequenceProfileMover::parse_my_tag( TagPtr const tag, moves::DataMap & data, protocols::filters::Filters_map const &, moves::Movers_map const &, core::pose::Pose const & pose )
{
	using namespace core::scoring;
    
    //handle cst_file_name
    protocols::jd2::JobOP job( protocols::jd2::JobDistributor::get_instance()->current_job() );
    std::string const input_file_name( job->input_tag() );
    core::Size const wheres_period( input_file_name.find_first_of( "." ) );
    std::string const dflt_cst_file_name( input_file_name.substr(0, wheres_period ) + ".cst" );
    set_cst_file_name( tag->getOption< std::string >( "file_name", dflt_cst_file_name ) );

    //handle profile_wgt
    set_profile_wgt( tag->getOption< core::Real >( "weight", 0.25 ) );
 
    /*
    if( profile_wgt_ ) {
        using namespace utility::pointer;
		for( std::map< std::string, ReferenceCountOP >::const_iterator it=data[ "scorefxns" ].begin(); it!=data[ "scorefxns" ].end(); ++it ){
			ScoreFunctionOP scorefxn( *data.get< ScoreFunction * >( "scorefxns", it->first ) );
			scorefxn->set_weight( res_type_constraint, profile_wgt_ );
            TR << "setting " << it->first << " res_type_constraint to " << profile_wgt_ << "\n";
		}
	}
        
    TR << "Changed all scorefxns to have profile weights of " << profile_wgt_ << std::endl;
   */
} //end parse_my_tag

std::string
SequenceProfileMoverCreator::keyname() const
{
	return SequenceProfileMoverCreator::mover_name();
}

protocols::moves::MoverOP
SequenceProfileMoverCreator::create_mover() const {
	return new SequenceProfileMover;
}

std::string
SequenceProfileMoverCreator::mover_name()
{ // This name was chosen for compatibility reasons - this name was hard coded into the logic of the DockDesignParser in r46190
	return "profile";
}


} // simple_moves
} // protocols

