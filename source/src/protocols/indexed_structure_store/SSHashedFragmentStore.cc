// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.
//
/// @file
/// @details Parses the VALL by secondary structure after it is read in hdf5 format
/// @author TJ Brunette tjbrunette@gmail.com




#include <protocols/indexed_structure_store/FragmentLookup.hh>
#include <protocols/indexed_structure_store/FragmentStore.hh>
#include <protocols/indexed_structure_store/FragmentStoreManager.hh>
#include <protocols/indexed_structure_store/SSHashedFragmentStore.hh>
#include <protocols/indexed_structure_store/search/QueryDatabase.hh>
#include <protocols/indexed_structure_store/StructureStoreManager.hh>
#include <core/pose/Pose.hh>
#include <core/scoring/dssp/Dssp.hh>
#include <core/sequence/SSManager.hh>
#include <core/types.hh>

#include <basic/Tracer.hh>
#include <basic/options/option.hh>
#include <basic/options/keys/indexed_structure_store.OptionKeys.gen.hh>

#include <utility/vector1.hh>
#include <numeric/alignment/QCPKernel.hh>
#include <numeric/xyzVector.hh>

#include <map>
#include <set>
#include <queue>


static basic::Tracer TR( "core.indexed_structure_store.SSHashedFragmentStore" );


namespace protocols {
namespace indexed_structure_store {
using namespace protocols;
using utility::vector1;
using namespace std;
using namespace numeric;

BackboneStub::BackboneStub(numeric::Real rmsd_match_i, numeric::Size index_match_i, numeric::Size ss_index_match_i){
	rmsd_match = rmsd_match_i;
	index_match = index_match_i;
	ss_index_match = ss_index_match_i;
}

SSHashedFragmentStore::SSHashedFragmentStore(std::string const & fragment_store_path,std::string const & fragment_store_format, std::string const & fragment_store_compression){
	std::map<numeric::Size, protocols::indexed_structure_store::FragmentStoreOP> found_fragment_store;
	found_fragment_store = FragmentStoreManager::get_instance()->find_previously_loaded_fragmentStore(fragment_store_path);
	if ( found_fragment_store.empty() != true ) {
		SSHashedFragmentStore_ = found_fragment_store;
	} else {
		//standard case. Load one of the two database formats
		if ( fragment_store_format=="hashed" ) {
			load_prehashed_fragmentStore(fragment_store_path);
		} else {
			Size fragment_size = 9;
			SSHashedFragmentStore_ = FragmentStoreManager::get_instance()->load_and_hash_fragmentStore(fragment_store_path,fragment_store_compression,fragment_size);
		}
	}
	if ( SS_stub_HashedFragmentStoreIndex_.size()==0 ) {
		init_SS_stub_HashedFragmentStore();
	}
}


void SSHashedFragmentStore::load_prehashed_fragmentStore(std::string const & fragment_store_path){
	using namespace basic::options;
	using namespace basic::options::OptionKeys;
	using namespace OptionKeys::indexed_structure_store;
	using namespace protocols::indexed_structure_store;
	std::string store_path = "";
	if ( fragment_store_path.size()>=1 ) {
		store_path = fragment_store_path;
	} else {
		store_path = option[OptionKeys::indexed_structure_store::fragment_store](); //error checking occurs in DB loading
	}
	vector1<string> fields_to_load;
	vector1<string> fields_to_load_types;
	string store_name = option[OptionKeys::indexed_structure_store::store_name]();
	string group_field ="ss_bin";
	fields_to_load.push_back("ss_bin");
	fields_to_load_types.push_back("int64");
	fields_to_load.push_back("phi");
	fields_to_load_types.push_back("real_per_residue");
	fields_to_load.push_back("psi");
	fields_to_load_types.push_back("real_per_residue");
	fields_to_load.push_back("omega");
	fields_to_load_types.push_back("real_per_residue");
	fields_to_load.push_back("cen");
	fields_to_load_types.push_back("real_per_residue");
	fields_to_load.push_back("aa");
	fields_to_load_types.push_back("char_per_residue");
	if ( option[OptionKeys::indexed_structure_store::exclude_homo].user() ) {
		fields_to_load.push_back("name");
		fields_to_load_types.push_back("five_char_per_residue");
	}
	SSHashedFragmentStore_=FragmentStoreManager::get_instance()->load_grouped_fragment_store(group_field,store_name,store_path,fields_to_load,fields_to_load_types);
}


void SSHashedFragmentStore::set_threshold_distance(Real threshold_distance){
	std::map<core::Size, protocols::indexed_structure_store::FragmentStoreOP>::iterator fragStoreMap_iter;
	if ( SSHashedFragmentStore_.begin()->second->fragment_threshold_distances[0]>threshold_distance ) { //fragment threshold distance needs to be set to the lowest calling value.
		for ( fragStoreMap_iter = SSHashedFragmentStore_.begin(); fragStoreMap_iter != SSHashedFragmentStore_.end(); fragStoreMap_iter++ ) {
			fragStoreMap_iter->second->add_threshold_distance_allFrag(threshold_distance);
		}
	}
}


void SSHashedFragmentStore::init_SS_stub_HashedFragmentStore(){
	core::Size fragment_length = SSHashedFragmentStore_.begin()->second->fragment_specification.fragment_length;
	vector1<std::string> types;
	Size min_loop_length = 1;
	Size max_loop_length = fragment_length-4;
	vector1<std::string> ss_stubs;
	std::map<std::string,std::vector<core::Size> > stubs_to_residues;
	for ( Size ii=min_loop_length; ii<=max_loop_length; ++ii ) {
		Size front_ss_length = 2;
		Size back_ss_length = 2;
		if ( fragment_length>front_ss_length+back_ss_length+ii ) { //within tolerance add a residue to the front
			front_ss_length+=1;
		}
		if ( fragment_length>front_ss_length+back_ss_length+ii ) { //still within tolerance add a residue to the back
			back_ss_length+=1;
		}
		std::string stubType1 = ""; //H,H
		std::string stubType2 = ""; //H,E
		std::string stubType3 = ""; //E,H
		std::string stubType4 = ""; //E,E
		//the back needs to be loaded first
		stubType1.insert(0,back_ss_length,'H');
		stubType2.insert(0,back_ss_length,'H');
		stubType3.insert(0,back_ss_length,'E');
		stubType4.insert(0,back_ss_length,'E');
		stubType1.insert(0,ii,'L');
		stubType2.insert(0,ii,'L');
		stubType3.insert(0,ii,'L');
		stubType4.insert(0,ii,'L');
		stubType1.insert(0,front_ss_length,'H');
		stubType2.insert(0,front_ss_length,'E');
		stubType3.insert(0,front_ss_length,'H');
		stubType4.insert(0,front_ss_length,'E');
		ss_stubs.push_back(stubType1);
		ss_stubs.push_back(stubType2);
		ss_stubs.push_back(stubType3);
		ss_stubs.push_back(stubType4);
		std::vector<core::Size> residues;
		core::Size pos = 0;
		while ( pos<front_ss_length ) {
			residues.push_back(pos);
			pos+=1;
		}
		pos=front_ss_length+ii;
		core::Size jj=0;
		while ( jj<back_ss_length ) {
			residues.push_back(pos);
			jj+=1;
			pos+=1;
		}
		stubs_to_residues.insert(std::pair<std::string,std::vector<core::Size> > (stubType1,residues));
		stubs_to_residues.insert(std::pair<std::string,std::vector<core::Size> > (stubType2,residues));
		stubs_to_residues.insert(std::pair<std::string,std::vector<core::Size> > (stubType3,residues));
		stubs_to_residues.insert(std::pair<std::string,std::vector<core::Size> > (stubType4,residues));
	}
	core::sequence::SSManager SM;
	std::map<core::Size, protocols::indexed_structure_store::FragmentStoreOP>::iterator fragStoreMap_iter;
	for ( fragStoreMap_iter = SSHashedFragmentStore_.begin(); fragStoreMap_iter != SSHashedFragmentStore_.end(); fragStoreMap_iter++ ) {
		std::string fragStore_ss_string = SM.index2symbolString(fragStoreMap_iter->first,fragment_length);
		for ( core::Size ii=1; ii<=ss_stubs.size(); ++ii ) {
			if ( fragStore_ss_string.substr(0,ss_stubs[ii].size())==ss_stubs[ii] ) {
				std::vector<core::Size> residues = stubs_to_residues[ss_stubs[ii]];
				fragStoreMap_iter->second->generate_residue_subset_fragment_store(residues);
				if ( SS_stub_HashedFragmentStoreIndex_.find(ss_stubs[ii]) == SS_stub_HashedFragmentStoreIndex_.end() ) {
					vector1<core::Size> fragStoreIndex_vector;
					SS_stub_HashedFragmentStoreIndex_.insert(std::pair<std::string,vector1<core::Size> > (ss_stubs[ii],fragStoreIndex_vector));
				}
				SS_stub_HashedFragmentStoreIndex_[ss_stubs[ii]].push_back(fragStoreMap_iter->first);
			}
		}
	}
}


Size SSHashedFragmentStore::get_valid_resid(core::pose::Pose const & pose,int resid){
	using namespace protocols::indexed_structure_store;
	core::Size fragment_length = SSHashedFragmentStore_.begin()->second->fragment_specification.fragment_length;
	if ( resid<1 ) {
		TR << "invalid resid encountered n-term" << resid << std::endl;
		resid = 1;
	}
	if ( resid+fragment_length-1>pose.total_residue() ) {
		TR << "invalid resid encountered c-term" << resid << std::endl;
		resid = (int)pose.total_residue()-fragment_length+1;
	}
	return(core::Size(resid));
}

set<std::string> SSHashedFragmentStore::potential_valid_ss_strings(std::string frag_ss){
	//dssp sometimes makes errors this corrects some of thems.
	//Note: this was not designed to fix cases where two inaccuracies happened in the same fragment. ie HLLLHLH
	set<std::string> valid_frag_ss;
	set<std::string> tmp_frag_ss;
	set<std::string> union_frag_ss;
	valid_frag_ss.insert(frag_ss);
	set<std::string>::iterator iter;
	//step1 HL -> LL,HH
	for ( iter=valid_frag_ss.begin(); iter!=valid_frag_ss.end(); ++iter ) {
		core::Size found = iter->find("HL");
		while ( found!=std::string::npos ) {
			std::string tmp_string1 = *iter;
			std::string tmp_string2 = *iter;
			tmp_string1[found] ='L';
			tmp_string2[found+1] ='H';
			tmp_frag_ss.insert(tmp_string1);
			tmp_frag_ss.insert(tmp_string2);
			found = iter->find("HL",found+2);
		}
	}
	set_union(valid_frag_ss.begin(),valid_frag_ss.end(),tmp_frag_ss.begin(),tmp_frag_ss.end(),inserter(union_frag_ss,union_frag_ss.begin()));
	valid_frag_ss = union_frag_ss;
	tmp_frag_ss.clear();
	union_frag_ss.clear();
	//step2 EL -> LL,EE
	for ( iter=valid_frag_ss.begin(); iter!=valid_frag_ss.end(); ++iter ) {
		//set<std::string> union_frag_ss;
		core::Size found = iter->find("EL");
		while ( found!=std::string::npos ) {
			std::string tmp_string1 = *iter;
			std::string tmp_string2 = *iter;
			tmp_string1[found] ='L';
			tmp_string2[found+1] ='E';
			tmp_frag_ss.insert(tmp_string1);
			tmp_frag_ss.insert(tmp_string2);
			found = iter->find("EL",found+2);
		}
	}
	set_union(valid_frag_ss.begin(),valid_frag_ss.end(),tmp_frag_ss.begin(),tmp_frag_ss.end(),inserter(union_frag_ss,union_frag_ss.begin()));
	valid_frag_ss = union_frag_ss;
	tmp_frag_ss.clear();
	union_frag_ss.clear();
	//step3 LH -> LL,HH
	for ( iter=valid_frag_ss.begin(); iter!=valid_frag_ss.end(); ++iter ) {
		//set<std::string> union_frag_ss;
		core::Size found = iter->find("LH");
		while ( found!=std::string::npos ) {
			std::string tmp_string1 = *iter;
			std::string tmp_string2 = *iter;
			tmp_string1[found] ='H';
			tmp_string2[found+1] ='L';
			tmp_frag_ss.insert(tmp_string1);
			tmp_frag_ss.insert(tmp_string2);
			found = iter->find("LH",found+2);
		}
	}
	set_union(valid_frag_ss.begin(),valid_frag_ss.end(),tmp_frag_ss.begin(),tmp_frag_ss.end(),inserter(union_frag_ss,union_frag_ss.begin()));
	valid_frag_ss = union_frag_ss;
	tmp_frag_ss.clear();
	union_frag_ss.clear();
	//step4 LE -> LL,EE
	for ( iter=valid_frag_ss.begin(); iter!=valid_frag_ss.end(); ++iter ) {
		//set<std::string> union_frag_ss;
		core::Size found = iter->find("LE");
		while ( found!=std::string::npos ) {
			std::string tmp_string1 = *iter;
			std::string tmp_string2 = *iter;
			tmp_string1[found] ='E';
			tmp_string2[found+1] ='L';
			tmp_frag_ss.insert(tmp_string1);
			tmp_frag_ss.insert(tmp_string2);
			found = iter->find("EL",found+2);
		}
	}
	set_union(valid_frag_ss.begin(),valid_frag_ss.end(),tmp_frag_ss.begin(),tmp_frag_ss.end(),inserter(union_frag_ss,union_frag_ss.begin()));
	valid_frag_ss = union_frag_ss;
	tmp_frag_ss.clear();
	union_frag_ss.clear();
	return(valid_frag_ss);
}


Real SSHashedFragmentStore::max_rmsd_in_region(core::pose::Pose const & pose, vector1<core::Size> resids){
	Real high_rmsd=0;
	for ( core::Size ii=1; ii<=resids.size(); ++ii ) {
		core::Size valid_resid = get_valid_resid(pose,resids[ii]);
		Real tmp_rmsd = lookback_account_for_dssp_inaccuracy(pose,valid_resid,true,0.0);
		if ( tmp_rmsd >high_rmsd ) {
			high_rmsd=tmp_rmsd;
		}
	}
	return(high_rmsd);
}

Real SSHashedFragmentStore::lookback_account_for_dssp_inaccuracy(core::pose::Pose const & pose, core::Size resid,bool find_closest, Real rms_threshold){
	core::scoring::dssp::Dssp dssp( pose );
	dssp.dssp_reduced();
	std::string dssp_string = dssp.get_dssp_secstruct();
	core::Size fragment_length = SSHashedFragmentStore_.begin()->second->fragment_specification.fragment_length;
	std::string frag_ss = dssp_string.substr(resid-1,fragment_length);
	return(lookback_account_for_dssp_inaccuracy(pose,resid,frag_ss,find_closest,rms_threshold));
}

Real SSHashedFragmentStore::lookback_account_for_dssp_inaccuracy(core::pose::Pose const & pose, core::Size resid,std::string frag_ss, bool find_closest, Real rms_threshold){
	set<std::string> valid_frag_ss = potential_valid_ss_strings(frag_ss);
	set<std::string>::iterator iter;
	iter = valid_frag_ss.find("HHHHHHHHH"); //only check all helical if that was requested. This is for time.
	if ( iter!=valid_frag_ss.end() ) {
		valid_frag_ss.erase(iter);
	}
	iter = valid_frag_ss.find(frag_ss);
	if ( iter!=valid_frag_ss.end() ) {
		valid_frag_ss.erase(iter);
	}
	Real low_rmsd = lookback(pose,resid,frag_ss,find_closest); //tries original first.
	for ( iter=valid_frag_ss.begin(); iter!=valid_frag_ss.end(); ++iter ) {
		if ( low_rmsd>rms_threshold || find_closest ) {
			Real tmp_rmsd = lookback(pose,resid,*iter,find_closest);
			if ( tmp_rmsd<low_rmsd ) {
				low_rmsd = tmp_rmsd;
			}
		}
	}
	return(low_rmsd);
}

Real SSHashedFragmentStore::lookback_account_for_dssp_inaccuracy(core::pose::Pose const & pose, core::Size resid, std::string frag_ss, Real & match_rmsd, core::Size & match_index, core::Size & match_ss_index){
	using namespace protocols::indexed_structure_store;
	set<std::string> valid_frag_ss = potential_valid_ss_strings(frag_ss);
	set<std::string>::iterator iter;
	iter = valid_frag_ss.find("HHHHHHHHH"); //only check all helical if that was requested. This is for time.
	if ( iter!=valid_frag_ss.end() ) {
		valid_frag_ss.erase(iter);
	}
	Real low_rmsd = 5;
	core::sequence::SSManager SM;
	core::Size fragment_length = get_fragment_length();
	//get initial resid
	std::vector< xyzVector<Real> > coordinates;
	for ( core::Size ii = 0;  ii < fragment_length; ++ii ) {
		for ( std::string const & atom_name : get_fragment_store()->fragment_specification.fragment_atoms ) {
			coordinates.push_back(pose.residue(resid+ii).xyz(atom_name));
		}
	}
	//find top hit
	for ( iter=valid_frag_ss.begin(); iter!=valid_frag_ss.end(); ++iter ) {
		core::Size ss_index = SM.symbolString2index(*iter);
		if ( SSHashedFragmentStore_.find(ss_index)  != SSHashedFragmentStore_.end() ) {
			FragmentStoreOP selected_fragStoreOP = SSHashedFragmentStore_.at(ss_index);
			FragmentLookupOP selected_fragLookupOP = selected_fragStoreOP->get_fragmentLookup();
			FragmentLookupResult lookupResult = selected_fragLookupOP->lookup_closest_fragment(&coordinates[0]);
			Real tmp_rmsd = lookupResult.match_rmsd;
			core::Size tmp_index = lookupResult.match_index;
			if ( tmp_rmsd<low_rmsd ) {
				low_rmsd= tmp_rmsd;
				match_ss_index = ss_index;
				match_index = tmp_index;
				match_rmsd = tmp_rmsd;
			}
		}
	}
	return(low_rmsd);
}


Real SSHashedFragmentStore::lookback(core::pose::Pose const & pose, core::Size resid){
	using namespace protocols::indexed_structure_store;
	core::scoring::dssp::Dssp dssp( pose );
	dssp.dssp_reduced();
	std::string dssp_string = dssp.get_dssp_secstruct();
	core::Size fragment_length = SSHashedFragmentStore_.begin()->second->fragment_specification.fragment_length;
	std::string frag_ss = dssp_string.substr(resid-1,fragment_length);
	return(lookback(pose,resid,frag_ss,true));
}


Real SSHashedFragmentStore::lookback(core::pose::Pose const & pose, core::Size resid,string frag_ss,bool find_closest){
	using namespace protocols::indexed_structure_store;
	core::sequence::SSManager SM;
	core::Size fragmentStore_fragment_length = SSHashedFragmentStore_.begin()->second->fragment_specification.fragment_length;
	core::Size ss_index = SM.symbolString2index(frag_ss);
	Real returnRmsd;
	if ( SSHashedFragmentStore_.find(ss_index)  != SSHashedFragmentStore_.end() ) {
		//case where item is found in map;
		FragmentStoreOP selected_fragStoreOP = SSHashedFragmentStore_.at(ss_index);
		FragmentLookupOP selected_fragLookupOP = selected_fragStoreOP->get_fragmentLookup();
		std::vector< xyzVector<Real> > coordinates;
		for ( core::Size ii = 0;  ii < fragmentStore_fragment_length; ++ii ) {
			for ( std::string const & atom_name : selected_fragStoreOP->fragment_specification.fragment_atoms ) {
				coordinates.push_back(pose.residue(resid+ii).xyz(atom_name));
			}
		}
		FragmentLookupResult lookupResults;
		if ( find_closest ) {
			lookupResults = selected_fragLookupOP->lookup_closest_fragment(&coordinates[0]);
		} else {
			lookupResults = selected_fragLookupOP->lookup_fragment(&coordinates[0]);
		}
		returnRmsd = lookupResults.match_rmsd;
	} else {
		TR.Debug << "SS not found in map!! given rms 5" << std::endl;
		//case where item is not found in map. ABEGO not found in pdb is bad sign. Give this a 999 rmsd value
		returnRmsd = 5;
	}
	return(returnRmsd);
}

void SSHashedFragmentStore::lookback_stub(std::vector< xyzVector<Real> > coordinates, char resTypeBeforeLoop,char resTypeAfterLoop, core::Size loop_length, numeric::Real & top_match_rmsd, utility::vector1<BackboneStub> & stubVector, core::Real stubRmsdThreshold){
	using namespace protocols::indexed_structure_store;
	top_match_rmsd= 9999;
	//core::Size tmp_match_index;
	std::map<std::string, vector1<core::Size> >::iterator iter;
	for ( iter=SS_stub_HashedFragmentStoreIndex_.begin(); iter != SS_stub_HashedFragmentStoreIndex_.end(); ++iter ) {
		Size numb_L = std::count(iter->first.begin(), iter->first.end(), 'L');
		if ( (iter->first[0]==resTypeBeforeLoop) && (numb_L ==loop_length) && (iter->first[iter->first.size()-1]==resTypeAfterLoop) ) {
			for ( core::Size ii=1; ii<=iter->second.size(); ++ii ) {
				FragmentStoreOP stub_fragStoreOP = SSHashedFragmentStore_[iter->second[ii]]->residue_subset_fragment_store;
				FragmentLookupOP stub_fragLookupOP = stub_fragStoreOP->get_fragmentLookup();
				std::vector<FragmentLookupResult> lookupResults = stub_fragLookupOP->lookup_close_fragments(&coordinates[0],stubRmsdThreshold);
				for ( core::Size jj=0; jj<lookupResults.size(); ++jj ) {
					Real tmp_rmsd = lookupResults[jj].match_rmsd;
					core::Size tmp_index = lookupResults[jj].match_index;
					core::Size match_ss_index = iter->second[ii];
					struct BackboneStub stub_tmp(tmp_rmsd,tmp_index,match_ss_index);
					stubVector.push_back(stub_tmp);
					if ( tmp_rmsd<top_match_rmsd ) {
						top_match_rmsd = tmp_rmsd;
					}
				}
			}
		}
	}
}


vector <bool> SSHashedFragmentStore::generate_subset_residues_to_compare(core::Size loop_length,core::Size fragment_length){
	vector<bool> tmp;
	core::Size front_ss_length = 2;
	core::Size back_ss_length = 2;
	if ( fragment_length>(front_ss_length+back_ss_length+loop_length) ) { //within tolerance add a residue to the front
		front_ss_length+=1;
	}
	if ( fragment_length>(front_ss_length+back_ss_length+loop_length) ) { //still within tolerance add a residue to the back
		back_ss_length+=1;
	}
	core::Size pos=0;
	for ( core::Size ii=0; ii<front_ss_length; ++ii ) {
		tmp.push_back(true);
		pos+=1;
	}
	for ( Size ii=0; ii<loop_length; ++ii ) {
		tmp.push_back(false);
		pos+=1;
	}
	for ( core::Size ii=0; ii<back_ss_length; ++ii ) {
		tmp.push_back(true);
		pos+=1;
	}
	while ( pos<fragment_length ) {
		tmp.push_back(false);
		pos+=1;
	}
	return(tmp);
}

/* No longer used
void SSHashedFragmentStore::lookback_uncached_stub(std::vector< xyzVector<Real> > coordinates, core::Size stub_match_ss_index, core::Size loop_length, Real & match_rmsd, core::Size & match_index){
//Note: the short_stub_ss is used to minimize the number of abego types to search.
using namespace protocols::indexed_structure_store;
Real low_rmsd = 9999;
typedef alignment::QCPKernel<Real> QCPKernel;

QCPKernel::CoordMap coord_map(&coordinates.front().x(), 3, coordinates.size());

FragmentStoreOP selected_fragStoreOP = SSHashedFragmentStore_[stub_match_ss_index];
core::Size fragment_length = get_fragment_length();
vector<bool> residues_to_compare = generate_subset_residues_to_compare(loop_length,fragment_length);
for ( core::Size jj=0; jj<selected_fragStoreOP->num_fragments_; ++jj ) {
std::vector< xyzVector<Real> > fragCoordinates;
for ( core::Size kk = 0;  kk < residues_to_compare.size(); ++kk ) {
if ( residues_to_compare[kk] ) {
fragCoordinates.push_back(selected_fragStoreOP->fragment_coordinates[jj*9+kk]);
}
}

QCPKernel::CoordMap frag_coord_map(&fragCoordinates.front().x(), 3, fragCoordinates.size());

Real tmp_rmsd = QCPKernel::calc_coordinate_rmsd(coord_map, frag_coord_map);
if ( tmp_rmsd<low_rmsd ) {
low_rmsd = tmp_rmsd;
match_index = jj;
match_rmsd = tmp_rmsd;
}
}
}
*/


std::vector< xyzVector<Real> > SSHashedFragmentStore::get_fragment_coordinates(core::Size db_index,core::Size match_index){
	FragmentStoreOP selected_fragStoreOP = SSHashedFragmentStore_.at(db_index);
	return(selected_fragStoreOP->get_fragment_coordinates(match_index));
}

// std::vector< xyzVector<Real> > SSHashedFragmentStore::lookback_xyz(pose::Pose const & pose, core::Size resid){
//  using namespace protocols::indexed_structure_store;
//  core::sequence::ABEGOManager AM;
//  typedef xyzVector<Real> Vec;
//  utility::vector1< std::string > abegoSeq = AM.get_symbols( pose,1 );//1 stands for class of ABEGO strings
//  core::Size fragmentStore_fragment_length = SSHashedFragmentStore_.begin()->second->fragment_specification.fragment_length;
//  std::string fragAbegoStr = "";
//  for ( core::Size ii=0; ii<fragmentStore_fragment_length; ++ii ) {
//   fragAbegoStr += abegoSeq[resid+ii];
//  }
//  core::Size base5index = AM.symbolString2base5index(fragAbegoStr);
//  TR.Debug << "fragAbegoStr:" << fragAbegoStr << std::endl;
//  if ( SSHashedFragmentStore_.find(base5index) == SSHashedFragmentStore_.end() ) {
//   utility_exit_with_message("ABEGO not found in map. FAILURE");
//  }
//  //case where item is found in map;
//  FragmentStoreOP selected_fragStoreOP = SSHashedFragmentStore_.at(base5index);
//  FragmentLookupOP selected_fragLookupOP = selected_fragStoreOP->get_fragmentLookup();
//  std::vector< xyzVector<Real> > coordinates;
//  std::vector< xyzVector<Real> > fragCoordinates;
//  for ( core::Size ii = 0;  ii < fragmentStore_fragment_length; ++ii ) {
//   for ( std::string const & atom_name : selected_fragStoreOP->fragment_specification.fragment_atoms ) {
//    coordinates.push_back(pose.residue(resid+ii).xyz(atom_name));
//   }
//  }
//  FragmentLookupResult lookupResult = selected_fragLookupOP->lookup_closest_fragment(&coordinates[0]);
//  for ( core::Size ii = 0;  ii < fragmentStore_fragment_length; ++ii ) {
//   Real xTmp = selected_fragStoreOP->fragment_coordinates[lookupResult.match_index+ii].x();
//   Real yTmp = selected_fragStoreOP->fragment_coordinates[lookupResult.match_index+ii].y();
//   Real zTmp = selected_fragStoreOP->fragment_coordinates[lookupResult.match_index+ii].z();
//   fragCoordinates.push_back(Vec(xTmp,yTmp,zTmp));
//  }
//  return(fragCoordinates);
// }



// vector<FragmentLookupResult> SSHashedFragmentStore::get_N_fragments(std::string abego_string,core::Size topNFrags){
//  //get number of fragments
//  FragmentStoreOP selected_fragStoreOP = get_fragment_store(abego_string);
//  vector<core::Size> chosen_fragments;
//  vector<FragmentLookupResult> lookupResults;
//  //core::Size random_index = random::rg().random_range(0,selected_fragStoreOP->num_fragments_);
//  for ( core::Size ii=0; ii<topNFrags && ii<selected_fragStoreOP->num_fragments_; ++ii ) {
//   core::Size random_frag = random::rg().random_range(0,selected_fragStoreOP->num_fragments_);
//   while ( std::find(chosen_fragments.begin(),chosen_fragments.end(),random_frag)!= chosen_fragments.end() )
//     random_frag = random::rg().random_range(0,selected_fragStoreOP->num_fragments_);
//   chosen_fragments.push_back(random_frag);
//  }
//  for ( core::Size ii=0; ii<chosen_fragments.size(); ++ii ) {
//   FragmentLookupResult tmpFragInfo;
//   tmpFragInfo.match_index = chosen_fragments[ii];
//   lookupResults.push_back(tmpFragInfo);
//  }
//  return(lookupResults);
// }

// struct less_then_match_rmsd
// {
//  inline bool operator() (const FragmentLookupResult& struct1, const FragmentLookupResult& struct2)
//  {
//   return (struct1.match_rmsd < struct2.match_rmsd);
//  }
// };


// vector<FragmentLookupResult> SSHashedFragmentStore::get_topN_fragments(std::string /*selectionType*/,core::Size topNFrags, pose::Pose const & pose, core::Size resid,Real rms_threshold,std::string fragAbegoStr){
//  vector<FragmentLookupResult> lookupResults = get_fragments_below_rms(pose,resid,rms_threshold,fragAbegoStr);
//  //sort array based on rms
//  std::sort(lookupResults.begin(), lookupResults.end(),less_then_match_rmsd());
//  vector<FragmentLookupResult> topLookupResults;
//  for ( int ii=0; ii<(int)topNFrags; ++ii ) {
//   topLookupResults.push_back(lookupResults[ii]);
//  }
//  return(topLookupResults);
// }

void  SSHashedFragmentStore::get_hits_below_rms(core::pose::Pose const & pose, core::Size resid, Real rms_threshold, vector1<vector<Real> > & hits_cen, vector1<Real> & hits_rms, vector1<std::string> & hits_aa){
	//get residue coordinates
	core::sequence::SSManager SM;
	//---------------------------------------------------------------------
	std::vector< xyzVector<Real> > coordinates;
	FragmentStoreOP tmp_fragStoreOP = get_fragment_store();
	for ( core::Size ii = 0;  ii < get_fragment_length(); ++ii ) {
		for ( std::string const & atom_name : tmp_fragStoreOP->fragment_specification.fragment_atoms ) {
			coordinates.push_back(pose.residue(resid+ii).xyz(atom_name));
		}
	}
	//get frag dssp
	core::scoring::dssp::Dssp dssp( pose );
	dssp.dssp_reduced();
	std::string dssp_string = dssp.get_dssp_secstruct();
	core::Size fragment_length = SSHashedFragmentStore_.begin()->second->fragment_specification.fragment_length;
	std::string frag_ss = dssp_string.substr(resid-1,fragment_length);
	//generate valid SS types while removing all helical
	set<std::string> valid_frag_ss = potential_valid_ss_strings(frag_ss);
	set<std::string>::iterator iter;
	if ( frag_ss != "HHHHHHHHH" ) {
		iter = valid_frag_ss.find("HHHHHHHHH");
		if ( iter!=valid_frag_ss.end() ) {
			valid_frag_ss.erase(iter);
		}
	}
	//loop through valid SS and collect fragments
	for ( iter=valid_frag_ss.begin(); iter!=valid_frag_ss.end(); ++iter ) {
		core::Size ss_index = SM.symbolString2index(*iter);
		if ( SSHashedFragmentStore_.find(ss_index)!= SSHashedFragmentStore_.end() ) {
			FragmentStoreOP selected_fragStoreOP = SSHashedFragmentStore_.at(ss_index);
			FragmentLookupOP selected_fragLookupOP = selected_fragStoreOP->get_fragmentLookup();
			vector<FragmentLookupResult> lookupResults = selected_fragLookupOP->lookup_close_fragments(&coordinates[0], rms_threshold);
			for ( auto & lookupResult : lookupResults ) {
				hits_rms.push_back(lookupResult.match_rmsd);
				std::vector<Real> cen_list_frag = selected_fragStoreOP->realVector_groups["cen"][lookupResult.match_index];
				hits_cen.push_back(cen_list_frag);
				std::string tmp_aa = selected_fragStoreOP->string_groups["aa"][lookupResult.match_index];
				hits_aa.push_back(tmp_aa);
			}
		}
	}
}



FragmentStoreOP SSHashedFragmentStore::get_fragment_store(core::Size db_index){
	if ( SSHashedFragmentStore_.find(db_index) != SSHashedFragmentStore_.end() ) {
		return(SSHashedFragmentStore_.at(db_index));
	}
	return(nullptr);
}

FragmentStoreOP SSHashedFragmentStore::get_fragment_store(){
	return(SSHashedFragmentStore_.begin()->second);
}

std::map<core::Size, protocols::indexed_structure_store::FragmentStoreOP> SSHashedFragmentStore::get_hashed_fragment_store(){
	return(SSHashedFragmentStore_);
}


Size SSHashedFragmentStore::get_fragment_length(){
	using namespace protocols::indexed_structure_store;
	core::Size fragmentStore_fragment_length = SSHashedFragmentStore_.begin()->second->fragment_specification.fragment_length;
	return(fragmentStore_fragment_length);
}

}
}

