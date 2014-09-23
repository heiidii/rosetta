// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @brief  Class to write kinemage-formatted output for Residue and Conformation
/// @file   core/conformation/ResidueKinWriter.hh
/// @author Andrew Leaver-Fay

#ifndef INCLUDED_core_conformation_ResidueKinWriter_fwd_hh
#define INCLUDED_core_conformation_ResidueKinWriter_fwd_hh


// Utility headers
#include <utility/pointer/owning_ptr.hh>

namespace core {
namespace conformation {

class ResidueKinWriter;
class ConformationKinWriter;

typedef utility::pointer::shared_ptr< ResidueKinWriter > ResidueKinWriterOP;
typedef utility::pointer::shared_ptr< ResidueKinWriter const > ResidueKinWriterCOP;

typedef utility::pointer::shared_ptr< ConformationKinWriter > ConformationKinWriterOP;
typedef utility::pointer::shared_ptr< ConformationKinWriter const > ConformationKinWriterCOP;

} // conformation
} // core


#endif
