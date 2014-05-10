// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file src/core/environment/Enironment.hh
/// @brief An environment that automatically distributes rights to shared degrees of freedom (e.g. fold tree)
///
/// @author Justin Porter

#ifndef INCLUDED_core_environment_DofPassport_hh
#define INCLUDED_core_environment_DofPassport_hh

// Unit Headers
#include <core/environment/DofPassport.fwd.hh>

// Package headers
#include <core/environment/EnvCore.fwd.hh>

// Project headers
#include <core/kinematics/Jump.fwd.hh>
#include <core/kinematics/MoveMap.fwd.hh>

#include <core/conformation/Conformation.hh>

#include <utility/pointer/ReferenceCount.hh>

#include <core/id/TorsionID.hh>
#include <core/id/AtomID.hh>
#include <core/id/DOF_ID.hh>
#include <core/id/JumpID.hh>

// C++ Headers
#include <ostream>
#include <set>

// ObjexxFCL Headers

namespace core {
namespace environment {

class DofPassport : public utility::pointer::ReferenceCount {

  //make EnvCore a friend so that we can call the constructor of DofPassport
  friend class core::environment::EnvCore;

public:

  virtual ~DofPassport();

  void show( std::ostream& ) const;

  core::kinematics::MoveMapOP render_movemap( conformation::Conformation const& ) const;

  ///@brief configure passport to allow access to a bond length or angle dof_id
  ///@param id the DOF_ID to allow access to
  ///@pre parameter id must have DOF_Type id::D or id::THETA.
  ///@pre id must be valid
  void add_dof_access( id::DOF_ID const& id );

  ///@brief configure passport to allow access to a torsional dof
  ///@param dof_id the DOF_ID representation of the torsional DOF
  ///@param tor_id the TorsionID represenatation of the torsional DOF
  ///@pre dof_id must have DOF_Type id::PHI.
  ///@pre both dof_id and tor_id must be valid.
  // Undefined, commenting out to fix PyRosetta build  void add_dof_access( id::DOF_ID const& dof_id, id::TorsionID const& tor_id );

  ///@brief configure passport to allow access to a jump dof
  ///@param atom_id the AtomID of the atom building the jump
  ///@param jump_num the FoldTree
  ///@pre dof_id must have DOF_Type id::PHI.
  ///@pre both dof_id and tor_id must be valid.
  // Undefined, commenting out to fix PyRosetta build
  // void add_jump_access( core::id::AtomID const& atom_id,
  //                       core::Size const& jump_num,
  //                       core::id::JumpID const& jump_id );

  void revoke_all_access();

  // Undefined, commenting out to fix PyRosetta build
  // bool jump_access( EnvCore const&, core::id::AtomID const& ) const;
  // bool jump_access( EnvCore const&, core::Size const& jump_nr ) const;
  // bool jump_access( EnvCore const&, id::JumpID const& jid ) const;

  //@brief Check access in this passport for an id
  bool dof_access( core::id::DOF_ID const& id ) const;

  //@brief Check access in this passport for both correct environment and correct id.
  bool dof_access( EnvCore const&, core::id::DOF_ID const& id ) const;

  std::string const& mover() const;

private:

  DofPassport( std::string const& mover, Size env_id);

  DofPassport( DofPassport const& );

  //General code used by most (all?) access checks.
  bool access_check( EnvCore const& env, bool type_specific_check ) const;

  std::string mover_;
  core::Size env_id_;

  // Capable of storing all movable dofs.
  std::set< core::id::DOF_ID > accessible_dofs_;
  std::set< core::Size > accessible_jump_numbers_;
  std::set< core::id::TorsionID > accessible_torsions_;
  std::set< core::id::JumpID > accessible_jump_ids_;

}; // end DofPassport base class

extern std::ostream& operator<<( std::ostream&, DofPassport const& );
} // environment
} // core

#endif //INCLUDED_core_environment_DofPassport_HH
