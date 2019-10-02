// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   protocols/match/downstream/DownstreamAlgorithm.hh
/// @brief  Declaration for the base-class algorithm that creates hits
///         from an upstream builder, while managing the logic of their
///         creation.  Furthermore, the downstream algorithm manages update
///         of hits from previous rounds should they loose their potential
///         to result in matches.
/// @author Alex Zanghellini (zanghell@u.washington.edu)
/// @author Andrew Leaver-Fay (aleaverfay@gmail.com), porting to mini

#ifndef INCLUDED_protocols_match_downstream_DownstreamAlgorithm_hh
#define INCLUDED_protocols_match_downstream_DownstreamAlgorithm_hh

// Unit headers
#include <protocols/match/downstream/DownstreamAlgorithm.fwd.hh>

// Package headers
#include <protocols/match/BumpGrid.fwd.hh>
#include <protocols/match/Matcher.fwd.hh>

#include <protocols/match/downstream/ActiveSiteGrid.fwd.hh>
#include <protocols/match/downstream/DownstreamBuilder.fwd.hh>

// Project headers
#include <core/types.hh>
#include <core/conformation/Residue.fwd.hh>

// Utility headers
#include <utility/pointer/ReferenceCount.hh>

// C++ headers
#include <list>

#include <core/id/AtomID.fwd.hh>
#include <protocols/match/Hit.fwd.hh>

namespace protocols {
namespace match {
namespace downstream {

/// @brief A class for an algorithm.  Given a conformation of the upstream partner,
/// the algorithm is responsible for producing a set of hits.
class DownstreamAlgorithm : public utility::pointer::ReferenceCount
{
public:
	typedef core::Size   Size;
	typedef core::Vector Vector;

public:
	DownstreamAlgorithm( Size geom_cst_id );
	DownstreamAlgorithm( DownstreamAlgorithm const & );
	DownstreamAlgorithm const & operator = ( DownstreamAlgorithm const & );

	~DownstreamAlgorithm() override;

	virtual
	DownstreamAlgorithmOP
	clone() const = 0;

	/// @brief Main driver function for hit generation.  The algorithm
	/// is responsible for generating hits at all scaffold build points that
	/// are valid for this geometric constraint.  The base class provides an
	/// iterate-across-all-positions-and-splice-together-hit-lists implementation,
	/// however, derived classes may overload this function.  The base class
	/// function is parallelizable with OpenMP. The returned hit list must be in sorted
	/// order by 1) hit.scaffold_build_id() and then by 2) hit.upstream_conf_id().
	virtual
	std::list< Hit >
	build_hits_at_all_positions(
		Matcher & matcher
	);


	/// @brief Reset appropriate Matcher data to spawn the deletion of hits from other rounds
	/// following either hit generation by this geometric constraint, or
	/// following hit generation by another geometric constraint which caused the deletion
	/// of hits from this geometric constraint.  The classic match algorithm, for example,
	/// resets the occupied-space hash so that other classic-match algorithms can delete their
	/// non-viable hits in subsequent calls to respond_to_peripheral_hitlist_change.
	virtual
	void
	respond_to_primary_hitlist_change( Matcher & matcher, Size round_just_completed );

	/// @brief Following the change in the number of hits of some other round -- either
	/// from the conclusion of that round in which a new set of hits has been generated,
	/// or from the cascading change to the hits from round A which were deleted after the
	/// conclusion of round B.
	virtual
	void
	respond_to_peripheral_hitlist_change( Matcher & matcher );


	/// @brief Return a set of hits given a conformation of an upstream residue.
	/// This method must be bit-wise constant and parallelizable in derived classes.
	virtual
	std::list< Hit >
	build(
		Size const scaffold_build_point_id,
		Size const upstream_conf_id,
		core::conformation::Residue const & upstream_residue
	) const = 0;

	/// @brief This method returns 'false' if the hits generated by this
	/// DownstreamAlgorithm store the 6-dimensional coordinate
	/// of the downstream partner in hit.second(), and therefore
	/// intend for the Matcher to find matches for this algorithm's hits by
	/// hashing the 6-dimensional coordinate.  This method returns 'true'
	/// if the DownstreamAlgorithm does not use hit.second to store a
	/// point in 6D and instead intends the Matcher to find matches
	/// by querying this DownstreamAlgorithm's hits_to_include_with_partial_match
	/// method.
	virtual
	bool
	upstream_only() const = 0;

	/// @brief This method returns 'true' if the Real6 portion of the returned
	/// hits are "original" descriptions of the downstream coordinates,
	/// and not merely duplicates of the downstream coordinates from previous
	/// rounds.  This method returns 'false' if the downstream portion
	/// of the returned hits are duplications of previous-round geometries or
	/// if the hits returned by this class do not describe the geometry of
	/// the downstream coordinates
	virtual
	bool
	generates_primary_hits() const = 0;

	/// @brief Called at the conclusion of matching, the Matcher signals
	/// to the downstream algorithm that it's time to prepare for
	/// match generation; if the downstream algorithm needs to enumerate
	/// compatible hits in response to the invokation of its
	/// hits_to_include_with_partial_match method, then now is the time
	/// to prepare for those calls.  Base class has a noop implementation.
	virtual
	void
	prepare_for_match_enumeration( Matcher const & );

	/// @brief This method is invoked by the Matcher as it enumerates matches should
	/// this class return "true" in it's upstream_only.
	/// method.  The Matcher will enumerate matches placing hits into the match
	/// for all of the geometric-constraints which describe the downstream partner's
	/// 6D coordinates in their hits, and then for all other geometric constraints
	/// in ascending order.  The hits for upstream residues 1 to geom_cst_id() - 1
	/// are all valid; the hit for geom-cst i, given that i > geom_cst_id() - 1 is
	/// valid if and only if the downstream algorithm for geom-cst i returns true
	/// in it's upstream_only().
	/// The Hit const * list should point at the Matcher's hits.  Between prepare-for-match
	/// enumeration and note_match_enumeration_completed, the Matcher provides
	/// a guarantee to the downstream algorithm that it's hit lists have not changed,
	/// and therefore all Hit *'s remain valid.
	/// The Matcher is providing a "match_dspos1 const &" instead of a "match const &"
	/// because this DownstreamAlgorithm should not be examining the 6D coordinates of
	/// the downstream partner according to other geometric constraints, and because
	/// the Matcher's match_dspos1-enumeration technique could not readily construct
	/// a match, the Matcher's match-enumeration technique can readily construct a
	/// match_dspos1.  Data in the input match_dspos1 relating to the downstream
	/// partner is undefined.
	virtual
	HitPtrListCOP
	hits_to_include_with_partial_match( match_dspos1 const & m ) const = 0;

	void
	set_bb_grid( BumpGridCOP bbgrid );

	void
	set_active_site_grid(
		ActiveSiteGridCOP active_site_grid
	);

	virtual
	Size
	n_possible_hits_per_upstream_conformation() const = 0;

	Size
	geom_cst_id() const;

	void
	set_dsbuilder(
		DownstreamBuilderOP dsbuilder
	);

	DownstreamBuilderOP
	get_dsbuilder() const;

	bool
	are_colliding(
		core::conformation::Residue const & us_res /*upstream*/,
		core::conformation::Residue const & ds_res /*downstream*/,
		utility::vector1< core::id::AtomID > const & ds_atoms,
		utility::vector1< core::Size > const & catalytic_atoms
	) const;

	//  virtual
	//  void
	//  initialize_upstream_residue(
	//    core::conformation::Residue const& us_res /*upstream residue*/
	//  );

protected:

	BumpGrid const &
	bbgrid() const {
		return *bbgrid_;
	}

	bool
	active_site_grid_set() const {
		return active_site_grid_ != 0;
	}

	ActiveSiteGrid const &
	active_site_grid() const {
		return *active_site_grid_;
	}

	/// @brief Non-virtual, const method for generating hits by first iterating across all build points
	/// and then invoking upstream_builder->build( build_point_i ) and splicing together the results.
	/// This method is invoked by default if the derived class does not override build_hits_at_all_positions.
	std::list< Hit >
	default_build_hits_at_all_positions(
		Matcher const & matcher
	) const;


private:
	Size geom_cst_id_; // which geometric constraint is this a downstream-algorithm for?
	BumpGridCOP bbgrid_;
	ActiveSiteGridCOP active_site_grid_;
	DownstreamBuilderOP dsbuilder_;

};

}
}
}

#endif
