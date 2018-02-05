// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/fold_from_loops/DisplayPoseLabelsMover.hh
/// @brief Prints all the Labels of the Pose
/// @author Jaume Bonet (jaume.bonet@gmail.com)

#ifndef INCLUDED_fold_from_loops_movers_DisplayPoseLabelsMover_hh
#define INCLUDED_fold_from_loops_movers_DisplayPoseLabelsMover_hh

// Unit headers
#include <protocols/fold_from_loops/movers/DisplayPoseLabelsMover.fwd.hh>
#include <protocols/moves/Mover.hh>

// Protocol headers
#include <protocols/moves/DsspMover.hh>
#include <core/kinematics/MoveMap.hh>
#include <core/kinematics/FoldTree.hh>

// Core headers
#include <core/pose/Pose.fwd.hh>
#include <core/select/movemap/MoveMapFactory.hh>
#include <core/pack/task/TaskFactory.hh>

// Basic/Utility headers
#include <basic/datacache/DataMap.fwd.hh>

namespace protocols {
namespace fold_from_loops {
namespace movers {

///@brief Adds constraints generated by ConstraintGenerators to a pose
class DisplayPoseLabelsMover : public protocols::moves::Mover {

public:
	DisplayPoseLabelsMover();

	// destructor (important for properly forward-declaring smart-pointer members)
	~DisplayPoseLabelsMover() override;

	void
	apply( core::pose::Pose & pose ) override;

	/// @brief parse XML tag (to use this Mover in Rosetta Scripts)
	void
	parse_my_tag(
		utility::tag::TagCOP tag,
		basic::datacache::DataMap & data,
		protocols::filters::Filters_map const & filters,
		protocols::moves::Movers_map const & movers,
		core::pose::Pose const & reference_pose ) override;

	void add_labels_as_remark( core::pose::Pose & pose ) const;

	core::Size title_width() const { return title_width_; };
	void title_width( core::Size pick ) { title_width_ = pick; };
	bool use_dssp() const { return use_dssp_; };
	void use_dssp( bool pick ) { use_dssp_ = pick; };
	bool write() const { return write_; };
	void write( bool pick ) { write_ = pick; };
	core::select::movemap::MoveMapFactoryOP movemap_factory() const { return movemap_factory_; };
	void movemap_factory( core::select::movemap::MoveMapFactoryOP pick ) { movemap_factory_ = pick; };
	core::kinematics::MoveMapOP movemap_from_pose( core::pose::Pose const & pose ) const {
		return movemap_factory_->create_movemap_from_pose( pose );
	};
	core::pack::task::TaskFactoryOP tasks() const { return tasks_; };
	void tasks( core::pack::task::TaskFactoryOP pick ) { tasks_ = pick; };

private:
	std::map< std::string, std::string > find_labels( core::pose::Pose const & pose ) const;
	std::map< std::string, utility::vector1< core::Size > >
	find_labels2( core::pose::Pose const & pose ) const;
	void print_data( std::string id, std::string content, bool force );
	void print_movemap( core::pose::Pose const & pose );
	void print_task_operators( core::pose::Pose const & pose );
	void simple_visualize_fold_tree( core::kinematics::FoldTree const & fold_tree, std::ostream& out );
	bool is_nubinitio_tree( core::kinematics::FoldTree const & fold_tree );

	static core::Size default_title_width() { return 15; };
	static bool default_use_dssp() { return true; };
	static bool default_write() { return false; };

public:
	/// @brief required in the context of the parser/scripting scheme
	protocols::moves::MoverOP fresh_instance() const override;
	/// @brief required in the context of the parser/scripting scheme
	protocols::moves::MoverOP clone() const override;
	std::string get_name() const override;
	static std::string mover_name();
	static void provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd );

private:
	core::Size title_width_;
	bool use_dssp_;
	core::select::movemap::MoveMapFactoryOP movemap_factory_;
	core::pack::task::TaskFactoryOP tasks_;
	bool write_;

};

}
} //protocols
} //fold_from_loops

#endif //INCLUDED_fold_from_loops_DisplayPoseLabelsMover_hh