// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/simple_moves/sidechain_moves/SidechainMCMover.cc
/// @brief implementation of SidechainMCMover class and functions
/// @author Colin A. Smith (colin.smith@ucsf.edu)

#include <protocols/simple_moves/sidechain_moves/SidechainMover.hh>
#include <protocols/simple_moves/sidechain_moves/SidechainMCMover.hh>
#include <protocols/simple_moves/sidechain_moves/SidechainMCMoverCreator.hh>

#include <basic/prof.hh>

// Core Headers
#include <core/chemical/ResidueType.hh>
#include <core/conformation/Residue.hh>
// AUTO-REMOVED #include <core/conformation/ResidueFactory.hh>
#include <core/pack/task/TaskFactory.hh>
#include <core/pack/task/PackerTask.hh>
#include <core/pose/Pose.hh>
// AUTO-REMOVED #include <core/pack/dunbrack/DunbrackRotamer.hh>
// AUTO-REMOVED #include <core/pack/dunbrack/SingleResidueDunbrackLibrary.hh>
// AUTO-REMOVED #include <core/pack/dunbrack/RotamerLibraryScratchSpace.hh>
// AUTO-REMOVED #include <core/pack/task/operation/TaskOperations.hh>
// AUTO-REMOVED #include <core/scoring/ScoringManager.hh>
#include <core/types.hh>
#include <basic/Tracer.hh>
// AUTO-REMOVED #include <basic/basic.hh>
#include <core/pack/interaction_graph/SimpleInteractionGraph.hh>
#include <basic/datacache/DataMap.hh>
#include <protocols/canonical_sampling/MetropolisHastingsMover.hh>
#include <protocols/moves/MonteCarlo.hh>
#include <protocols/rosetta_scripts/util.hh>
#include <core/scoring/ScoreFunction.hh>
#include <utility/tag/Tag.hh>

// Numeric Headers
// AUTO-REMOVED #include <numeric/angle.functions.hh>
// AUTO-REMOVED #include <numeric/constants.hh>
// AUTO-REMOVED #include <numeric/conversions.hh>
#include <numeric/random/random.hh>

// AUTO-REMOVED #include <core/scoring/TenANeighborGraph.hh>
// AUTO-REMOVED #include <core/scoring/Energies.hh>


// C++ Headers
#include <ostream>
#include <sstream>
// AUTO-REMOVED #include <fstream>
// AUTO-REMOVED #include <utility/fixedsizearray1.hh>

#include <utility/string_util.hh>
#include <utility/vector0.hh>
#include <utility/vector1.hh>

//Auto Headers
#include <utility/excn/Exceptions.hh>
#include <core/pack/task/operation/TaskOperation.hh>

#ifdef WIN_PYROSETTA
	#include <protocols/canonical_sampling/ThermodynamicObserver.hh>
#endif


using namespace core;
using namespace core::pose;
using namespace basic;
using namespace protocols::moves;


static numeric::random::RandomGenerator Rg(38627227);
static basic::Tracer TR("protocols.simple_moves.sidechain_moves.SidechainMCMover");

namespace protocols {
namespace simple_moves {
namespace sidechain_moves {

std::string
SidechainMCMoverCreator::keyname() const {
	return SidechainMCMoverCreator::mover_name();
}

protocols::moves::MoverOP
SidechainMCMoverCreator::create_mover() const {
	return new SidechainMCMover;
}

std::string
SidechainMCMoverCreator::mover_name() {
	return "SidechainMC";
}

SidechainMCMover::SidechainMCMover():
	protocols::simple_moves::sidechain_moves::SidechainMover(),
	current_(),
	previous_(),
	best_(),
	temperature_(0),
	ntrials_(0),
	best_energy_(0),
	sfxn_(),
	inherit_scorefxn_temperature_(false),
	ig_(0),
	accepts_(0),
	current_ntrial_(0),
	score_pre_apply_(0),
	score_post_apply_(0),
	metropolis_hastings_mover_(0)
{}

SidechainMCMover::SidechainMCMover(
	core::pack::dunbrack::RotamerLibrary const & rotamer_library
):
	protocols::simple_moves::sidechain_moves::SidechainMover(rotamer_library),
	current_(),
	previous_(),
	best_(),
	temperature_(0),
	ntrials_(0),
	best_energy_(0),
	sfxn_(),
	inherit_scorefxn_temperature_(false),
	ig_(0),
	accepts_(0),
	current_ntrial_(0),
	score_pre_apply_(0),
	score_post_apply_(0),
	metropolis_hastings_mover_(0)
{}

SidechainMCMover::~SidechainMCMover() {}


void
SidechainMCMover::setup( core::scoring::ScoreFunctionCOP sfxn ){
	ig_ = new core::pack::interaction_graph::SimpleInteractionGraph(); //commented out debug
	//(*sfxn)(pose); //gets called in apply
	set_scorefunction( *sfxn );
	ig_->set_scorefunction( *sfxn );
	//ig_->initialize( pose ); //gets called in apply. we can't count on the graph being up-to-date between setup and apply
}

void
SidechainMCMover::show_counters( std::ostream & out){
	out << "SCMCMover: trials " << current_ntrial_ << " accepts= " << (accepts_/current_ntrial_) << std::endl;
}

protocols::moves::MoverOP
SidechainMCMover::clone() const {
  return( protocols::moves::MoverOP( new protocols::simple_moves::sidechain_moves::SidechainMCMover( *this ) ) );
}

protocols::moves::MoverOP
SidechainMCMover::fresh_instance() const {
	return (protocols::moves::MoverOP( new SidechainMCMover ));
}

/// @detailed
void
SidechainMCMover::apply(
	Pose & pose
)
{

	bool const DEBUG = false;

	using namespace core::scoring;
	using namespace protocols;
	using namespace protocols::moves;
	//using namespace protocols::fast_sc_mover;

	utility::vector1< core::Real > new_chi;
	core::Real current_energy = sfxn_(pose);
	score_pre_apply_ = current_energy;
	init_task(pose);

	current_.resize(pose.total_residue());
	previous_.resize(pose.total_residue());
	best_.resize(pose.total_residue());
	runtime_assert(temperature_ != 0);
	runtime_assert(ntrials_ != 0);
	runtime_assert(packed_residues().size() > 0);

	if (inherit_scorefxn_temperature_) {
		runtime_assert(metropolis_hastings_mover_);
		// update temperature every time in case temperature changes are implemented in the future
		set_temperature(metropolis_hastings_mover_->monte_carlo()->temperature());
	}

	 // for debugging
	pose::Pose temp(pose);
	pose::Pose dummy(pose);
	 //

	for(core::Size itr = 1; itr <= pose.total_residue(); itr++){
		current_[ itr ] = new core::conformation::Residue(pose.residue( itr ));
	}

	//	PROF_START( SIMPLEINTGRAPH );
	//SimpleInteractionGraphOP ig(new SimpleInteractionGraph()); //commented out debug
	//ig->set_scorefunction( sfxn_ ); //commented out debug
	ig_->initialize( pose ); //commented out debug
	SidechainMover::init_task( pose );
	//	PROF_STOP( SIMPLEINTGRAPH  );
	//runtime_assert(ig_ != 0);

	for( core::Size iter_i = 1; iter_i <= ntrials_; iter_i++){
		//pick randomly
		core::Size rand_res;
		do {
			//pick residue, respecting underlying packer task
			rand_res = packed_residues()[Rg.random_range(1, packed_residues().size())];
		}while ( pose.residue( rand_res ).name1() == 'P'); //SidechainMover cannot sample proline rotamers? (is this true?)

		core::conformation::ResidueOP new_state( new core::conformation::Residue( pose.residue( rand_res ) ) );

		/// APL Note: you have to remove output if you're trying to optimize.
		if ( TR.visible( basic::t_debug )) {
			TR.Debug << "old-chi-angles: ";
			for(unsigned int i = 1; i <= new_state->nchi(); i++){
				TR.Debug << new_state->chi( i ) << " ";
			}
			TR.Debug << std::endl;
		}
		//		if( !task_initialized() ){
		//			init_task( pose );
		//		}
		new_state = make_move( new_state );
		//new_state->update_actcoord();
		if ( TR.visible( basic::t_debug )) {
			TR.Debug << "new-chi-angles: ";
			for(unsigned int i = 1; i <= new_state->nchi(); i++){
				TR.Debug << new_state->chi( i ) << " ";
			}
			TR.Debug << std::endl;
		}


		PROF_START( SIMPLEINTGRAPH );
		core::Real delta_energy = ig_->consider_substitution( rand_res, new_state );
		PROF_STOP( SIMPLEINTGRAPH );

		if( DEBUG )
		{//debug
			core::Real s1=	sfxn_(temp);
			dummy = temp;
			//for(unsigned int i = 1; i <= new_state->nchi(); i++ ){
			//dummy.set_chi( i, rand_res, new_state->chi( i ) );
			//}
			dummy.replace_residue( rand_res, *new_state, true );
			core::Real s2 = sfxn_(dummy);

			if( (s1 - s2) - delta_energy > 0.05 ) {
				TR.Debug << "WARNING: ENERGIES DON'T MATCH UP! " << s1 << " " << s2 << " " << (s1 - s2) << " " << delta_energy << std::endl;
				dummy.dump_pdb("dummy.pdb");
				temp.dump_pdb("temp.pdb");
				//exit(1);

			}else{
				TR.Debug << "energies match up " << std::endl;
			}
		}//debug


		if( pass_metropolis( delta_energy, SidechainMover::last_proposal_density_ratio() ) ){ //ek
			if( DEBUG )
			{ //debug//
				core::Real s1=	sfxn_(temp);
				temp.replace_residue( rand_res, *new_state, true );
				core::Real s2 = sfxn_(dummy);
				TR.Debug << "current energy after accept: " << sfxn_(temp) << " delta " << (s2-s1) << std::endl;
				//for( core::Size itr_i = 1; itr_i <= new_state->nchi(); itr_i++ ){
				//temp.set_chi( itr_i, rand_res, new_state->chi(itr_i) );
				//}
			}

			if ( TR.visible( basic::t_debug )) {
				TR.Debug << "passed metropolis! assigning new move to current " << std::endl;
			}
			previous_[ rand_res ] = current_[ rand_res ] ;

			PROF_START( SIMPLEINTGRAPH );
			ig_->commit_change( rand_res );
			PROF_STOP( SIMPLEINTGRAPH );
			current_energy -= delta_energy;
			current_[ rand_res ] =   new_state;
			if( ( current_energy ) <  best_energy_ ){
				best_[ rand_res ] = new_state;
				best_energy_ = current_energy;
			}
		} else{ //rejected metropolis criterion
			PROF_START( SIMPLEINTGRAPH );
			ig_->reject_change( rand_res );
			PROF_STOP( SIMPLEINTGRAPH );
		}
	} // n_iterations


	for(core::Size res_i = 1; res_i <= current_.size(); res_i++ ){
		//for(core::Size chi_i = 1; chi_i <= current_[ res_i ]->nchi(); chi_i++){
			//pose.set_chi( chi_i, res_i, current_[ res_i ]->chi( chi_i ) );
		//}
		pose.replace_residue( res_i, (*current_[ res_i ]), true );
	}

	score_post_apply_ = current_energy;
	if (metropolis_hastings_mover_) {
		score_post_apply_ = sfxn_(pose);
		//TR << "Score Actual: " << score_post_apply_ << " Accumulated: " << current_energy << " Delta: " << current_energy - score_post_apply_ << std::endl;
	}

	type("sc_mc");
}

std::string
SidechainMCMover::get_name() const {
	return "SidechainMCMover";
}

bool
SidechainMCMover::pass_metropolis(core::Real delta_energy , core::Real last_proposal_density_ratio ){

	core::Real boltz_factor = delta_energy / temperature_;
	core::Real probability = std::exp( std::min( 40.0, std::max( -40.0, boltz_factor ))) *  last_proposal_density_ratio ;

	if( probability < 1 && Rg.uniform() >= probability ) {
		current_ntrial_++;
		if ( TR.visible( basic::t_debug )) {
			TR.Debug << "delta energy is " << delta_energy << " probability: " << probability << " accepted: FALSE " << std::endl;
		}
		return false;
		}
	else {
		accepts_++;
		current_ntrial_++;
		if ( TR.visible( basic::t_debug )) {
			TR.Debug << "delta energy is " << delta_energy << " probability: " << probability << " accepted: TRUE" << std::endl;
		}
		return true;
	}
}

void
SidechainMCMover::parse_my_tag( utility::tag::TagCOP const tag, basic::datacache::DataMap & data, protocols::filters::Filters_map const &, protocols::moves::Movers_map const &, core::pose::Pose const & pose) {

	// code duplication: should really call SidechainMover::parse_my_tag() instead of having most of the code below
	if ( tag->hasOption("task_operations") ) {

		core::pack::task::TaskFactoryOP new_task_factory( new core::pack::task::TaskFactory );

		std::string const t_o_val( tag->getOption<std::string>("task_operations") );
		typedef utility::vector1< std::string > StringVec;
		StringVec const t_o_keys( utility::string_split( t_o_val, ',' ) );
		for ( StringVec::const_iterator t_o_key( t_o_keys.begin() ), end( t_o_keys.end() );
					t_o_key != end; ++t_o_key ) {
			if ( data.has( "task_operations", *t_o_key ) ) {
				new_task_factory->push_back( data.get< core::pack::task::operation::TaskOperation* >( "task_operations", *t_o_key ) );
			} else {
				throw utility::excn::EXCN_RosettaScriptsOption("TaskOperation " + *t_o_key + " not found in basic::datacache::DataMap.");
			}
		}

		set_task_factory(new_task_factory);

	} else {

		pack::task::PackerTaskOP pt = core::pack::task::TaskFactory::create_packer_task( pose );
		set_task( pt );
		pt->restrict_to_repacking();
		// probably should call init_task(), but it's cleaner if we wait untill it's called in apply()
	}

	ntrials_ = tag->getOption<core::Size>( "ntrials", 10000 );
	set_preserve_detailed_balance( tag->getOption<bool>( "preserve_detailed_balance", 1 ) );
	temperature_ = tag->getOption<core::Real>( "temperature", 1.0 );
	set_inherit_scorefxn_temperature( tag->getOption<bool>( "inherit_scorefxn_temperature", inherit_scorefxn_temperature() ) );

	set_prob_uniform( tag->getOption<core::Real>( "prob_uniform", 0.0 ) );
	set_prob_withinrot( tag->getOption<core::Real>( "prob_withinrot", 0.0 ) );
	set_prob_random_pert_current( tag->getOption<core::Real>( "prob_random_pert_current", 0.0 ) );
	core::Real between_rot = 1.0 - prob_uniform() - prob_withinrot () - prob_random_pert_current();

	setup( rosetta_scripts::parse_score_function( tag, data )->clone() );

	TR
		<< "Initialized SidechainMCMover from .xml file:"
		<< " ntrials=" << ntrials_
		<< " temperature= " << temperature_
		<< " detailed balance= " << (preserve_detailed_balance()?"true":"false")
		<< " scorefunction=" << rosetta_scripts::get_score_function_name(tag)
		<< " Probablities uniform/withinrot/random_pert/betweenrot: "
		<< prob_uniform() << '/' <<prob_withinrot() << '/' << prob_random_pert_current() << '/' << between_rot
		<< std::endl;
}

void
SidechainMCMover::initialize_simulation(
	core::pose::Pose & pose,
	protocols::canonical_sampling::MetropolisHastingsMover const & metropolis_hastings_mover,
	core::Size cycle //default=0; non-zero if trajectory is restarted
)
{
	SidechainMover::initialize_simulation(pose, metropolis_hastings_mover,cycle);

	if (inherit_scorefxn_temperature_) {
		runtime_assert(metropolis_hastings_mover_);
		set_scorefunction(metropolis_hastings_mover_->monte_carlo()->score_function());
		set_temperature(metropolis_hastings_mover_->monte_carlo()->temperature());
	}
}


} // sidechain_moves
} // simple_moves
} // protocols
