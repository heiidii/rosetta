// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   devel/matdes/OligomericAverageDegreeFilter.cc
/// @brief  Calculates AverageDegree within the context of an unbound oligomer
/// @author Neil King (neilking@u.washington.edu)

// Unit Headers
#include <devel/matdes/OligomericAverageDegreeFilter.hh>
#include <devel/matdes/OligomericAverageDegreeFilterCreator.hh>

// Project Headers
#include <core/types.hh>
#include <core/pose/Pose.hh>
#include <core/pose/symmetry/util.hh>
#include <core/pack/task/PackerTask.hh>
#include <core/pack/task/TaskFactory.hh>
#include <core/conformation/Conformation.hh>

// Utility headers
#include <utility/vector1.fwd.hh>
#include <basic/Tracer.hh>
#include <protocols/rosetta_scripts/util.hh>
#include <protocols/elscripts/util.hh>
#include <ObjexxFCL/FArray1D.hh>
#include <ObjexxFCL/format.hh>
#include <core/kinematics/FoldTree.hh>
#include <core/conformation/Residue.hh>
#include <protocols/moves/DataMap.hh>
#include <protocols/moves/Mover.hh>
#include <protocols/jd2/Job.hh>
#include <protocols/jd2/JobDistributor.hh>

// Parser headers
#include <protocols/filters/Filter.hh>
#include <utility/tag/Tag.hh>
#include <utility/string_util.hh>

#include <utility/vector0.hh>
#include <utility/vector1.hh>


//// C++ headers
static basic::Tracer TR("devel.matdes.OligomericAverageDegreeFilter");

namespace devel {
namespace matdes {

// @brief default constructor
OligomericAverageDegreeFilter::OligomericAverageDegreeFilter():
  task_factory_( NULL ),
  threshold_( 0 ),
  distance_threshold_( 10.0 ),
	jump_id_( 1 ),
	sym_dof_names_( "" ),
	multicomp_( 0 )
{}


// @brief constructor with arguments
OligomericAverageDegreeFilter::OligomericAverageDegreeFilter( core::pack::task::TaskFactoryOP task_factory, core::Real const t, core::Real const d, core::Size jump, std::string dof_names, bool mcomp ):
	task_factory_( task_factory ),
	threshold_( t ),
	distance_threshold_( d ),
	jump_id_( jump),
	sym_dof_names_( dof_names ),
	multicomp_( mcomp )
{}

// @brief copy constructor
OligomericAverageDegreeFilter::OligomericAverageDegreeFilter( OligomericAverageDegreeFilter const & rval ):
	Super( rval ),
	task_factory_( rval.task_factory_ ),
	threshold_( rval.threshold_ ),
	distance_threshold_( rval.distance_threshold_ ),
	jump_id_( rval.jump_id_ ),
	sym_dof_names_( rval.sym_dof_names_ ),
	multicomp_( rval.multicomp_ )
{}

// @brief destructor
OligomericAverageDegreeFilter::~OligomericAverageDegreeFilter() {}

protocols::filters::FilterOP
OligomericAverageDegreeFilter::fresh_instance() const{
  return new OligomericAverageDegreeFilter();
}

protocols::filters::FilterOP
OligomericAverageDegreeFilter::clone() const{
  return new OligomericAverageDegreeFilter( *this );
}

// @brief getters
core::pack::task::TaskFactoryOP OligomericAverageDegreeFilter::task_factory() const { return task_factory_; }
core::Real OligomericAverageDegreeFilter::threshold() const { return threshold_; }
core::Real OligomericAverageDegreeFilter::distance_threshold() const { return distance_threshold_; }
core::Size OligomericAverageDegreeFilter::jump_id() const { return jump_id_; }
std::string OligomericAverageDegreeFilter::sym_dof_names() const { return sym_dof_names_; }
bool OligomericAverageDegreeFilter::write2pdb() const { return write2pdb_; }
bool OligomericAverageDegreeFilter::multicomp() const { return multicomp_; }

// @brief setters
void OligomericAverageDegreeFilter::task_factory( core::pack::task::TaskFactoryOP task_factory ) { task_factory_ = task_factory; }
void OligomericAverageDegreeFilter::threshold( core::Real const t ) { threshold_ = t; }
void OligomericAverageDegreeFilter::distance_threshold( core::Real const d ) { distance_threshold_ = d; }
void OligomericAverageDegreeFilter::jump_id( core::Size const jump ) { jump_id_ = jump; }
void OligomericAverageDegreeFilter::sym_dof_names( std::string const dof_names ) { sym_dof_names_ = dof_names; }
void OligomericAverageDegreeFilter::write2pdb( bool const write ) { write2pdb_ = write; }
void OligomericAverageDegreeFilter::multicomp( bool const mcomp ) { multicomp_ = mcomp; }

/// @brief
core::Real OligomericAverageDegreeFilter::compute( Pose const & pose, bool const & verbose, bool const & write ) const
{

	runtime_assert( task_factory() );
	utility::vector1<std::string> sym_dof_name_list;
	std::string sym_dof_name;
  core::pack::task::PackerTaskCOP packer_task( task_factory()->create_task_and_apply_taskoperations( pose ) );

  // Partition pose according to specified jump and symmetry information

	Size nsubposes = 1;
	if ( sym_dof_names_ != "" ) {
		sym_dof_name_list = utility::string_split( sym_dof_names_ , ',' );
		nsubposes = sym_dof_name_list.size();
		TR.Debug << "nsubposes: " << nsubposes << std::endl;
	}

  core::Size count_residues( 0 );
  core::Size count_neighbors( 0 );
	for (Size i = 1; i <= nsubposes; i++) {
		int sym_aware_jump_id;
		if ( sym_dof_names() != "" ) {
			TR.Debug << "computing neighbors for sym_dof_name " << sym_dof_name_list[i] << std::endl;
			sym_aware_jump_id = core::pose::symmetry::sym_dof_jump_num( pose, sym_dof_name_list[i] );
  	} else {
			sym_aware_jump_id = core::pose::symmetry::get_sym_aware_jump_num(pose, jump_id() );
		}
		ObjexxFCL::FArray1D_bool is_upstream ( pose.total_residue(), false );
		pose.fold_tree().partition_by_jump( sym_aware_jump_id, is_upstream );

		// Count neighbors in the sub_pose
		for( core::Size resi=1; resi<=pose.total_residue(); ++resi ){
			if(pose.residue_type(resi).aa() == core::chemical::aa_vrt) continue;
			if( packer_task->being_packed( resi ) ){
				//if ( is_upstream(resi) ) { utility_exit_with_message("Your packable residues are upstream of the jump you defined! Check your TaskOperation or your jump."); }
				bool which_side = is_upstream(resi);  //fpd  designable residues may be upstream ... that's ok as long as we're separated
				if ( multicomp() && (which_side != false) ) continue; 
				++count_residues;
				TR.Debug << "resi: " << resi << std::endl;
				core::Size resi_neighbors( 0 );
				core::conformation::Residue const res_target( pose.conformation().residue( resi ) );
				for( core::Size j=1; j<=pose.total_residue(); ++j ) {
					if ( is_upstream(j) == which_side ) {
						core::conformation::Residue const resj( pose.residue( j ) );
						if(resj.aa() == core::chemical::aa_vrt) continue;
						core::Real const distance( resj.xyz( resj.nbr_atom() ).distance( res_target.xyz( res_target.nbr_atom() ) ) );
					 	if( distance <= distance_threshold() ){
							TR.Debug << "j: " << j << std::endl;
							TR.Debug << "count_residues: " << count_residues << " count_neighbors: " << count_neighbors << std::endl;
						 	++count_neighbors;
						 	++resi_neighbors;
					 	}
					}
				}
			if ( verbose ) { TR << "Connectivity of " << res_target.name3() << resi << " is " << resi_neighbors << std::endl; }
			if ( write ) { write_to_pdb( resi, res_target.name3(), resi_neighbors ); }
			}
		}
	}
  return( (core::Real) count_neighbors / count_residues );

} // compute

void OligomericAverageDegreeFilter::write_to_pdb( core::Size const residue, std::string const residue_name, core::Size const neighbors ) const
{

	protocols::jd2::JobOP job(protocols::jd2::JobDistributor::get_instance()->current_job());
	std::string filter_name = this->name();
	std::string user_name = this->get_user_defined_name();
	std::string output_string = filter_name + " " + user_name + ": " + residue_name + ObjexxFCL::string_of(residue) + " = " + ObjexxFCL::string_of(neighbors);
	job->add_string(output_string);

}

// @brief returns true if the given pose passes the filter, false otherwise.
// In this case, the test is whether the residues defined by the TaskOperation(s)
// have a high enough OligomericAverageDegree (i.e., are sufficiently
// well-anchored within the building block).
// The difference between this filter and the standard AverageDegree filter
// is that here we create an "unbound" pose by partitioning the pose
// by the rigid body DOF specified in the pose's symmetry information, and
// then calculating the number of neighbors for each residue over the entire
// unbound pose rather than within each chain. This allows calculation of the
// AverageDegree within the context of oligomeric building blocks.
bool OligomericAverageDegreeFilter::apply( Pose const & pose ) const
{

	// Get the oligomeric avg_deg from the compute function and filter
  core::Real const average_degree( compute( pose, true, write2pdb() ) );
  return( average_degree >= threshold() );

} // apply_filter

/// @brief parse xml
void
OligomericAverageDegreeFilter::parse_my_tag(
	utility::tag::TagPtr const tag,
	protocols::moves::DataMap & data,
	protocols::filters::Filters_map const &,
	protocols::moves::Movers_map const &,
	core::pose::Pose const & )
{
  TR << "OligomericAverageDegreeFilter"<<std::endl;
  task_factory( protocols::rosetta_scripts::parse_task_operations( tag, data ) );
  threshold( tag->getOption< core::Size >( "threshold", 0 ) );
  distance_threshold( tag->getOption< core::Real >( "distance_threshold", 10.0 ) );
  jump_id( tag->getOption< core::Size >( "jump", 1 ) );
	sym_dof_names( tag->getOption< std::string >( "sym_dof_names" , "JS1" ) );
	write2pdb( tag->getOption< bool >("write2pdb", 0) );
	multicomp( tag->getOption< bool >("multicomp", 0) );
  TR << "with options threshold: " <<threshold() << " and distance_threshold " << distance_threshold() << std::endl;
}

void OligomericAverageDegreeFilter::parse_def( utility::lua::LuaObject const & def,
				utility::lua::LuaObject const & ,
				utility::lua::LuaObject const & tasks ) {
  TR << "OligomericAverageDegreeFilter"<<std::endl;
	task_factory( protocols::elscripts::parse_taskdef( def["tasks"], tasks ));
	threshold( def["threshold"] ? def["threshold"].to<core::Size>() : 0 );
	distance_threshold( def["distance_threshold"] ? def["distance_threshold"].to<core::Real>() : 10.0 );
	jump_id( def["jump"] ? def["jump"].to<core::Size>() : 1 );
	write2pdb( def["write2pdb"] ? def["write2pdb"].to<bool>() : 0 );
  TR << "with options threshold: " <<threshold() << " and distance_threshold " << distance_threshold() << std::endl;
}
core::Real
OligomericAverageDegreeFilter::report_sm( core::pose::Pose const & pose ) const
{
  return( compute( pose, false, false ) );
} 

void
OligomericAverageDegreeFilter::report( std::ostream & out, core::pose::Pose const & pose ) const
{
  out << "OligomericAverageDegreeFilter returns " << compute( pose, false, false ) << std::endl;
}

protocols::filters::FilterOP
OligomericAverageDegreeFilterCreator::create_filter() const { return new OligomericAverageDegreeFilter; }

std::string
OligomericAverageDegreeFilterCreator::keyname() const { return "OligomericAverageDegree"; }


} // matdes
} // devel
