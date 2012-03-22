// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet;
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file MessageListener.hh
///
/// @brief base message listening class to be derived from for all message listeners
/// @author Tim Jacobs


#ifndef INCLUDED_protocols_jd2_message_listening_MessageListener_HH
#define INCLUDED_protocols_jd2_message_listening_MessageListener_HH

#include <protocols/jd2/message_listening/MessageListener.fwd.hh>
#include <utility/pointer/ReferenceCount.hh>

#include <string>

namespace protocols {
namespace jd2 {
namespace message_listening {

class MessageListener : public utility::pointer::ReferenceCount
{

public:
	//recieve data from a slave node
	virtual void recieve(std::string data)=0;

	//Given the identifier for a slave node, fill the return data to be sent
	//back and specify whether the slave should give more data. The use case for
	//this would be when you have some logic in the slave that you want executed
	//only once, or only once for each unique identifier.
	virtual bool request(std::string identifier, std::string & return_data)=0;

};

} //namespace message_listening
} //namespace jd2
} //namespace protocols

#endif
