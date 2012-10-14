// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
// :noTabs=false:tabSize=4:indentSize=4:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   core/fragments/ConstantLengthFragSet.cc
/// @brief  set of fragments for a certain alignment frame
/// @author Oliver Lange (olange@u.washington.edu)
/// @author James Thompson (tex@u.washington.edu)

// Unit Headers
#include <core/fragment/ConstantLengthFragSet.hh>

// Package Headers
#ifdef WIN32
#include <core/fragment/FragID.hh>
#endif

#include <core/fragment/BBTorsionSRFD.hh>
#include <core/fragment/BBTorsionSRFD.fwd.hh>
#include <core/fragment/Frame.hh>
#include <core/fragment/FragData.hh>
#include <core/fragment/ConstantLengthFragSetIterator_.hh>

// Project Headers
#include <core/types.hh>

// ObjexxFCL Headers
#include <ObjexxFCL/string.functions.hh>

// Utility headers
#include <utility/vector1.fwd.hh>
#include <utility/io/izstream.hh>
#include <utility/pointer/owning_ptr.hh>
#include <basic/Tracer.hh>

// C/C++ headers
#include <algorithm>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

#include <core/fragment/FrameIterator.hh>
#include <utility/vector1.hh>

namespace ObjexxFCL { namespace fmt { } } using namespace ObjexxFCL::fmt;

namespace core {
namespace fragment {

using namespace kinematics;
using namespace ObjexxFCL;

static basic::Tracer tr("core.fragments.ConstantLengthFragSet");

ConstantLengthFragSet::~ConstantLengthFragSet() {}

ConstantLengthFragSet::ConstantLengthFragSet( Size frag_length, std::string filename ) {
		set_max_frag_length( frag_length );
		read_fragment_file(filename);
	}

// if fragments are of differing length assertion in add will fail
ConstantLengthFragSet::ConstantLengthFragSet( FragSet const& fragments ) {
	set_max_frag_length( fragments.max_frag_length() );
	Parent::add( fragments );
}

///@brief there is only one Frame per position, end / max_overlap are ignored
Size ConstantLengthFragSet::region(
	MoveMap const& mm,
	core::Size start,
	core::Size end,
	core::Size , //min_overlap not used
	core::Size , //min_length not used
	FrameList &frames
) const {
	Size count( 0 );
	for ( Size pos=start; pos<=frames_.size() && pos<=end; pos++ ) {
		if ( frames_[pos] ) {
			if ( frames_[pos]->is_applicable( mm ) && frames_[pos]->is_valid() ) {
				frames.push_back( frames_[ pos ] );
				count++;
			}
		}
	}
	return count;
}

// This method will only be called if the frame has been incompatible with
// existing frames. In case of ConstantLengthFragSet aka single-kind-of Frame
// FragSet this means position at aframe->start() is empty
void ConstantLengthFragSet::add_( FrameOP aframe ) {
	if ( max_frag_length() ) runtime_assert( aframe->length() == max_frag_length() );
	Size seqpos( aframe->start() );
	if ( frames_.size() < seqpos ) {
		frames_.resize( seqpos, NULL );
	}
	runtime_assert( frames_[ seqpos ] == 0 ); //I consider this as error... don't add incompatible frames to a ConstantLengthFragSet
	frames_[ seqpos ] = aframe;
}

void ConstantLengthFragSet::read_fragment_file( std::string filename, Size top25, Size ncopies, bool bAnnotation ) {
	using std::cerr;
	using std::endl;

	utility::io::izstream data( filename );
	if ( !data.good() ) {
		cerr << "Open failed for file: " << data.filename() << endl;
		utility::exit( EXIT_FAILURE, __FILE__, __LINE__);
	}

	read_fragment_stream(data,top25,ncopies,bAnnotation);
}

void ConstantLengthFragSet::read_fragment_stream( utility::io::izstream & data, Size top25, Size ncopies, bool bAnnotation ) {
	using namespace ObjexxFCL;
	using namespace ObjexxFCL::fmt;
	using std::endl;
	using std::istringstream;
	using std::string;

	Real score = 0.0;
	string line;

	Size insertion_pos = 1;
	FragDataOP current_fragment( NULL );
	FrameOP frame;

	Size n_frags( 0 );
	while ( getline( data, line ) ) {
		// skip blank lines
		if ( line == "" || line == " " ) {
			// add current_fragment to the appropriate frame
			if ( current_fragment && frame ) {
				if ( !top25 || frame->nr_frags() < top25*ncopies ) {
					current_fragment->set_valid(); //it actually contains data
					if ( !frame->add_fragment( current_fragment ) ) {
						tr.Fatal << "Incompatible Fragment in file: " << data.filename() << endl;
						utility::exit( EXIT_FAILURE, __FILE__, __LINE__);
					} else {
						for ( Size i = 2; i <= ncopies; i++ ) frame->add_fragment( current_fragment );
					}
				}
			}

			// create a new current_fragment
			current_fragment = NULL;
			continue;
		}

		// skip lines beginning with # characters, representing comments
		// save score if it exists
		if ( line.substr(0,1) == "#" ) {
			istringstream in( line );
			string tag;
			in >> tag;
			if (!in.eof()) {
				in >> tag;
				if (tag == "score" && !in.eof()) {
					in >> score;
				}
			}
			continue;
		}

		// skip lines that are too short?? --- such a stupid thing
		if ( line.length() < 22 ) {
			tr.Warning << "line too short: Skipping line '" << line << "'" << endl;
			continue;
		}

		// skip position and neighbor lines for now
		if ( line.find("position") != string::npos ) {
			istringstream in( line );
			string tag;
			in >> tag;
			in >> insertion_pos;

			// create a new frame
			if ( frame && frame->is_valid() ) {
				add( frame );
				n_frags = std::max( n_frags, frame->nr_frags() );
			}
			frame = new Frame( insertion_pos );
			continue;
		}

		// actual reading happens here
		string pdbid = line.substr(1, 4);
		char chain = char_of(line.substr(6, 1));
		int aa_index = int_of(line.substr(8, 5));
		char aa = char_of(line.substr(14, 1));
		char ss = char_of(line.substr(16, 1));
		Real phi = float_of(line.substr(18, 9));
		Real psi = float_of(line.substr(27, 9));
		Real omega = float_of(line.substr(36, 9));

    BBTorsionSRFDOP res = new BBTorsionSRFD(3, ss, aa);  // 3 protein torsions

		// set torsions
		res->set_torsion(1, phi);
		res->set_torsion(2, psi);
		res->set_torsion(3, omega);

		// optionally read in and set cartesian coordinates for the residue's CA
		if (line.length() > 45) {
			Real x = float_of(line.substr(45, 9));
			Real y = float_of(line.substr(54, 9));
			Real z = float_of(line.substr(64, 9));
			res->set_coordinates(x, y, z);
			//tr.Info << "Read cartesian coordinates (" << x << "," << y << "," << z << ")" << endl;
		}

		// set predicted secondary structure
		res->set_secstruct(ss);

		if ( !current_fragment ) {
			if ( bAnnotation ) {
				current_fragment = new AnnotatedFragData( pdbid, aa_index, chain );
			}
			else {
				current_fragment = new FragData;
			}
		}
		current_fragment->add_residue(res);
		current_fragment->set_score(score);
	} // while ( getline( data, line ) )

	if ( frame && frame->is_valid() ) {
		add( frame );
		n_frags = std::max( n_frags, frame->nr_frags() );
	}

	tr.Info << "finished reading top " << n_frags << " "
					<< max_frag_length() << "mer fragments from file " << data.filename()
					<< endl;
}

FrameIterator ConstantLengthFragSet::begin() const {
	return FrameIterator( new ConstantLengthFragSetIterator_( frames_.begin(), frames_.end() ) );
}

FrameIterator ConstantLengthFragSet::end() const {
	return FrameIterator( new ConstantLengthFragSetIterator_( frames_.end(), frames_.end() ) );
}

} //fragment
} //core
