// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/fold_from_loops/AlignByResidueSelectorMover.hh
/// @brief Aligns two poses through the selected residues
/// @author Jaume Bonet (jaume.bonet@gmail.com)

#ifndef INCLUDED_fold_from_loops_movers_AlignByResidueSelectorMover_hh
#define INCLUDED_fold_from_loops_movers_AlignByResidueSelectorMover_hh

// Unit headers
#include <protocols/fold_from_loops/movers/AlignByResidueSelectorMover.fwd.hh>
#include <protocols/moves/Mover.hh>

// Protocol headers

// Core headers
#include <core/pose/Pose.hh>
#include <core/select/residue_selector/ResidueSelector.hh>

// Basic/Utility headers
#include <basic/datacache/DataMap.fwd.hh>

namespace protocols {
namespace fold_from_loops {
namespace movers {

///@brief Adds constraints generated by ConstraintGenerators to a pose
class AlignByResidueSelectorMover : public protocols::moves::Mover {

public:
	AlignByResidueSelectorMover();

	// destructor (important for properly forward-declaring smart-pointer members)
	~AlignByResidueSelectorMover() override;

	void
	apply( core::pose::Pose & pose ) override;

	/// @brief parse XML tag (to use this Mover in Rosetta Scripts)
	void
	parse_my_tag(
		utility::tag::TagCOP tag,
		basic::datacache::DataMap & data,
		protocols::filters::Filters_map const & filters,
		protocols::moves::Movers_map const & movers,
		core::pose::Pose const & reference_pose ) override;

	void reference_pose( core::pose::PoseOP const & ref ) { reference_pose_->detached_copy( *ref ); }
	void reference_pose( core::pose::Pose const & ref )   { reference_pose_->detached_copy( ref ); }
	void reference_selector( core::select::residue_selector::ResidueSelectorCOP const &  select ) { reference_select_ = select; }
	void reference_selector( core::select::residue_selector::ResidueSelector const &  select ) { reference_select_ = select.clone(); }
	void query_selector( core::select::residue_selector::ResidueSelectorCOP const &  select ) { query_select_ = select; }
	void query_selector( core::select::residue_selector::ResidueSelector const &  select ) { query_select_ = select.clone(); }

	/// @brief required in the context of the parser/scripting scheme
	protocols::moves::MoverOP
	fresh_instance() const override;

	/// @brief required in the context of the parser/scripting scheme
	protocols::moves::MoverOP
	clone() const override;

	std::string
	get_name() const override;

	static
	std::string
	mover_name();

	static
	void
	provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd );

private:
	core::select::residue_selector::ResidueSelectorCOP reference_select_;
	core::select::residue_selector::ResidueSelectorCOP query_select_;
	core::pose::PoseOP reference_pose_;

};

}
} //protocols
} //fold_from_loops

#endif //INCLUDED_fold_from_loops_AlignByResidueSelectorMover_hh