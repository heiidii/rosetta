// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/antibody/design/util.hh
/// @brief 
/// @author Jared Adolf-Bryfogle (jadolfbr@gmail.com)

#ifndef INCLUDED_protocols_antibody_design_UTIL_HH
#define INCLUDED_protocols_antibody_design_UTIL_HH

#include <string>
#include <core/types.hh>
#include <utility/vector1.hh>
#include <map>

namespace protocols {
namespace antibody{
namespace design{
using namespace protocols::antibody;
using namespace utility;

///@brief Returns (?,?,?) With question marks of length n to help create database query using IN operator
std::string
get_string_for_IN(core::Size n);

///@brief Gets all possible graft permutations. 
void
get_all_graft_permutations(
	vector1<core::Size > & total_cdr_set,
	vector1<vector1< core::Size > > & all_permutations,
	vector1< core::Size >current_index,
	core::Size const recurse_index);
		
} //design
} //antibody
} //protocols


#endif	//#ifndef INCLUDED_protocols/antibody/design_UTIL_HH

