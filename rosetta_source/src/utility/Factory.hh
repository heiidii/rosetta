// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=1 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file utility/Factory.hh
/// @brief A singleton templated factory, is is usually created by protocols/init/init.cc
/// object generated by factory must have name() and create()
/// Factory< Outputter > and Factory< Inputter > are two examples of this factory in use
/// Wonder if there is a way to do lazy initialization of this singleton
/// @author Ken Jung

#ifndef INCLUDED_utility_Factory_hh
#define INCLUDED_utility_Factory_hh

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>
#include <boost/functional/factory.hpp>
#include <utility/exit.hh> 
#include <string>
#include <map>

namespace utility {

template < class T >
class Factory {

		typedef typename boost::scoped_ptr < Factory<T> > FactorySCP;
		typedef typename boost::shared_ptr < T > TSP;
		typedef typename boost::function< TSP () > Creator;
		typedef typename std::map<std::string, Creator> CreatorMap;
		typedef typename CreatorMap::const_iterator cmap_itr;

		private:
    Factory(){};
  public:
    ~Factory(){}
    static Factory * get_instance();
				TSP from_string( std::string const & name );
				bool has_string( std::string const & name );
    void register_string( std::string const & name, Creator m );

  private:
    static FactorySCP factory_;
				CreatorMap cmap_;
};

template < class T >
bool Factory<T>::has_string( std::string const & name ) {
		typename Factory<T>::cmap_itr itr =  cmap_.find( name );
		if( itr != cmap_.end() ) {
				return true;
		} else {
				return false;
		}
}

template < class T >
typename Factory<T>::TSP Factory<T>::from_string( std::string const & name ) {
		typename Factory<T>::cmap_itr itr =  cmap_.find( name );
		if( itr != cmap_.end() ) {
				if ( ! itr->second ) 
					utility_exit_with_message( "Error: String \"" + name + "\" is registered, but points to an invalid create() function.  Check where you registered with this factory that you are passing the right function when you call register_string().");
				TSP m = itr->second();
				return m->create();
		} else {
					utility_exit_with_message( "Error: String \"" + name + "\" is not registered.  Are you sure you registered that string with this Factory?  Try checking protocols/init/init.<something>Factory.ihh for " + name + ", boost::factory seems to point to invalid functor" );
				return TSP();
		}
}

template < class T >
Factory<T> * Factory<T>::get_instance() {
  if ( ! factory_ ) {
				typename Factory<T>::FactorySCP tmp(new Factory<T>());
				factory_.swap( tmp );
		}
		return factory_.get();
}

// need to change all these to actually throw exceptions
template < class T >
void Factory<T>::register_string( std::string const & name, Creator m ) {
		if ( name == "UNDEFINED NAME" )
				utility_exit_with_message("Can't map derived object with undefined type name. In english, you're trying to register a class that doesn't have a name() function");
		if ( cmap_.find( name ) != cmap_.end() )
				utility_exit_with_message("Factory already has a Creator with name \"" + name + "\".  Conflicting object names" );
		cmap_[name] = m;
}


template <class T, class U>
class FactoryRegister {
public:
    FactoryRegister(){
      Factory<T> * factory = Factory<T>::get_instance();
      factory->register_string( U::name(), boost::factory< boost::shared_ptr< U > >() );
    }
};

} //utility
#endif
