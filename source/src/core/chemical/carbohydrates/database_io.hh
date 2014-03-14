// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file    core/chemical/carbohydrates/database_io.hh
/// @brief   Database input/output function declarations for carbohydrate-specific data.
/// @author  Labonte

#ifndef INCLUDED_core_chemical_carbohydrates_database_io_HH
#define INCLUDED_core_chemical_carbohydrates_database_io_HH

// Project headers
#include <core/types.hh>

// Utility headers
#include <utility/vector1.hh>
#include <utility/io/izstream.hh>

// C++ headers
#include <map>
#include <string>


namespace core {
namespace chemical {
namespace carbohydrates {

/// @brief  Local method that opens a file and returns its data as a list of lines after checking for errors.
utility::vector1<std::string> get_lines_from_file_data(std::string const & filename);


/// @brief  Return a list of strings, which are saccharide-specific properties and modifications, read from a database
/// file.
utility::vector1<std::string> read_properties_from_database_file(std::string const & filename);

/// @brief  Return a map of strings to strings, which are saccharide-specific 3-letter codes mapped to IUPAC roots, read
/// from a database file.
std::map<std::string, std::string> read_codes_and_roots_from_database_file(std::string const & filename);

}  // namespace carbohydrates
}  // namespace chemical
}  // namespace core

#endif  // INCLUDED_core_chemical_carbohydrates_database_io_HH
