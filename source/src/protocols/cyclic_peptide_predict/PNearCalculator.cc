// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/cyclic_peptide_predict/PNearCalculator.cc
/// @brief A class to compute P_Near and DG_folding.
/// @details PNear = sum_i( exp(-(rmsd_i/lambda_i)^2 * exp(-E_i/kbt) ) / sum_j( -exp(-E_j/kbt) ).
/// DG_folding = -kbt*ln( PNear/( 1 - PNear ) )
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)

// Project headers:
#include <protocols/cyclic_peptide_predict/PNearCalculator.hh>

// Basic headers:
#include <basic/Tracer.hh>

// Utility headers:
#include <utility/pointer/memory.hh>

// C++ headers:
#include <cmath>

static basic::Tracer TR( "protocols.cyclic_peptide_predict.PNearCalculator" );


namespace protocols {
namespace cyclic_peptide_predict {

/// @brief Options constructor.
PNearCalculator::PNearCalculator(
	core::Real const lambda,
	core::Real const kbt
) {
	runtime_assert_string_msg( lambda > 0, "Error in PNearCalculator constructor: The value of lambda must be greater than zero!  Got " + std::to_string(lambda) + " instead." );
	runtime_assert_string_msg( kbt > 0, "Error in PNearCalculator constructor: The value of k_B*T must be greater than zero!  Got " + std::to_string(kbt) + " instead." );
	lambda_ = lambda;
	kbt_ = kbt;
}

/// @brief Destructor.
PNearCalculator::~PNearCalculator() = default;

/// @brief Clone operation: make a copy of this object, and return an owning pointer to the copy.
PNearCalculatorOP
PNearCalculator::clone() const {
	return utility::pointer::make_shared< PNearCalculator >( *this );
}

/*******************************************************************************
* PUBLIC MEMBER FUNCTIONS
*******************************************************************************/

/// @brief Add a data point to the set used to compute PNear.
void
PNearCalculator::add_data_point(
	core::Real const energy,
	core::Real const rmsd
) {
	if ( rmsd < 0 ) return; //Do nothing for negative rmsd values.

	// Is this the lowest energy we've seen?
	if ( energies_.empty() || energy < minenergy_ ) {
		minenergy_ = energy;
	}

	energies_.push_back(energy);
	rmsds_.push_back(rmsd);

}

/// @brief Compute PNear and DeltaG_folding given the data points that have been
/// added previously using add_data_point().
/// @details The pnear, Keq, and dgfolding values are overwritten by this operation.  (These
/// are the outputs.)
void
PNearCalculator::compute_pnear_and_dgfolding(
	core::Real &pnear,
	core::Real &Keq,
	core::Real &dgfolding
) const {
	core::Size const nentries(energies_.size());
	debug_assert( nentries == rmsds_.size() );
	core::Real numerator(0.0), denominator(0.0);

	core::Real const lambdasq( lambda_*lambda_ );

	for ( core::Size i(1); i<=nentries; ++i ) {
		core::Real const P( std::exp( -(energies_[i] - minenergy_)/kbt_ ) );
		numerator += std::exp( -rmsds_[i]*rmsds_[i]/lambdasq )*P;
		denominator += P;
	}

	if ( denominator > 1.0e-14 ) {
		pnear = numerator / denominator;
	} else {
		pnear = 0.0;
	}
	Keq = ( pnear < 0.999999999 ? pnear/(1.0-pnear) : 0.999999999/(1.0-0.999999999) );
	if ( Keq < 1.0e-10 ) Keq = 1.0e-10;
	dgfolding = -kbt_*std::log( Keq );
}


} //cyclic_peptide_predict
} //protocols
