// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/multistage_rosetta_scripts/MRSJob.cc
/// @brief
/// @detailed
/// @author Jack Maguire, jack@med.unc.edu


#include <protocols/multistage_rosetta_scripts/MRSJob.hh>

#include <basic/Tracer.hh>

#include <core/scoring/ScoreFunction.hh>
#include <core/pose/Pose.hh>

#include <protocols/moves/Mover.hh>
#include <protocols/moves/MoverFactory.hh>
#include <protocols/filters/Filter.hh>
#include <protocols/filters/FilterFactory.hh>
#include <protocols/jd3/standard/MoverAndPoseJob.hh>

#include <utility/tag/Tag.hh>
#include <utility/pointer/memory.hh>

static basic::Tracer TR( "protocols.multistage_rosetta_scripts.MRSJob" );

using namespace utility;

namespace protocols {
namespace multistage_rosetta_scripts {

//Constructor
MRSJob::MRSJob( core::Size num_times_to_repeat, core::Size max_num_results ) :
	max_num_results_( max_num_results ),
	num_times_to_repeat_ (num_times_to_repeat )
{}

//Destructor
MRSJob::~MRSJob()
{}

jd3::CompletedJobOutput MRSJob::run() {
	runtime_assert( pose_ );
	core::pose::PoseOP orig_pose = pose_;

	jd3::CompletedJobOutput output;
	debug_assert( num_times_to_repeat_ );
	for ( core::Size ii = 1; ii <= num_times_to_repeat_; ++ii ) {
		if ( ii != num_times_to_repeat_ ) {
			pose_ = orig_pose->clone();
		} else {
			pose_ = orig_pose;
		}

		if ( ii != 1 ) {
			for ( mover_or_filter & mof : protocols_ ) {
				if ( mof.mover ) {
					mof.mover = mof.mover->clone();
				}

				if ( mof.filter ) {
					mof.filter = mof.filter->clone();
				}
			}
		}

		jd3::CompletedJobOutput output_ii = run_inner();
		for ( auto & jr_ii : output_ii.job_results ) {
			output.job_results.push_back( jr_ii );
		}
	}
	if ( output.job_results.size() ) {
		output.status = jd3::jd3_job_status_success;
	} else {
		output.status = jd3::jd3_job_status_failed_do_not_retry;
	}

	return output;
}

jd3::CompletedJobOutput MRSJob::run_inner() {

	jd3::CompletedJobOutput output;
	output.status = jd3::jd3_job_status_failed_do_not_retry;//status has no protocols

	//store which protocol is the final mover in case we want to call get_additional_output()
	//this is done in a somewhat inefficient way - but it too short to matter and I hope it makes it more clear what we are doing
	core::Size index_of_last_mover = 0;
	core::Size count = 0;
	for ( mover_or_filter const & mf : protocols_ ) {
		debug_assert( !mf.mover != !mf.filter );//xor

		++count;
		if ( mf.is_mover ) {
			index_of_last_mover = count;
			debug_assert( mf.mover );
		}
	}

	count = 0;
	for ( std::list< mover_or_filter >::const_iterator it = protocols_.begin();
			it != protocols_.end(); ++it ) {

		++count;
		mover_or_filter const & protocol = (*it);//TODO clone here?
		if ( protocol.is_mover ) {
			debug_assert( pose_ );
			protocol.mover->apply( *pose_ );

			if ( count == index_of_last_mover && max_num_results_ != 1 ) {
				core::Size output_count = 1;
				while ( pose_ && ( !max_num_results_ || output_count <= max_num_results_ ) ) {

					//run rest of the protocol
					for ( std::list< mover_or_filter >::const_iterator it2 = std::next( it );
							it2 != protocols_.end(); ++it2 ) {
						mover_or_filter const & filter_protocol = (*it2);

						//has to be filter
						debug_assert( filter_protocol.filter );
						debug_assert( ! filter_protocol.mover );
						if ( !filter_protocol.filter->apply( *pose_ ) ) {
							break;
						}
						if ( std::next( it2 ) == protocols_.end() ) {
							core::Real score = filter_protocol.filter->score( *pose_ );
							if ( positive_scores_are_better_ ) score *= -1;//The queen will sort assuming negative scores are better
							jd3::JobSummaryOP summary( pointer::make_shared< jd3::standard::EnergyJobSummary >( score ) );
							jd3::JobResultOP result( pointer::make_shared< jd3::standard::PoseJobResult >( pose_ ) );
							output.job_results.push_back( std::make_pair( summary, result ) );
							output.status = jd3::jd3_job_status_success;
						}

					}
					pose_ = protocol.mover->get_additional_output();
					++output_count;
				}
				break;
			}

		} else {
			if ( !protocol.filter->apply( *pose_ ) ) {
				output.status = jd3::jd3_job_status_failed_do_not_retry;
				return output;
			}

			if ( std::next( it ) == protocols_.end() ) {
				core::Real const score = protocol.filter->report_sm( *pose_ );
				jd3::JobSummaryOP summary( pointer::make_shared< jd3::standard::EnergyJobSummary >( score ) );
				jd3::JobResultOP result( pointer::make_shared< jd3::standard::PoseJobResult >( pose_ ) );
				output.job_results.push_back( std::make_pair( summary, result ) );
				output.status = jd3::jd3_job_status_success;
			}
		}
	}//for it

	if ( max_num_results_ ) {
		debug_assert( output.job_results.size() <= max_num_results_ );
	}

	return output;
}

void
MRSJob::parse_my_tag(
	utility::tag::TagCOP stage_subtag,
	basic::datacache::DataMap & map,
	filters::Filters_map const & filters,
	moves::Movers_map const & movers,
	core::pose::Pose const & pose,
	std::map< std::string, utility::tag::TagCOP > const & mover_tags_by_name,
	std::map< std::string, utility::tag::TagCOP > const & filter_tags_by_name
) {

	moves::MoverFactory const * mover_factory = moves::MoverFactory::get_instance();
	filters::FilterFactory const * filter_factory = filters::FilterFactory::get_instance();

	utility::vector0< utility::tag::TagCOP > const & subtags = stage_subtag->getTags();
	for ( unsigned short int i = 0; i<subtags.size(); ++i ) {
		utility::tag::TagCOP subtag = subtags[ i ];

		if ( subtag->getName() == "Add"  ) { //not sort
			std::string const mover_name = subtag->getOption< std::string >( "mover",
				subtag->getOption< std::string >( "mover_name", "" )
			);//either one works

			if ( mover_name.size() ) {
				auto iter = mover_tags_by_name.find( mover_name );
				if ( iter == mover_tags_by_name.end() ) {
					utility_exit_with_message( "No Mover Named: " + mover_name );
				}
				utility::tag::TagCOP mover_tag = iter->second;
				protocols_.push_back( mover_or_filter( mover_factory->newMover( mover_tag, map, filters, movers, pose ) ) );
				debug_assert( protocols_.back().mover );
			}
		} else {
			positive_scores_are_better_ = ! subtag->getOption< bool >( "negative_score_is_good", true );
		}

		std::string const filter_name = subtag->getOption< std::string >( "filter",
			subtag->getOption< std::string >( "filter_name", "" )
		);
		if ( filter_name.size() ) {
			auto iter = filter_tags_by_name.find( filter_name );
			if ( iter == filter_tags_by_name.end() ) {
				utility_exit_with_message( "No Filter Named: " + filter_name );
			}
			utility::tag::TagCOP filter_tag = iter->second;
			protocols_.push_back( mover_or_filter( filter_factory->newFilter( filter_tag, map, filters, movers, pose ) ) );
			debug_assert( protocols_.back().filter );
		} else if ( i == subtags.size() - 1 ) {
			utility_exit_with_message( "All stages need to finish with a filter." );
		}

	}
}

core::Real
MRSJob::metric_for_evaluation( core::pose::PoseOP pose ) const {
	return protocols_.back().filter->score( *pose );
}

} //multistage_rosetta_scripts
} //protocols