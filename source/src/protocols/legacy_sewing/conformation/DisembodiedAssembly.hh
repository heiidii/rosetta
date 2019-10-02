// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/legacy_sewing/conformation/DisembodiedAssembly.hh
///
/// @brief A set of simple container classes used by the SewingProtocol. These classes are used during the Hashing of coordinates, as well
/// as during assembly of low-resolution assemblies.
///
/// @author Tim Jacobs

#ifndef INCLUDED_protocols_legacy_sewing_conformation_DisembodiedAssembly_hh
#define INCLUDED_protocols_legacy_sewing_conformation_DisembodiedAssembly_hh

//Package headers


//Unit headers
#include <protocols/legacy_sewing/conformation/Assembly.hh>

//Protocol headers


//Utility headers
#include <utility/pointer/owning_ptr.functions.hh>

//C++ headers


namespace protocols {
namespace legacy_sewing  {

///@brief An Assembly is a collection of SewSegments. Assemblies are created using the
///geometric "compatibility" data generated by the Hasher.
class DisembodiedAssembly : public protocols::legacy_sewing::Assembly {

public:

	///@brief default constructor
	DisembodiedAssembly();

	AssemblyOP
	clone() override;

	void
	append_model(
		Model const & model,
		ScoreResult const & edge_score
	) override;

};



} //legacy_sewing namespace
} //protocols namespace

#endif
