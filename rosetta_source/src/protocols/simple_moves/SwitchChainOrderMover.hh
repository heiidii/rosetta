// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file SwitchChainOrderMover.hh
/// @brief switch the chain order

#ifndef INCLUDED_protocols_simple_moves_SwitchChainOrderMover_hh
#define INCLUDED_protocols_simple_moves_SwitchChainOrderMover_hh

#include <protocols/simple_moves/SwitchChainOrderMover.fwd.hh>
#include <protocols/moves/Mover.hh>
#include <protocols/moves/DataMapObj.hh>

#include <protocols/moves/DataMap.fwd.hh>
#include <protocols/filters/Filter.fwd.hh>
#include <core/pose/Pose.fwd.hh>

// C++ Headers
#include <string>

#include <utility/vector1.hh>


namespace protocols {
namespace simple_moves {

class SwitchChainOrderMover : public moves::Mover {
public:
	SwitchChainOrderMover();
	// Undefinded, commenting out to fix PyRosetta build  SwitchChainOrderMover( std::string const & );

	virtual void apply( core::pose::Pose & pose );
	virtual std::string get_name() const;

	virtual moves::MoverOP clone() const;
	virtual moves::MoverOP fresh_instance() const;

	void chain_order( std::string const co );
	std::string chain_order() const;

	virtual void parse_my_tag(
		utility::tag::TagPtr const tag,
		protocols::moves::DataMap & data,
		protocols::filters::Filters_map const & filters,
		protocols::moves::Movers_map const & movers,
		core::pose::Pose const & pose );

private:
	std::string chain_order_;
	utility::pointer::owning_ptr< protocols::moves::DataMapObj< utility::vector1< core::Size > > > residue_numbers_; /// dflt NULL; a vector of residue numbers placed on the DataMap which need to be changed due to the chain order switch
};


} // simple_moves
} // protocols

#endif
