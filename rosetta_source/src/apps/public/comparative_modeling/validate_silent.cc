// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file apps/public/comparative_modeling/validate_silent.cc
/// @author Christopher Miles (cmiles@uw.edu)

// C/C++ headers
#include <iostream>
#include <string>

// Utility headers
#include <basic/options/option.hh>
#include <basic/options/keys/OptionKeys.hh>
#include <basic/options/keys/in.OptionKeys.gen.hh>
#include <basic/options/keys/out.OptionKeys.gen.hh>
#include <devel/init.hh>
#include <utility/exit.hh>
#include <utility/vector1.hh>

// Project headers
#include <core/io/silent/SilentFileData.hh>
#include <core/io/silent/SilentStruct.hh>
#include <core/sequence/util.hh>

#define PCT_THRESHOLD 0.9

using namespace std;

int main(int argc, char* argv[]) {
  using namespace basic::options;
  using namespace basic::options::OptionKeys;
  using namespace core::io::silent;
  devel::init(argc, argv);

  if (!option[in::file::fasta].user()) {
    utility_exit_with_message("Failed to provide required argument -in:file:fasta");
  }

  if (!option[in::file::silent].user()) {
    utility_exit_with_message("Failed to provide required argument -in:file:silent");
  }

  if (!option[out::file::silent].user()) {
    utility_exit_with_message("Failed to provide required argument -out:file:silent");
  }

  // Prevent the silent file parser from aborting on malformed input
  option[in::file::silent_read_through_errors].value(true);

  utility::vector1<string> sequences = core::sequence::read_fasta_file_str(option[in::file::fasta]()[1]);
  const string ref_sequence = sequences[1];
  const string input_file = option[in::file::silent]()[1];
  const string output_file = option[out::file::silent]();

  cout << "Reference: " << ref_sequence << endl;

  SilentFileData sfd_in, sfd_out;
  sfd_in.read_file(input_file);

  size_t num_good = 0, num_bad = 0;

  utility::vector1<string> tags = sfd_in.tags();
  for (utility::vector1<string>::const_iterator i = tags.begin(); i != tags.end(); ++i) {
    SilentStructOP decoy = sfd_in[*i];
    string sequence = decoy->sequence().one_letter_sequence();

    bool matches = ref_sequence.compare(0, ref_sequence.length(), sequence, 0, ref_sequence.length()) == 0;
    if (matches) {
      sfd_out.write_silent_struct(*decoy, output_file, false);
      ++num_good;
    } else {
      cerr << "Removed tag: " << *i << " seq: " << sequence << endl;
      ++num_bad;
    }
  }

  // print summary statistics
  double total = num_good + num_bad;
  double pct_good = num_good / total;
  double pct_bad = num_bad / total;
  cout << "pct_good: " << pct_good << " pct_bad: " << pct_bad << endl;

  // if the percentage of failures exceeds a threshold, flag the file as corrupt
  // to external callers through return codes
  return (pct_good >= PCT_THRESHOLD) ? 0 : 1;
}
