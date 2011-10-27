// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 sw=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/filters/RotamerBoltzmannWeightFilter.hh
/// @brief Reports to Tracer which residues are designable in a taskfactory
/// @author Sarel Fleishman (sarelf@uw.edu)

#ifndef INCLUDED_protocols_protein_interface_design_filters_RotamerBoltzmannWeight_hh
#define INCLUDED_protocols_protein_interface_design_filters_RotamerBoltzmannWeight_hh


// Project Headers
#include <protocols/filters/Filter.hh>
#include <core/pose/Pose.fwd.hh>
#include <utility/tag/Tag.fwd.hh>
#include <protocols/moves/DataMap.fwd.hh>
#include <protocols/moves/Mover.fwd.hh>
#include <protocols/protein_interface_design/filters/RotamerBoltzmannWeight.fwd.hh>
#include <core/scoring/ScoreFunction.fwd.hh>
#include <core/pack/task/TaskFactory.fwd.hh>
#include <core/chemical/AA.hh>

//Auto Headers
// Auto-header: duplicate removed #include <core/pack/task/TaskFactory.fwd.hh>

// Unit headers

namespace protocols {
namespace protein_interface_design{
namespace filters {

class RotamerBoltzmannWeight : public protocols::filters::Filter
{
private:
	typedef protocols::filters::Filter parent;
public:
	/// @brief default ctor
	RotamerBoltzmannWeight();
	///@brief Constructor with a single target residue
	virtual bool apply( core::pose::Pose const & pose ) const;
	virtual void report( std::ostream & out, core::pose::Pose const & pose ) const;
	virtual core::Real report_sm( core::pose::Pose const & pose ) const;
	virtual protocols::filters::FilterOP clone() const;
	virtual protocols::filters::FilterOP fresh_instance() const;
	core::Real compute( core::pose::Pose const & pose ) const;
	virtual ~RotamerBoltzmannWeight();
	core::pack::task::TaskFactoryOP task_factory() const;
	void task_factory( core::pack::task::TaskFactoryOP task_factory );
	void parse_my_tag( utility::tag::TagPtr const tag,
		protocols::moves::DataMap &,
		protocols::filters::Filters_map const &,
		protocols::moves::Movers_map const &,
		core::pose::Pose const & );

	utility::vector1< core::Size > first_pass_ala_scan( core::pose::Pose const & pose ) const; // return a list of residues that pass the ddG threshold
	core::Real compute_Boltzmann_weight( core::pose::Pose const & pose, core::Size const resi ) const;
	void rb_jump( core::Size const jump );
	core::Size rb_jump() const;
	void repacking_radius( core::Real const rad );
	core::Real repacking_radius() const;
	core::Real ddG_threshold() const;
	void ddG_threshold( core::Real const ddG );
	void scorefxn( core::scoring::ScoreFunctionOP scorefxn );
	core::scoring::ScoreFunctionOP scorefxn() const;
	core::Real temperature() const;
	void temperature( core::Real const temp );
	bool unbound() const;
	void unbound( bool const u );
	void threshold_probability( core::chemical::AA const aa_type, core::Real const probability );
	core::Real threshold_probability( core::chemical::AA const aa_type ) const;
	void energy_reduction_factor( core::Real const factor );
	core::Real energy_reduction_factor() const;
	core::Real interface_interaction_energy( core::pose::Pose const & pose, core::Size const res ) const;
	bool compute_entropy_reduction() const;
	void compute_entropy_reduction( bool const cer );
	void repack( bool const repack );
  bool repack() const;
	bool skip_ala_scan() const;
	void skip_ala_scan( bool const s );
	std::string type() const;
	void type( std::string const & s );
	core::Real compute_modified_ddG( core::pose::Pose const & pose, std::ostream & out ) const;
private:
	core::pack::task::TaskFactoryOP task_factory_;
	core::Size rb_jump_; // dflt 1.
	bool unbound_; // dflt true. what is the reference state for computing the boltz weight
	core::scoring::ScoreFunctionOP scorefxn_;
	core::Real temperature_; //dflt 0.8 ; this is actually kT rather than just T
	core::Real ddG_threshold_; //dflt 1.5 a preliminary alanine scan will identify all allowed positions that also have at least ddG_threshold_ effect on binding for further analysis.
	core::Real repacking_radius_; // dflt 6.0. how much to repack with each rotamer
	utility::vector1< core::Real > threshold_probabilities_per_residue_; // dflt 0.1; the rotamer probability below which the energy_reduction_factor kicks in
	core::Real energy_reduction_factor_;//dflt 0.5; by how much to decrease the binding energy

	bool compute_entropy_reduction_; //dflt false; compute the difference between the bound and unbound states
	bool repack_; //dflt true; carry out ddG (true) or dG (false) calculations
/// the following are mutable b/c they only sum up data in the filter. They do not affect
/// how the filter is run (so the filter will remain logically constant, despite these variables)
/// Mutability here allows the variables to be changed even in const methods.
	mutable std::map< core::Size, core::Real > ddGs_;//save the ddG values for a final report
	mutable std::map< core::Size, core::Real > rotamer_probabilities_;//ditto for the probabilities
	std::string type_;
	bool skip_ala_scan_;//dflt false; if true, only considers the task_factory
	bool fast_calc_; //default false for now
};

} // filters
} //protein_interface_design
} // protocols

#endif //INCLUDED_protocols_Filters_RotamerBoltzmannWeightFilter_HH_

