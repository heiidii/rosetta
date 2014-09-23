// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file
/// @brief declaration of class for adducts on residues
/// @author Jim Havranek


#ifndef INCLUDED_core_chemical_Adduct_fwd_hh
#define INCLUDED_core_chemical_Adduct_fwd_hh


// Unit headers

// Project headers

// Utility headers
#include <utility/pointer/owning_ptr.hh>

// C++ headers
// Commented by inclean daemon #include <string>


namespace core {
namespace chemical {

class Adduct;
typedef utility::pointer::shared_ptr< Adduct > AdductOP;


}
}

#endif
