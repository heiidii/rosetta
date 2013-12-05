// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/rotamer_sampler/McRotamer.hh
/// @brief Abstract Base Class for Markov chain rotamer sampler.
/// @author Fang-Chieh Chou


#ifndef INCLUDED_protocols_rotamer_sampler_McRotamer_HH
#define INCLUDED_protocols_rotamer_sampler_McRotamer_HH

// Unit headers
#include <protocols/rotamer_sampler/McRotamer.fwd.hh>

// Package headers
#include <protocols/rotamer_sampler/RotamerBase.hh>

namespace protocols {
namespace rotamer_sampler {

class McRotamer: public RotamerBase {
public:
	McRotamer():
		RotamerBase(),
		uniform_sampling_( false )
	{}

	virtual ~McRotamer(){}

	/// @brief Update the DOFs
	virtual void update() = 0;

	/// @brief Set uniform sampling (instead of Gaussian)
	virtual void set_uniform_sampling( bool const setting ) {
		uniform_sampling_ = setting;
	}

	/// @brief Get uniform sampling state
	virtual bool uniform_sampling() const {
		return uniform_sampling_;
	}

	/// @brief Name of the class
	virtual std::string get_name() const { return "McRotamer"; }

private:
	bool uniform_sampling_;
};

}
}
#endif
