// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   core/scoring/etable/count_pair/CountPairFactory.fwd.hh
/// @brief
/// @author Andrew Leaver-Fay (leaverfa@email.unc.edu)

// Unit Headers
#include <core/scoring/etable/count_pair/CountPairFactory.hh>

// Package Headers
#include <core/scoring/etable/count_pair/types.hh>
#include <core/scoring/etable/count_pair/CountPairAll.hh>
#include <core/scoring/etable/count_pair/CountPairFunction.fwd.hh>
#include <core/scoring/etable/count_pair/CountPair1B.hh>
#include <core/scoring/etable/count_pair/CountPair2B.hh>
#include <core/scoring/etable/count_pair/CountPairIntraRes.hh>
#include <core/scoring/etable/count_pair/CountPairGeneric.hh>

#include <core/scoring/etable/count_pair/CountPairCrossover3.hh>
#include <core/scoring/etable/count_pair/CountPairCrossover3full.hh>
#include <core/scoring/etable/count_pair/CountPairCrossover4.hh>
#include <core/scoring/etable/count_pair/CountPairCrossover34.hh>


// Project Headers
#include <core/chemical/ResidueConnection.hh>
#include <core/conformation/Residue.hh>
#include <core/scoring/etable/atom_pair_energy_inline.hh>

//Auto Headers
#include <utility/vector1.hh>


namespace core {
namespace scoring {
namespace etable {
namespace count_pair {

CountPairFunctionOP
CountPairFactory::create_count_pair_function(
	conformation::Residue const & res1,
	conformation::Residue const & res2,
	CPCrossoverBehavior crossover
)
{
	CPResidueConnectionType connection = determine_residue_connection( res1, res2 );
	CountPairFunctionOP cpfxn;

	if ( connection != CP_NO_BONDS ) {
		switch ( connection ) {
		case CP_NO_BONDS :
			utility_exit_with_message("Can't create count pair function for two unbonded residues.");
			break;
		case CP_ONE_BOND : {
			// scope for res1connat, res2connat initializations
			debug_assert( res1.connections_to_residue( res2 ).size() == 1 );
			debug_assert( res2.connections_to_residue( res1 ).size() == 1 );

			Size res1connat = res1.residue_connection( res1.connections_to_residue( res2 )[ 1 ] ).atomno();
			Size res2connat = res2.residue_connection( res2.connections_to_residue( res1 )[ 1 ] ).atomno();
			switch ( crossover ) {
			case CP_CROSSOVER_3 :
				cpfxn = utility::pointer::make_shared< CountPair1B< CountPairCrossover3 > >( res1, res1connat, res2, res2connat );
				break;
			case CP_CROSSOVER_3FULL :
				cpfxn = utility::pointer::make_shared< CountPair1B< CountPairCrossover3full > >( res1, res1connat, res2, res2connat );
				break;
			case CP_CROSSOVER_4 :
				cpfxn = utility::pointer::make_shared< CountPair1B< CountPairCrossover4 > >( res1, res1connat, res2, res2connat );
				break;
			case CP_CROSSOVER_34 :
				cpfxn = utility::pointer::make_shared< CountPair1B< CountPairCrossover34 > >( res1, res1connat, res2, res2connat );
				break;
			}
		}
			break;
		default : {
			CountPairGenericOP gcpfxn( new CountPairGeneric( res1, res2 ) );
			switch ( crossover ) {
			case CP_CROSSOVER_3 :
			case CP_CROSSOVER_3FULL :
				gcpfxn->set_crossover( 3 );
				break;
			case CP_CROSSOVER_4 :
				gcpfxn->set_crossover( 4 );
				break;
			case CP_CROSSOVER_34 :
				gcpfxn->set_crossover( 4 );
				break;
			}

			cpfxn = gcpfxn;
		}
			break;
		}
	} else if ( crossover == CP_CROSSOVER_34 && ( res1.polymeric_sequence_distance( res2 ) == 2 ) ) {
		Size midway( res1.seqpos() > res2.seqpos() ? res2.seqpos() + 1 : res1.seqpos() + 1 );
		// scope for res1connat, res2connat initializations
		debug_assert( res1.connections_to_residue( midway ).size() == 1 );
		debug_assert( res2.connections_to_residue( midway ).size() == 1 );

		Size res1connat = res1.residue_connection( res1.connections_to_residue( midway )[ 1 ] ).atomno();
		Size res2connat = res2.residue_connection( res2.connections_to_residue( midway )[ 1 ] ).atomno();
		cpfxn = utility::pointer::make_shared< CountPair2B< CountPairCrossover34 > >( res1, res1connat, res2, res2connat );
	} else {
		cpfxn = utility::pointer::make_shared< CountPairAll >();
	}
	return cpfxn;
}


/// @details Instantiate a count-pair function on the *stack* and rely on an
/// invoker to call a function of the newly created count-pair function.  The
/// count-pair function is removed from the stack without new or delete having
/// been called.
void
CountPairFactory::create_count_pair_function_and_invoke(
	conformation::Residue const & res1,
	conformation::Residue const & res2,
	CPCrossoverBehavior crossover,
	Invoker & invoker
)
{
	CPResidueConnectionType connection = determine_residue_connection( res1, res2 );

	if ( connection != CP_NO_BONDS ) {
		switch ( connection ) {
		case CP_NO_BONDS :
			utility_exit_with_message("Can't create count pair function for two unbonded residues.");
			break;
		case CP_ONE_BOND : {
			// scope for res1connat, res2connat initializations
			Size res1connat, res2connat;
			try {
				debug_assert( res1.connections_to_residue( res2 ).size() == 1 );
				// Is it possible to be asymmetric -- could res2 have no connections to res1?
				if ( res2.connections_to_residue( res1 ).size() == 0 ) {
					std::cout << res1.seqpos() << " " << res2.seqpos() << std::endl;
					std::cout << res1.type().name() << " " << res2.type().name() << std::endl;
				}
				debug_assert( res2.connections_to_residue( res1 ).size() == 1 );
				res1connat = res1.residue_connection( res1.connections_to_residue( res2 )[ 1 ] ).atomno();
				res2connat = res2.residue_connection( res2.connections_to_residue( res1 )[ 1 ] ).atomno();
			} catch ( utility::excn::Exception const & e ) {
				// At Steven's behest I removed this tracer
				//TR.Error << "CP_ONE_BOND logic; accessing connections of " << res1.seqpos() << " to " << res2.seqpos() << " or vice versa is problematic " << std::endl;
				throw e;
			}
			switch ( crossover ) {
			case CP_CROSSOVER_3 : {
				CountPair1B< CountPairCrossover3 > cpfxn( res1, res1connat, res2, res2connat );
				invoker.invoke( cpfxn );
			}
				break;
			case CP_CROSSOVER_3FULL : {
				CountPair1B< CountPairCrossover3full > cpfxn( res1, res1connat, res2, res2connat );
				invoker.invoke( cpfxn );
			}
				break;
			case CP_CROSSOVER_4 : {
				CountPair1B< CountPairCrossover4 > cpfxn( res1, res1connat, res2, res2connat );
				invoker.invoke( cpfxn );
			}
				break;
			case CP_CROSSOVER_34 : {
				CountPair1B< CountPairCrossover34 > cpfxn( res1, res1connat, res2, res2connat );
				invoker.invoke( cpfxn );
			}
				break;
			}
		}
			break;
		default : {
			CountPairGeneric gcpfxn( res1, res2 );
			gcpfxn.set_crossover( (crossover == CP_CROSSOVER_3 || crossover == CP_CROSSOVER_3FULL) ? 3 : 4 );
			invoker.invoke( gcpfxn );
		}
			break;
		}
	} else if ( crossover == CP_CROSSOVER_34 && ( res1.polymeric_sequence_distance( res2 ) == 2 ) ) {
		Size midway( res1.seqpos() > res2.seqpos() ? res2.seqpos() + 1 : res1.seqpos() + 1 );
		Size res1connat, res2connat;
		// scope for res1connat, res2connat initializations
		try {
			debug_assert( res1.connections_to_residue( midway ).size() == 1 );
			debug_assert( res2.connections_to_residue( midway ).size() == 1 );

			res1connat = res1.residue_connection( res1.connections_to_residue( midway )[ 1 ] ).atomno();
			res2connat = res2.residue_connection( res2.connections_to_residue( midway )[ 1 ] ).atomno();
		} catch ( utility::excn::Exception const & e ) {
			// At Steven's behest I removed this tracer
			//TR.Error << "CP_CROSSOVER_34 logic; accessing connections of " << res1.seqpos() << " to " << res2.seqpos() << " or vice versa is problematic " << std::endl;
			throw e;
		}
		CountPair2B< CountPairCrossover34 > cpfxn( res1, res1connat, res2, res2connat );
		invoker.invoke( cpfxn );
	} else {
		CountPairAll cpfxn;
		invoker.invoke( cpfxn );
	}
}

CPResidueConnectionType
CountPairFactory::determine_residue_connection(
	conformation::Residue const & res1,
	conformation::Residue const & res2
)
{
	if ( res1.is_pseudo_bonded( res2.seqpos() ) ) {
		return CP_MULTIPLE_BONDS_OR_PSEUDOBONDS;
	} else if ( res1.is_bonded(res2) ) {
		// We can run into some odd cases when bonding isn't symmetrical
		runtime_assert( res2.is_bonded(res1) );
		if ( res1.connections_to_residue( res2 ).size() == 1 ) {
			return CP_ONE_BOND;
		} else {
			return CP_MULTIPLE_BONDS_OR_PSEUDOBONDS;
		}
	}

	return CP_NO_BONDS;
}


CountPairFunctionOP
CountPairFactory::create_intrares_count_pair_function(
	conformation::Residue const & res,
	CPCrossoverBehavior crossover
)
{
	CountPairFunctionOP cpfxn;

	switch ( crossover ) {
	case CP_CROSSOVER_3FULL :
		cpfxn = utility::pointer::make_shared< CountPairIntraRes< CountPairCrossover3full > >( res );
		break;
	case CP_CROSSOVER_3 :
		cpfxn = utility::pointer::make_shared< CountPairIntraRes< CountPairCrossover3 > >( res );
		break;
	case CP_CROSSOVER_4 :
		cpfxn = utility::pointer::make_shared< CountPairIntraRes< CountPairCrossover4 > >( res );
		break;
	case CP_CROSSOVER_34 :
		cpfxn = utility::pointer::make_shared< CountPairIntraRes< CountPairCrossover34 > >( res );
		break;
	}

	return cpfxn;

}

} // namespace count_pair
} // namespace etable
} // namespace scoring
} // namespace core


