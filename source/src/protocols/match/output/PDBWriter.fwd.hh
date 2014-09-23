// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
// :noTabs=false:tabSize=4:indentSize=4:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/match/output/PDBWriter.fwd.hh
/// @brief
/// @author Florian Richter (floric@u.washington.edu)

#ifndef INCLUDED_protocols_match_output_PDBWriter_fwd_hh
#define INCLUDED_protocols_match_output_PDBWriter_fwd_hh

// Utility headers
#include <utility/pointer/owning_ptr.hh>

namespace protocols {
namespace match {
namespace output {

class PDBWriter;
class CloudPDBWriter;
class PoseMatchOutputWriter;

typedef utility::pointer::shared_ptr< PDBWriter > PDBWriterOP;
typedef utility::pointer::shared_ptr< PDBWriter const > PDBWriterCOP;

typedef utility::pointer::shared_ptr< CloudPDBWriter > CloudPDBWriterOP;
typedef utility::pointer::shared_ptr< CloudPDBWriter const > CloudPDBWriterCOP;

typedef utility::pointer::shared_ptr< PoseMatchOutputWriter > PoseMatchOutputWriterOP;
typedef utility::pointer::shared_ptr< PoseMatchOutputWriter const > PoseMatchOutputWriterCOP;

}
}
}

#endif
