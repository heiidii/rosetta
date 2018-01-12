// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   protocols/dna_dock/DNAClashCheckFilter.fwd.hh
/// @brief
/// @author Carl Walkey (cwalkey@u.washington.edu)


#ifndef INCLUDED_protocols_dna_dock_DNAClashCheckFilter_fwd_hh
#define INCLUDED_protocols_dna_dock_DNAClashCheckFilter_fwd_hh


// Utility headers
#include <utility/pointer/owning_ptr.fwd.hh>

namespace protocols {
namespace dna_dock {

// Forward
class DNAClashCheckFilter;

// Types
typedef utility::pointer::shared_ptr< DNAClashCheckFilter >  DNAClashCheckFilterOP;
typedef utility::pointer::shared_ptr< DNAClashCheckFilter const >  DNAClashCheckFilterCOP;

} // namespace dna_dock
} // namespace protocols

#endif