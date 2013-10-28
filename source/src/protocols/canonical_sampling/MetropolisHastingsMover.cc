// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/canonical_sampling/MetropolisHastingsMover.cc
/// @brief MetropolisHastingsMover methods implemented
/// @author


// Unit Headers
#include <protocols/canonical_sampling/MetropolisHastingsMover.hh>
#include <protocols/canonical_sampling/MetropolisHastingsMoverCreator.hh>


// protocols headers
#include <protocols/backrub/BackrubMover.hh>
// AUTO-REMOVED #include <basic/datacache/DataMap.hh>
#include <protocols/jd2/JobDistributor.hh>
#include <protocols/jd2/util.hh>
#include <protocols/moves/MonteCarlo.hh>
#include <protocols/moves/Mover.hh>
#include <protocols/moves/MoverFactory.hh>
#include <protocols/simple_moves/sidechain_moves/SidechainMover.hh>
#include <protocols/simple_moves/sidechain_moves/SidechainMCMover.hh>
#include <protocols/simple_moves/BackboneMover.hh>
#include <protocols/canonical_sampling/ThermodynamicMover.hh>
#include <protocols/canonical_sampling/ThermodynamicObserver.hh>
#include <protocols/rosetta_scripts/util.hh>

// #include <core/scoring/constraints/ConstraintIO.hh>
// #include <core/scoring/constraints/ConstraintSet.hh>
// #include <core/scoring/constraints/ConstraintSet.fwd.hh>

// core headers
#include <core/kinematics/MoveMap.hh>
// AUTO-REMOVED #include <basic/options/option_macros.hh>
#include <basic/options/keys/in.OptionKeys.gen.hh>
#include <basic/options/keys/packing.OptionKeys.gen.hh>
#include <core/pack/task/TaskFactory.hh>
#include <core/pack/task/operation/TaskOperations.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/types.hh>
#include <basic/Tracer.hh>
// AUTO-REMOVED #include <core/conformation/Residue.hh> // REQUIRED FOR WINDOWS

// numeric headers
#include <numeric/random/random.hh>

// utility headers
#include <utility/file/file_sys_util.hh>
#include <utility/io/ozstream.hh>
#include <utility/pointer/owning_ptr.hh>
#include <utility/tag/Tag.hh>

#include <core/chemical/ResidueType.hh>
#include <utility/vector0.hh>
#include <utility/vector1.hh>

//Auto Headers
#include <utility/excn/Exceptions.hh>
#include <basic/options/option.hh>

// C++ Headers

using basic::T;
using basic::Error;
using basic::Warning;

static basic::Tracer TR( "protocols.canonical_sampling.MetropolisHastingsMover" );
static numeric::random::RandomGenerator RG(638767547);

namespace protocols {
namespace canonical_sampling {


std::string
MetropolisHastingsMoverCreator::keyname() const {
	return MetropolisHastingsMoverCreator::mover_name();
}

protocols::moves::MoverOP
MetropolisHastingsMoverCreator::create_mover() const {
	return new MetropolisHastingsMover;
}

std::string
MetropolisHastingsMoverCreator::mover_name() {
	return "MetropolisHastings";
}

MetropolisHastingsMover::MetropolisHastingsMover() :
	monte_carlo_(0),
	ntrials_(1000),
	trial_(0)
{}

MetropolisHastingsMover::MetropolisHastingsMover(
	MetropolisHastingsMover const & metropolis_hastings_mover
) :
	//utility::pointer::ReferenceCount(),
	Mover(metropolis_hastings_mover),
	monte_carlo_( new protocols::moves::MonteCarlo(*metropolis_hastings_mover.monte_carlo_) ),
	ntrials_(metropolis_hastings_mover.ntrials_),
	output_name_(metropolis_hastings_mover.output_name_),
	weighted_sampler_(metropolis_hastings_mover.weighted_sampler_),
	trial_(metropolis_hastings_mover.trial_)
{
	for (core::Size i = 1; i <= metropolis_hastings_mover.movers_.size(); ++i) {
		movers_.push_back(reinterpret_cast<ThermodynamicMover *>(metropolis_hastings_mover.movers_[i]()));
	}

	for (core::Size i = 1; i <= metropolis_hastings_mover.observers_.size(); ++i) {
		observers_.push_back(reinterpret_cast<ThermodynamicObserver *>(metropolis_hastings_mover.observers_[i]()));
	}

	if (metropolis_hastings_mover.tempering_) {
		tempering_ = reinterpret_cast<TemperatureController *>(metropolis_hastings_mover.tempering_());
		if (monte_carlo_) tempering_->set_monte_carlo(monte_carlo_);
	}
}

MetropolisHastingsMover::~MetropolisHastingsMover(){}

core::Size
MetropolisHastingsMover::prepare_simulation( core::pose::Pose & pose ) {
	if (output_name() == "") {
		set_output_name(protocols::jd2::JobDistributor::get_instance()->current_output_name());
		TR.Info  << " obtained output name from JobDistributor " << std::endl;
		output_name_from_job_distributor_ = true;
	} else {
		TR.Info << " running with preset output name: " << output_name() << std::endl;
	}

	if ( !tempering_ ) {
		//get this done before "initialize_simulation" is called no movers and observers
		TR.Info << "no temperature controller in MetropolisHastings defined... generating FixedTemperatureController" << std::endl;
		tempering_= new protocols::canonical_sampling::FixedTemperatureController( monte_carlo_->temperature() );
	}
	using namespace core;
	bool restart = false;
	core::Size cycle_number = 0;
	Size temp_level = 0;
	Real temperature = -1.0;
// 	for (core::Size i = 1; i <= observers_.size() && !restart; ++i) {
// 		TR << "Attempting restart using " << observers_[i]->get_name() << std::endl;
// 		restart = observers_[i]->restart_simulation(pose, *this, cycle_number, temp_level, temperature );
// 		if ( restart ) TR << "Restarted using " << observers_[i]->get_name() << std::endl;
// 	}

	if ( !restart ) {
		cycle_number = 0; //make sure this is zero if we don't have a restart.
		tempering_->initialize_simulation(pose, *this, cycle_number );
	} else {
		tempering_->initialize_simulation(pose, *this, temp_level, temperature, cycle_number );
	}

	for (core::Size i = 1; i <= movers_.size(); ++i) {
		TR << "Initializing " << movers_[i]->get_name() << std::endl;
		movers_[i]->set_metropolis_hastings_mover(this);
		movers_[i]->initialize_simulation(pose, *this, cycle_number);
	}

	runtime_assert( monte_carlo_ );
	monte_carlo_->reset(pose);
	monte_carlo_->reset_counters();

	for (core::Size i = 1; i <= observers_.size(); ++i) {
		TR << "Initializing " << observers_[i]->get_name() << std::endl;
		observers_[i]->initialize_simulation(pose, *this, cycle_number);
	}

	TR << "Initial Score:\n";
	monte_carlo_->score_function().show(TR, pose);

	TR << "Running " << ntrials_-cycle_number << " trials..." << std::endl;
	return cycle_number;
}

void
MetropolisHastingsMover::apply( core::pose::Pose& pose ) {
	output_name_from_job_distributor_ = false;

	Size start_cycle = prepare_simulation( pose );

	for (trial_ = start_cycle+1; trial_ <= ntrials_; ++trial_) {

		ThermodynamicMoverOP mover(random_mover());
		mover->apply(pose);
		bool accepted = monte_carlo_->boltzmann(
         pose,
				 mover->type(),
				 mover->last_proposal_density_ratio(),
				 mover->last_inner_score_temperature_delta()
		);
		set_last_move( mover );
		set_last_accepted( accepted );
		tempering_->temperature_move( pose, monte_carlo_->last_accepted_score() );
		mover->observe_after_metropolis(*this);
		TR.Debug << "current move accepted " << accepted <<std::endl;
		for (core::Size i = 1; i <= observers_.size(); ++i) {
			observers_[i]->observe_after_metropolis(*this);
		}
// 		core::scoring::constraints::ConstraintSetCOP cst_set = pose.constraint_set();
// 		if ( !cst_set->has_constraints() )
// 			utility_exit_with_message("cst_set is empty!");
// 		if ( trial_ % 500 == 0 ) {
// 			cst_set->show_violations(TR.Info, pose, 76, 1);
// 		}
	}

	wind_down_simulation( pose );
}

void
MetropolisHastingsMover::wind_down_simulation( core::pose::Pose& pose) {
	for (core::Size i = 1; i <= movers_.size(); ++i) {
		TR << "Finalizing " << movers_[i]->get_name() << std::endl;
		movers_[i]->finalize_simulation(pose, *this);
	}

	for (core::Size i = 1; i <= observers_.size(); ++i) {
		TR << "Finalizing " << observers_[i]->get_name() << std::endl;
		observers_[i]->finalize_simulation(pose, *this);
	}

	tempering_->finalize_simulation(pose, *this);

	TR << "Final Score:" << std::endl;
	monte_carlo_->score_function().show(TR, pose);
	TR.flush();

	monte_carlo_->show_counters();

	if (output_name_from_job_distributor_) set_output_name("");
}

std::string
MetropolisHastingsMover::get_name() const
{
	return "MetropolisHastingsMover";
}

protocols::moves::MoverOP
MetropolisHastingsMover::clone() const
{
	return new protocols::canonical_sampling::MetropolisHastingsMover(*this);
}

protocols::moves::MoverOP
MetropolisHastingsMover::fresh_instance() const
{
	return new MetropolisHastingsMover;
}

bool
MetropolisHastingsMover::reinitialize_for_each_job() const { return false; }

bool
MetropolisHastingsMover::reinitialize_for_new_input() const { return false; }

void
MetropolisHastingsMover::parse_my_tag(
	utility::tag::TagCOP const tag,
	basic::datacache::DataMap & data,
	protocols::filters::Filters_map const & filters,
	protocols::moves::Movers_map const & movers,
	core::pose::Pose const & pose
) {
	///ntrials
	ntrials_ = tag->getOption< core::Size >( "trials", ntrials_ );

	//monte-carlo
	//read tag scorefxn
	core::scoring::ScoreFunctionOP score_fxn(protocols::rosetta_scripts::parse_score_function(tag, data));
	core::Real const temperature(tag->getOption< core::Real >( "temperature", 0.6 ) );
	monte_carlo_ = new protocols::moves::MonteCarlo(*score_fxn, temperature);

	//add movers and observers
	utility::vector0< utility::tag::TagCOP > const subtags( tag->getTags() );
	for( utility::vector0< utility::tag::TagCOP >::const_iterator subtag_it = subtags.begin(); subtag_it != subtags.end(); ++subtag_it ) {
		TagCOP const subtag = *subtag_it;
		protocols::moves::MoverOP mover;
		if (subtag->getName() == "Add") { //add existing mover
			std::string mover_name = subtag->getOption<std::string>( "mover_name", "null" );
			protocols::moves::Movers_map::const_iterator mover_iter( movers.find( mover_name ) );
			if ( mover_iter == movers.end() ) {
				TR << "Mover not found for XML tag:\n" << subtag << std::endl;
				throw utility::excn::EXCN_RosettaScriptsOption("");
			}
			mover = mover_iter->second;
		} else { //generate new mover
			protocols::moves::MoverFactory * mover_factory(protocols::moves::MoverFactory::get_instance());
			mover = mover_factory->newMover(subtag, data, filters, movers, pose);
		}

		ThermodynamicMoverOP th_mover( dynamic_cast<ThermodynamicMover *>( mover() ) );
		ThermodynamicObserverOP th_observer( dynamic_cast<ThermodynamicObserver *>( mover() ) );
		TemperatureControllerOP temp_controller( dynamic_cast< TemperatureController* >( mover() ) );
		//figure out if ThermodynamicMover or ThermodynamicObserver
		if ( th_mover ) { //its a mover
			core::Real const weight( subtag->getOption< core::Real >( "sampling_weight", 1 ) );
			add_mover( th_mover, weight, subtag );
			// 			add_mover( th_mover, weight );
		} else if ( th_observer ) { //its an observer
			//it might also be a tempering module...
			if ( temp_controller ) { // it is a temperature controller
				if ( tempering_ ) {
					throw utility::excn::EXCN_RosettaScriptsOption( "cannot define two TemperatureControllers" );
				}
				set_tempering( temp_controller );
			} else {  //no just an plain old observer
				add_observer( th_observer );
			}
		} else { //its something different
			TR << "Mover is not a ThermodynamicMover or ThermodynamicObserver for XML tag:\n" << subtag << std::endl;
			throw utility::excn::EXCN_RosettaScriptsOption("");
		}
	}
}

protocols::moves::MonteCarloCOP
MetropolisHastingsMover::monte_carlo() const {
	return monte_carlo_;
}

protocols::moves::MonteCarlo& MetropolisHastingsMover::nonconst_monte_carlo() {
	return *monte_carlo_;
}

void
MetropolisHastingsMover::set_monte_carlo(
	protocols::moves::MonteCarloOP monte_carlo
)
{
	monte_carlo_ = monte_carlo;
	if ( tempering_ ) tempering_->set_monte_carlo( monte_carlo_ );
}

void
MetropolisHastingsMover::set_tempering(
	TemperatureControllerOP tempering
) {
	tempering_=tempering;
	if ( monte_carlo_ ) tempering_->set_monte_carlo( monte_carlo_ );
}

TemperatureControllerCOP
MetropolisHastingsMover::tempering() const {
	return tempering_;
}

void
MetropolisHastingsMover::set_ntrials(
	core::Size ntrials
) {
	ntrials_ = ntrials;
}

bool
MetropolisHastingsMover::finished() const {
	return trial_ > ntrials_;
}

std::string const &
MetropolisHastingsMover::output_name() const {
	return output_name_;
}

void
MetropolisHastingsMover::set_output_name(
	std::string const & output_name
) {
	output_name_ = output_name;
}

std::string
MetropolisHastingsMover::output_file_name(
	std::string const & suffix,
	bool cumulate_jobs, //= false
	bool cumulate_replicas //= false
) const {

	if (cumulate_jobs || cumulate_replicas) runtime_assert(utility::io::ozstream::MPI_reroute_rank() >= 0);

	std::ostringstream file_name_stream;

	if (!cumulate_jobs) {
		file_name_stream << output_name_;
	}

	core::Size const replica( protocols::jd2::current_replica() );
	if (!cumulate_replicas && replica) {
		if ( file_name_stream.str().length() ) file_name_stream << "_";
		file_name_stream << std::setfill('0') << std::setw(3) << replica;
	}

	if ( suffix.length() ) {
		if ( file_name_stream.str().length() ) file_name_stream << "_";
		file_name_stream << suffix;
	}

	return file_name_stream.str();
}

void
MetropolisHastingsMover::set_last_move(
  ThermodynamicMoverOP setting
) {
	last_move_ = setting;
}

ThermodynamicMover const&
MetropolisHastingsMover::last_move() const {
	assert( last_move_ );
	return *last_move_;
}

ThermodynamicMoverOP
MetropolisHastingsMover::random_mover()
{
	return movers_[weighted_sampler_.random_sample(RG)];
}

void
MetropolisHastingsMover::add_mover(
	ThermodynamicMoverOP mover,
	core::Real weight
)
{
	mover->set_preserve_detailed_balance(true);
	movers_.push_back(mover);
	weighted_sampler_.add_weight(weight);
}

void
MetropolisHastingsMover::add_mover(
        ThermodynamicMoverOP mover,
        core::Real weight,
        utility::tag::TagCOP const&
        )
{
        add_mover( mover, weight );
}

void
MetropolisHastingsMover::add_backrub_mover(
	core::Real weight
)
{
	if (!weight) return;

	protocols::backrub::BackrubMoverOP backrub_mover(new protocols::backrub::BackrubMover);
	backrub_mover->branchopt().read_database();

	add_mover(backrub_mover, weight);
}

void
MetropolisHastingsMover::add_small_mover(
	core::Real weight
)
{
	if (!weight) return;

	using namespace basic::options;
	using namespace basic::options::OptionKeys;

	protocols::simple_moves::SmallMoverOP small_mover(new protocols::simple_moves::SmallMover);
	small_mover->nmoves(1);
	core::kinematics::MoveMapOP movemap = new core::kinematics::MoveMap;
	if (utility::file::file_exists(option[ in::file::movemap ])) {
		movemap->init_from_file(option[ in::file::movemap ]);
	} else {
		movemap->set_bb(true);
	}
	small_mover->movemap(movemap);

	add_mover(small_mover, weight);
}

void
MetropolisHastingsMover::add_shear_mover(
	core::Real weight
)
{
	if (!weight) return;

	using namespace basic::options;
	using namespace basic::options::OptionKeys;

	protocols::simple_moves::ShearMoverOP shear_mover(new protocols::simple_moves::ShearMover);
	shear_mover->nmoves(1);
	core::kinematics::MoveMapOP movemap = new core::kinematics::MoveMap;
	if (utility::file::file_exists(option[ in::file::movemap ])) {
		movemap->init_from_file(option[ in::file::movemap ]);
	} else {
		movemap->set_bb(true);
	}
	shear_mover->movemap(movemap);

	add_mover(shear_mover, weight);
}

void
MetropolisHastingsMover::add_sidechain_mover(
	core::Real weight,
	core::Real prob_uniform,
	core::Real prob_withinrot,
	bool preserve_cbeta
)
{
	if (!weight) return;

	using namespace basic::options;
	using namespace basic::options::OptionKeys;

	core::pack::task::TaskFactoryOP main_task_factory = new core::pack::task::TaskFactory;
	main_task_factory->push_back( new core::pack::task::operation::InitializeFromCommandline );
	if ( option[ packing::resfile ].user() ) {
		main_task_factory->push_back( new core::pack::task::operation::ReadResfile );
	} else {
		main_task_factory->push_back( new core::pack::task::operation::RestrictToRepacking );
	}
	if (preserve_cbeta) main_task_factory->push_back( new core::pack::task::operation::PreserveCBeta );

	protocols::simple_moves::sidechain_moves::SidechainMoverOP sidechain_mover(new protocols::simple_moves::sidechain_moves::SidechainMover);
	sidechain_mover->set_task_factory(main_task_factory);
	sidechain_mover->set_prob_uniform(prob_uniform);
	sidechain_mover->set_prob_withinrot(prob_withinrot);

	add_mover(sidechain_mover, weight);
}

void
MetropolisHastingsMover::add_sidechain_mc_mover(
	core::Real weight,
	core::Real prob_uniform,
	core::Real prob_withinrot,
	bool preserve_cbeta,
	core::Size ntrials
)
{
	if (!weight) return;

	using namespace basic::options;
	using namespace basic::options::OptionKeys;

	core::pack::task::TaskFactoryOP main_task_factory = new core::pack::task::TaskFactory;
	main_task_factory->push_back( new core::pack::task::operation::InitializeFromCommandline );
	if ( option[ packing::resfile ].user() ) {
		main_task_factory->push_back( new core::pack::task::operation::ReadResfile );
	} else {
		main_task_factory->push_back( new core::pack::task::operation::RestrictToRepacking );
	}
	if (preserve_cbeta) main_task_factory->push_back( new core::pack::task::operation::PreserveCBeta );

	protocols::simple_moves::sidechain_moves::SidechainMCMoverOP sidechain_mc_mover(new protocols::simple_moves::sidechain_moves::SidechainMCMover);
	sidechain_mc_mover->set_task_factory(main_task_factory);
	sidechain_mc_mover->set_prob_uniform(prob_uniform);
	sidechain_mc_mover->set_prob_withinrot(prob_withinrot);
	sidechain_mc_mover->setup(monte_carlo_->score_function().clone());
	sidechain_mc_mover->set_ntrials(ntrials);

	add_mover(sidechain_mc_mover, weight);
}

void
MetropolisHastingsMover::add_observer(
	ThermodynamicObserverOP observer
)
{
	observers_.push_back(observer);
}

} //moves
} //protocols

