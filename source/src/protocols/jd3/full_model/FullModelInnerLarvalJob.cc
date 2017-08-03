// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   protocols/jd3/full_model/FullModelInnerLarvalJob.cc
/// @brief  Method definitions for the InnerLarvalJob class
/// @author Andy Watkins (amw579@stanford.edu)

// Unit headers
#include <protocols/jd3/full_model/FullModelInnerLarvalJob.hh>

//C++ headers
#include <string>
#include <sstream>

// Utility headers
#include <utility/vector1.hh>


#ifdef    SERIALIZATION
// Utility serialization headers
#include <utility/serialization/serialization.hh>

// Cereal headers
#include <cereal/types/polymorphic.hpp>
#endif // SERIALIZATION

namespace protocols {
namespace jd3 {
namespace full_model {

FullModelInnerLarvalJob::FullModelInnerLarvalJob() :
	InnerLarvalJob(),
	prelim_job_node_( 0 )
{}

FullModelInnerLarvalJob::FullModelInnerLarvalJob( core::Size nstruct ) :
	InnerLarvalJob( nstruct ),
	prelim_job_node_( 0 )
{}

FullModelInnerLarvalJob::FullModelInnerLarvalJob( core::Size nstruct, core::Size prelim_job_node ) :
	InnerLarvalJob( nstruct ),
	prelim_job_node_( prelim_job_node )
{}

FullModelInnerLarvalJob::FullModelInnerLarvalJob( FullModelInnerLarvalJob const & src ) :
	InnerLarvalJob( src ),
	prelim_job_node_( src.prelim_job_node_ )
{}


FullModelInnerLarvalJob::~FullModelInnerLarvalJob() = default;

/// @brief Mutual comparison of this inner job to the other inner job
/// so that if either one thinks it's not the same as the other, then
/// it returns false.  Invokes the same() function on both this and other
bool
FullModelInnerLarvalJob::operator == ( InnerLarvalJob const & other ) const
{
	if ( InnerLarvalJob::operator == ( other ) ) {
		FullModelInnerLarvalJob const & other_std = static_cast< FullModelInnerLarvalJob const & > ( other );
		return prelim_job_node_ == other_std.prelim_job_node_;
	}
	return false;
}

/// @details Since this is the base-class function, then by construction
/// other is equivalent to this.
/// @note classes derived from FullModelInnerLarvalJob must perform dynamic casts
/// to ensure the other FullModelInnerLarvalJob has the same type as them
bool
FullModelInnerLarvalJob::same_type( InnerLarvalJob const & other ) const
{
	return dynamic_cast< FullModelInnerLarvalJob const * > ( &other );
}

void
FullModelInnerLarvalJob::show( std::ostream & out ) const
{
	out << "FullModelInnerLarvalJob::show stubbed out";
}

std::ostream &
operator<< ( std::ostream & out, const FullModelInnerLarvalJob & inner_job )
{
	inner_job.show( out );
	return out;
}

core::Size FullModelInnerLarvalJob::prelim_job_node() const
{
	return prelim_job_node_;
}

void
FullModelInnerLarvalJob::prelim_job_node( core::Size setting )
{
	prelim_job_node_ = setting;
}

} // namespace full_model
} // namespace jd3
} // namespace protocols

#ifdef    SERIALIZATION

template< class Archive >
void
protocols::jd3::full_model::FullModelInnerLarvalJob::save( Archive & arc ) const {
	arc( cereal::base_class< protocols::jd3::InnerLarvalJob > ( this ) );
	arc( prelim_job_node_ );
}

template< class Archive >
void
protocols::jd3::full_model::FullModelInnerLarvalJob::load( Archive & arc ) {
	arc( cereal::base_class< protocols::jd3::InnerLarvalJob > ( this ) );
	arc( prelim_job_node_ );
}

SAVE_AND_LOAD_SERIALIZABLE( protocols::jd3::full_model::FullModelInnerLarvalJob );
CEREAL_REGISTER_TYPE( protocols::jd3::full_model::FullModelInnerLarvalJob )

CEREAL_REGISTER_DYNAMIC_INIT( protocols_jd3_full_model_FullModelInnerLarvalJob )
#endif // SERIALIZATION