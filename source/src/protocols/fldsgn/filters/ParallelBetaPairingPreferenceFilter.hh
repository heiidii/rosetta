// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file ./src/protocols/fldsgn/filters/ParallelBetaPairingPreferenceFilter.hh
/// @brief header file for ParallelBetaPairingPreferenceFilter class.
/// @details
/// @author Nobuyasu Koga ( nobuyasu@uw.edu )


#ifndef INCLUDED_protocols_fldsgn_filters_ParallelBetaPairingPreferenceFilter_hh
#define INCLUDED_protocols_fldsgn_filters_ParallelBetaPairingPreferenceFilter_hh

// Unit Headers
#include <protocols/fldsgn/filters/ParallelBetaPairingPreferenceFilter.fwd.hh>

// Package Headers
#include <protocols/filters/Filter.hh>

// Project Headers
#include <core/chemical/AA.hh>
#include <core/pose/Pose.fwd.hh>

// Parser headers
#include <basic/datacache/DataMap.fwd.hh>
#include <protocols/filters/Filter.fwd.hh>
#include <utility/tag/Tag.fwd.hh>

#include <utility/vector1.hh>


//// C++ headers

namespace protocols {
namespace fldsgn {
namespace filters {

class ParallelBetaPairingPreferenceFilter : public protocols::filters::Filter {
public: // typedef

	typedef protocols::filters::Filter Super;

	typedef core::Size Size;
	typedef core::Real Real;
	typedef core::chemical::AA AA;
	typedef protocols::filters::Filter Filter;
	typedef std::string String;
	typedef protocols::filters::FilterOP FilterOP;
	typedef core::pose::Pose Pose;

	typedef utility::tag::TagCOP TagCOP;
	typedef basic::datacache::DataMap DataMap;


public:// constructor/destructor


	// @brief default constructor
	ParallelBetaPairingPreferenceFilter();

	// @brief constructor with arguments
	// Undefinded comminting out to fix PyRosetta build  ParallelBetaPairingPreferenceFilter( String const & ss );

	// @brief copy constructor
	ParallelBetaPairingPreferenceFilter( ParallelBetaPairingPreferenceFilter const & rval );

	// @brief destructor
	~ParallelBetaPairingPreferenceFilter() override{}


public:// virtual constructor


	// @brief make clone
	FilterOP clone() const override { return utility::pointer::make_shared< ParallelBetaPairingPreferenceFilter >( *this ); }

	// @brief make fresh instance
	FilterOP fresh_instance() const override { return utility::pointer::make_shared< ParallelBetaPairingPreferenceFilter >(); }


public:// set filter value


	void filter_value( Real const value );


public:// main calculator


	/// @brief compute number of contacts
	Real compute( Pose const & pose ) const;


public:// helper functions


	/// @brief
	Real score_pairmatrix( AA aa1, AA aa2 ) const;


public:// virtual main operations


	/// @brief used to report score
	Real report_sm( Pose const & pose ) const override;

	/// @brief used to report score
	void report( std::ostream & out, Pose const & pose ) const override;

	// @brief returns true if the given pose passes the filter, false otherwise.
	bool apply( Pose const & pose ) const override;


public:// parser


	void parse_my_tag( TagCOP tag,
		basic::datacache::DataMap &
	) override;

	std::string
	name() const override;

	static
	std::string
	class_name();

	static
	void
	provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd );

private:

	/// @brief Initialize this filter.
	void initialize_filter() const;


private:


	/// @brief
	Real filter_value_;

	/// @brief
	mutable utility::vector1< utility::vector1< Real > >  score_pairmatrix_;

	/// @brief
	bool verbose_;

	/// @brief Has this been initialized?
	mutable bool initialized_ = false;


};

} // filters
} // fldsgn
} // protocols

#endif
