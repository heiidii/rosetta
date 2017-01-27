// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/simple_moves/PertMinMoverCreator.hh
/// @brief Random perturbation, followed by minimization
/// @author Rocco Moretti (rmorettiase@gmail.com)

#ifndef INCLUDED_protocols_simple_moves_PertMinMoverCreator_hh
#define INCLUDED_protocols_simple_moves_PertMinMoverCreator_hh

#include <protocols/moves/MoverCreator.hh>

namespace protocols {
namespace simple_moves {

class PertMinMoverCreator : public protocols::moves::MoverCreator {

public:

	virtual protocols::moves::MoverOP create_mover() const override;
	virtual std::string keyname() const override;
	void provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) const override;
};

} //protocols
} //simple_moves

#endif //INCLUDED_protocols/simple_moves_PertMinMover_fwd_hh



