// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/nonlocal/PolicyFactory.hh
/// @author Christopher Miles (cmiles@uw.edu)

#ifndef PROTOCOLS_NONLOCAL_POLICYFACTORY_HH_
#define PROTOCOLS_NONLOCAL_POLICYFACTORY_HH_

// C/C++ headers
// AUTO-REMOVED #include <string>

// Project headers
#include <core/types.hh>
#include <core/fragment/FragSet.fwd.hh>

// Package headers
#include <protocols/nonlocal/Policy.fwd.hh>

#include <string>

namespace protocols {
namespace nonlocal {

class PolicyFactory {
 public:
  /// @brief If a valid policy name was provided, returns a Policy object of the
  /// given type, exits otherwise. The resulting instance is constructed with a
  /// copy of the given set of fragments, which may be optionally filtered by
  /// rank (see num_fragments).
  static PolicyOP get_policy(const std::string& policy_name,
                             core::fragment::FragSetCOP fragments,
                             core::Size num_fragments = core::SZ_MAX);
};

}  // namespace nonlocal
}  // namespace protocols

#endif  // PROTOCOLS_NONLOCAL_POLICYFACTORY_HH_
