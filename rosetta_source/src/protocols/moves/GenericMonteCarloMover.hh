// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file src/protocols/moves/GenericMonteCarloMover.hh
/// @brief perform a given mover and sample structures by MonteCarlo
/// @detailed The "score" evaluation of pose during MC after applying mover is done by
/// ither FilterOP that can do report_sm() or ScoreFunctionOP you gave.
/// By setting sample_type_ to high, you can also sample the pose that have higher score.
/// @author Nobuyasu Koga ( nobuyasu@uw.edu )
/// @author Christopher Miles (cmiles@uw.edu)

#ifndef INCLUDED_protocols_moves_GenericMonteCarloMover_hh
#define INCLUDED_protocols_moves_GenericMonteCarloMover_hh

// Unit header
#include <protocols/moves/GenericMonteCarloMover.fwd.hh>

// C/C++ headers
#include <string>

// External headers
#include <boost/function.hpp>
#include <boost/unordered_map.hpp>

// Utility headers
#include <utility/vector1.hh>
#include <utility/tag/Tag.fwd.hh>

// Project Headers
#include <core/types.hh>
#include <core/pose/Pose.fwd.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/pack/task/PackerTask.hh>
#include <core/pack/task/TaskFactory.hh>
#include <protocols/filters/Filter.hh>

// Package headers
#include <protocols/moves/DataMap.fwd.hh>
#include <protocols/moves/MonteCarloStatus.hh>
#include <protocols/moves/Mover.hh>

namespace protocols {
namespace moves {

/// @brief Trigger API definition
typedef boost::function<bool(core::Size,
                             core::Size,
                             const core::pose::Pose&,
                             core::scoring::ScoreFunctionOP)> GenericMonteCarloMoverTrigger;


class GenericMonteCarloMover : public protocols::moves::Mover {
public:
	typedef protocols::moves::Mover Super;
	typedef std::string String;
	typedef core::Size Size;
	typedef core::Real Real;
	typedef core::pose::Pose Pose;
	typedef core::pose::PoseOP PoseOP;
	typedef core::scoring::ScoreFunction ScoreFunction;
	typedef core::scoring::ScoreFunctionOP ScoreFunctionOP;
	typedef protocols::filters::FilterOP FilterOP;
	typedef protocols::moves::MoverOP MoverOP;
	typedef core::pack::task::PackerTask PackerTask;
	typedef core::pack::task::PackerTaskOP PackerTaskOP;
	typedef core::pack::task::PackerTaskCOP PackerTaskCOP;
	typedef core::pack::task::TaskFactoryOP TaskFactoryOP;

	typedef utility::tag::TagPtr TagPtr;
	typedef protocols::moves::DataMap DataMap;
	typedef protocols::filters::Filters_map Filters_map;
	typedef protocols::moves::Movers_map Movers_map;

	/// @brief default constructor
	GenericMonteCarloMover();

	/// @brief value constructor without score function
	GenericMonteCarloMover(
		Size const maxtrials,
		MoverOP const & mover,
		Real const temperature = 0.0,
		String const sample_type = "low",
		bool const drift = true
	);

	/// @brief value constructor with score function
	// Undefined, commenting out to fix PyRosetta build
	/* GenericMonteCarloMover(
		Size const maxtrials,
		MoverOP const & mover,
		ScoreFunctionOP const & sfxn,
		Real const temperature = 0.0,
		String const sample_type = "low",
		bool const drift = true
	); */


	/// @brief value constructor with task operation via TaskFactory
	GenericMonteCarloMover(
		Size const maxtrials,
		MoverOP const & mover,
		TaskFactoryOP factory_in,
		Real const temperature = 0.0,
		String const sample_type = "low",
		bool const drift = true
	);


	/// @brief destructor
	~GenericMonteCarloMover();

	/// @brief create copy constructor
	virtual MoverOP clone() const;

	/// @brief create this type of objectt
	virtual MoverOP fresh_instance() const;

	/// @brief initialize object used in constructor
	void initialize();

	/// @brief apply GenericMonteCarloMover (Mover)
	virtual void apply( Pose & pose );

	virtual String get_name() const;

	/// @brief reset MC iterations, with pose used for the last & best structures
	void reset( Pose & pose );

	/// @brief return the simulation state to the lowest energy structure we've seen
	void recover_low( Pose & pose );

	/// @brief core of MC
	bool boltzmann( Pose & pose );

	Size num_designable( Pose & pose, PackerTaskOP & task);

 public: // accessor

	/// @brief return the last accepted pose
	PoseOP last_accepted_pose() const;

	/// @brief return the last accepted score
	Real last_accepted_score() const;

	/// @brief return the lowest score pose
	PoseOP lowest_score_pose() const;

	/// @brief return the lowest score
	Real lowest_score() const;

	/// @brief return the lowest score
	Real current_score() const;

	/// @brief return mc_accepted
	MCA mc_accpeted() const;

	/// @brief Return the score function in use
	ScoreFunctionOP score_function() const {
	 return scorefxn_;
	}

	/// @brief Adds a new trigger, returning its id.
	///
	/// Example:
	/// #include <boost/bind.hpp>
	/// #include <boost/function.hpp>
	///
	/// bool no_op(core::Size stage,
	///            core::Size num_stages,
	///            core::Size cycle,
	///            core::Size num_cycles,
	///            const core::pose::Pose&,
	///            core::scoring::ScoreFunctionOP) {}
	///
	/// Trigger callback = boost::bind(&no_op, STAGE, NUM_STAGES, _1, _2, _3, _4);
	/// Size trigger_id = add_trigger(callback);
	///
	/// The current stage and number of stages must be bound at creation time.
	/// This information provides the triggers with context about the current
	/// progress of the simulation as a whole.
	///
	/// If the trigger returns true, rescoring occurs.
	Size add_trigger(const GenericMonteCarloMoverTrigger& trigger);

	/// @brief Returns the number of triggers
	Size num_triggers() const;

	public: // mutators

	/// @brief Removes the trigger with the specified id
	void remove_trigger(Size trigger_id);

	/// @brief set max trials of MC trials
	void set_maxtrials( Size const ntrial );

	/// @brief set mover
	void set_mover( MoverOP const & mover );

	/// @brief Pose is evaluated by ScoreFunctionOP during MC trials
	void set_scorefxn( ScoreFunctionOP const & sfxn );

	/// @brief set temperature
	void set_temperature( Real const temp );

	/// @brief set sample type, max or min
	/// when sample_type == max, sample pose which have higher score
	/// when sample_type == min, sample pose which have lower score
	void set_sampletype( String const & type );

	/// @brief if drift=false, the pose is set back to the initial pose at each MC trial
	/// Of course, this is not MC sampling.
	void set_drift( bool const drift );

	/// @brief if preapply=true, auto-accept the first application of the submover,
	/// ignoring boltzman criteria.
	void set_preapply( bool const preapply=false );

	/// @brief if recover_low=true, after apply() the structure
	/// is the lowest energy structure, rather than the last accepted structure.
	void set_recover_low( bool const recover_low );

	/// @brief if boltz_rank=true, rank structures by the temperature-weighted
	/// sum of scores, rather than a single filter
	/// @detailed The score used here is the effective combined energy function
	/// that the Monte Carlo sampler is sampling over.
	void set_boltz_rank( bool const boltz_rank );

	/// @brief show scores of last_accepted_score and "best_score" ( = flip_sign_ * lowest_score )
	void show_scores( std::ostream & out ) const;

	/// @brief show counters of ntrial and acceptance ratio
	void show_counters( std::ostream & out ) const;

	virtual void parse_my_tag(
		TagPtr const tag,
		DataMap & data,
		Filters_map const & filters,
		Movers_map const & movers,
		Pose const &
	);

	///@brief parse "task_operations" XML option (can be employed virtually by derived Packing movers)
	virtual void parse_task_operations(
		TagPtr const,
		DataMap const &,
		Filters_map const &,
		Movers_map const &
);

	void add_filter( FilterOP filter, bool const adaptive, Real const temp, String const sample_type, bool rank_by=false);

	void stopping_condition( protocols::filters::FilterOP filter );
	protocols::filters::FilterOP stopping_condition() const;
private:
	/// @brief evalute pose by ScoreFunctionOP or FilterOP
	Real scoring( Pose & pose );

	/// @brief Executes all triggers. The order of trigger execution is undefined.
	/// Do not assume, depend, or in any way rely on a particular ordering.
	void fire_all_triggers(
		Size cycle,
		Size num_cycles,
		const Pose& pose,
		ScoreFunctionOP scoring);

	/// @brief max number of MC trials
	Size maxtrials_;

	/// @brief number of designable positions
	Size number_designable_;

	/// @brief mover
	MoverOP mover_;

	/// @brief task
	PackerTaskOP task_;

	/// @brief task factory
	TaskFactoryOP factory_;

	/// @brief Pose is evaluated by FilterOP which can do report_sm() during MC trials
	utility::vector1< FilterOP > filters_;
	utility::vector1< bool > adaptive_; // deflt true; are the filters adaptive or constant

	/// @brief acceptance criterion temperature
	utility::vector1< Real > temperatures_; // temperature per filter.
	utility::vector1< String > sample_types_; // low/high, dflt low
	utility::vector1< Real > last_accepted_scores_;

	/// @brief Count the number of rejections each filter resulted in.
	utility::vector1< core::Size > num_rejections_;

	/// @brief Pose is evaluated by ScoreFunctionOP during MC trials
	ScoreFunctionOP scorefxn_;

	/// @brief setter
	void task_factory( core::pack::task::TaskFactoryOP tf );

	/// @brief acceptance criterion temperature
	Real temperature_;// temperature for non-filters

	/// @brief set sample type, max or min
	/// when sample_type == max, sample pose which have higher score
	/// when sample_type == min, sample pose which have lower score
	String sample_type_;

	/// @brief if drift=false, the pose is set back to the initial pose at each MC trial
	/// Of course, this is not MC sampling.
	bool drift_;

	/// @brief Should we apply (and accept) the first application of the mover
	/// regardless of boltzman criteria? (Defaults true for historical reasons.)
	bool preapply_;

	/// @brief At the end of application, is the structure
	/// the last accepted structure (recover_low_==false)
	/// or is it the lowest energy structure (recover_low_=true)
	bool recover_low_;

	/// @brief By which filter (by index) are poses ranked by for non-Boltzman purposes
	/// (e.g. recover_low())
	Size rank_by_filter_;

	/// @brief If boltz_rank_=true, instead of ranking by a single filter, rank by the
	/// temperature-weighted sum of all scores
	bool boltz_rank_;

	/// @brief current score
	Real current_score_;

	/// @brief accepted structure
	Real last_accepted_score_;

	/// @brief lowest energy structure we've seen
	Real lowest_score_;

	/// @brief accepted structure
	PoseOP last_accepted_pose_;

	/// @brief lowest energy structure we've seen
	PoseOP lowest_score_pose_;

	/// @brief result of the last call to boltzmann
	MCA mc_accepted_;

	/// @brief to change the sing of calculated "score"
	Real flip_sign_;

	/// @brief diagnostics
	int trial_counter_;
	int accept_counter_;
	Real energy_gap_counter_;

	/// @brief Next trigger identifier to be assigned
	Size next_trigger_id_;

	/// @brief Collection of function callbacks
	boost::unordered_map<Size, GenericMonteCarloMoverTrigger> triggers_;
	protocols::filters::FilterOP stopping_condition_; //dflt false_filter; use this to stop an MC trajectory before maxtrials_ (if filter evaluates to true)
};

} // namespace moves
} // namespace protocols

#endif

