// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/antibody/AntibodyEnumManager.hh
/// @brief Functions for AntibodyEnumerators
/// @author Jared Adolf-Bryfogle (jadolfbr@gmail.com)

#ifndef INCLUDED_protocols_antibody_AntibodyEnumManager_hh
#define INCLUDED_protocols_antibody_AntibodyEnumManager_hh

#include <protocols/antibody/AntibodyEnumManager.fwd.hh>

#include <utility/pointer/ReferenceCount.hh>

#include <protocols/antibody/AntibodyEnum.hh>
#include <utility/vector1.hh>
#include <map>
#include <string>

namespace protocols{
namespace antibody{

///@brief Interface to this class is in AntibodyInfo.
class AntibodyEnumManager : public utility::pointer::ReferenceCount{
	
	
public:

	AntibodyEnumManager();
	
	virtual ~AntibodyEnumManager();
	
	////////////////// CDR names ///////////////////////////////////////////////
	
	CDRNameEnum
	cdr_name_string_to_enum(std::string const & cdr_name) const;
	
	std::string
	cdr_name_enum_to_string(CDRNameEnum const cdr_name) const;
	
	bool
	cdr_name_is_present(std::string const & cdr_name) const;
	
	////////////////// Numbering Schemes ///////////////////////////////////////
	
	AntibodyNumberingSchemeEnum
	numbering_scheme_string_to_enum(std::string const & numbering_scheme) const;
		
	std::string
	numbering_scheme_enum_to_string(AntibodyNumberingSchemeEnum const numbering_scheme) const;
	

	////////////////// H3 Base Type ////////////////////////////////////////////
	
	H3BaseTypeEnum
	h3_base_type_string_to_enum(std::string const & base_type) const;
	
	std::string
	h3_base_type_enum_to_string(H3BaseTypeEnum const base_type) const;
	
	
	///////////////// Packing Angle ////////////////////////////////////////////

	PackingAngleEnum
	packing_angle_string_to_enum(std::string const & angle_type) const;
	
	std::string
	packing_angle_enum_to_string(PackingAngleEnum const angle_type) const;
	
	
private:
	

	
	void setup();
	
	
	
	utility::vector1< std::string >  cdr_name_to_string_;
	std::map< std::string, CDRNameEnum > cdr_name_to_enum_;
	
	utility::vector1< std::string >  numbering_scheme_to_string_;
	std::map< std::string, AntibodyNumberingSchemeEnum > numbering_scheme_to_enum_;
	
	utility::vector1< std::string >  h3_base_type_to_string_;
	std::map< std::string, H3BaseTypeEnum > h3_base_type_to_enum_;
	
	utility::vector1< std::string >  packing_angle_to_string_;
	std::map< std::string, PackingAngleEnum > packing_angle_to_enum_;
};
}
}

#endif //#ifndef INCLUDED_protocols/antibody/ANTIBODYENUMMANAGER_HH

