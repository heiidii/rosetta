
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

#include <protocols/toolbox/KCluster.hh>

#include <core/init/init.hh>
#include <basic/options/option.hh>
#include <basic/options/option_macros.hh>
#include <basic/options/keys/in.OptionKeys.gen.hh>
#include <basic/options/keys/out.OptionKeys.gen.hh>
#include <basic/options/keys/cluster.OptionKeys.gen.hh>
#include <basic/Tracer.hh>

#include <core/pose/Pose.hh>
#include <core/io/pdb/pose_io.hh>
#include <core/pose/util.hh>
#include <core/import_pose/import_pose.hh>
#include <core/import_pose/pose_stream/PoseInputStream.fwd.hh>
#include <core/import_pose/pose_stream/SilentFilePoseInputStream.hh>
#include <core/id/AtomID.hh>

#include <core/io/silent/SilentFileData.hh>
#include <core/io/silent/SilentStruct.hh>
#include <core/io/silent/ProteinSilentStruct.hh>
#include <core/io/silent/SilentStructFactory.hh>

#include <core/scoring/Energies.hh>
#include <core/scoring/EnergyGraph.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/ScoreFunctionFactory.hh>
#include <core/conformation/Residue.hh>
#include <protocols/docking/metrics.hh>

#include <utility/exit.hh>
#include <utility/vector1.hh>
#include <utility/tools/make_vector1.hh>
#include <ObjexxFCL/FArray2D.hh>

#include <iostream>

#include <protocols/cluster/cluster.hh>
#include <core/scoring/rms_util.hh>

#include <basic/options/option_macros.hh>
#include <map>
#include <sstream>
#include <iomanip>

//use TM align for sequence independent
#include <protocols/hybridization/TMalign.hh>
#include <boost/lexical_cast.hpp>

//using boost threads
#ifdef USE_BOOST_THREAD
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#endif

using namespace core;
using namespace basic::options;
using namespace basic::options::OptionKeys;
using namespace protocols::toolbox;
using namespace core::io::silent;
//using namespace protocols::moves::mc_convergence_checks;

OPT_1GRP_KEY(Real,bb_cluster,cluster_radius)

static thread_local basic::Tracer TR( "bb_cluster" );

void do_align( core::pose::Pose const &pose1, core::pose::Pose const &pose2, core::Real &rms) {
	//core::scoring::ScoreFunctionOP scorefxn = core::scoring::get_score_function();
        rms=core::scoring::bb_rmsd(pose1,pose2);
}


int main(int argc, char *argv[])
{
  try {
	NEW_OPT(bb_cluster::cluster_radius, "radius for cluster",0.8);

	core::init::init(argc, argv);

	//read a list of pdbs
	Size input_pdb_size=basic::options::option[ basic::options::OptionKeys::in::file::s ]().size();
	assert(input_pdb_size>1);
	utility::vector1< core::pose::PoseOP > PoseVec;
	core::pose::PoseOP pose;
        for ( Size i = 1; i <= input_pdb_size; ++i ) {
		std::string pdbnamei=basic::options::option[ basic::options::OptionKeys::in::file::s ]()[i];
//		TR << pdbnamei << endl;
		pose = core::pose::PoseOP( new core::pose::Pose );
		core::import_pose::pose_from_pdb( *pose, pdbnamei.c_str() );
		PoseVec.push_back(pose);
	}

	//std::vector< core::Real > ddglist;
	//std::vector< std::string > taglist;
	//std::vector< std::string > outtaglist;
	//utility::vector1< std::string > outtaglist;
        //std::string scorecolumn=basic::options::option[ basic::options::OptionKeys::bb_cluster::column];
        //core::scoring::ScoreFunctionOP sfxn = core::scoring::get_score_function();
	//Real score;

	TR.Info << "PoseVec.size(): "<< PoseVec.size() << std::endl;

	//compute similarity matrix
	//use boost threads frank/rr_opt.cc
	//protocols/frag_picker/FragmentPicker.cc
        ObjexxFCL::FArray2D< core::Real > sc_matrix( PoseVec.size(),PoseVec.size(), 0.0 );
	#ifdef USE_BOOST_THREAD
	TR << " Running multi-thread version " << endl;
	boost::thread_group threads;
	for ( Size i = 1; i <= PoseVec.size(); ++i ) {
    		for ( Size j = i ; j <= PoseVec.size(); ++j ) {
			TR << "i " << i << " j " << j << " ";
			//boost::thread*  threadij = new boost::thread( &do_align, boost::cref(*PoseVec[i]), boost::cref(*PoseVec[j]), boost::ref(sc_matrix(i,j)));
			//threads.add_thread(threadij);
			threads.create_thread( boost::bind( &do_align, boost::cref(*PoseVec[i]), boost::cref(*PoseVec[j]), boost::ref(sc_matrix(i,j)) )) ;
			TR << "thread created"<< endl;
		}
	}
        threads.join_all();
	#else
	//TR << " use extras=boost_thread to compile boost version, seg fault, why? " << endl;
        for ( Size i = 1; i <= PoseVec.size(); ++i ) {
                for ( Size j = i+1 ; j <= PoseVec.size(); ++j ) {
                        do_align(*PoseVec[i], *PoseVec[j], sc_matrix(i,j));
						TR << "rms("<<i<<","<<j<<"): " << sc_matrix(i,j) << std::endl;
                }
        }
	//std::cerr << "compile with extras=boost_thread-mt!" << std::endl;
	#endif

	/*
	//print similarity matrix
	for ( Size i = 1; i <= PoseVec.size(); ++i ) {
	    for ( Size j = 1 ; j <= PoseVec.size(); ++j ) {
			TR.Info << sc_matrix(i,j) << " ";
		}
		TR.Info << std::endl;
	}
	*/

	//assign the other half of the matrix
	for ( Size i = 1; i <= PoseVec.size(); ++i ) {
	    for ( Size j = 1 ; j < i; ++j ) {
			sc_matrix(i,j)=sc_matrix(j,i);
		}
	}

	//print similarity matrix
	for ( Size i = 1; i <= PoseVec.size(); ++i ) {
	    for ( Size j = 1 ; j <= PoseVec.size(); ++j ) {
			TR.Info << sc_matrix(i,j) << " ";
		}
		TR.Info << std::endl;
	}

	//cluster similarity matrix
	//what algorithms will you use?
	//the one in Rosetta
	core::Real cluster_radius_=basic::options::option[ basic::options::OptionKeys::bb_cluster::cluster_radius];
	core::Size listsize=PoseVec.size();
	std::vector < int > neighbors ( PoseVec.size(), 0 );
	std::vector < int > clusternr ( PoseVec.size(), -1 );
	std::vector < int > clustercenter;
	core::Size mostneighbors;
	core::Size nclusters=0;
	core::Size i,j;
	std::vector <int> clustercentre;

        TR.Info << "Clustering of " << listsize << " structures with radius (bbrms) " <<  cluster_radius_ <<  std::endl;

  	// now assign groupings
  	while(true) {
    	// count each's neighbors
        for (i=0;i<listsize;++i ) {
          neighbors[i] = 0;
          if (clusternr[i]>=0) continue; // ignore ones already taken
          for (j=0;j<listsize;++j ) {
            if (clusternr[j]>=0) continue; // ignore ones already taken
	    //TR.Info << "sc_matrix("<<i+1<<","<<j+1<<") = " << sc_matrix( i+1, j+1 ) << std::endl;
            if ( sc_matrix( i+1, j+1 ) < cluster_radius_ ) neighbors[i]++;
          }
	  TR.Info << "i: " << i << " " << neighbors[i] << std::endl;
        }

        mostneighbors = 0;
        for (i=0;i<listsize;++i ) {
          if (neighbors[i]>neighbors[mostneighbors]) {
		mostneighbors=i;
		}
        }
	TR.Info << "mostneighbors =" << mostneighbors <<std::endl;
        if (neighbors[mostneighbors] <= 0) break;  // finished!

	for (i=0;i<listsize;++i ) {
      		if (clusternr[i]>=0) continue; // ignore ones already taken
      		if ( sc_matrix( i+1, mostneighbors+1 ) < cluster_radius_) {
        		clusternr[i] = mostneighbors;
      		}
    	}

    	clustercentre.push_back(mostneighbors);
    	nclusters++;

  	}

	TR.Info << "ncluster: " << nclusters << std::endl;

	for (i=0;i<clustercentre.size();++i ) {
		TR.Info << "CLUSTER " << i << " : ";
		//TR.Info << "CLUSTER " << i << ", " << clustercentre[i] <<" : ";
    		for (j=0;j<listsize;++j ) {
      			if (clusternr[j] == clustercentre[i]) {
				//TR.Info << j << " " ;
				//std::string tag( tag_from_pose( *PoseVec[j+1] ) );
				std::string tag=basic::options::option[ basic::options::OptionKeys::in::file::s ]()[j+1];
				std::string fn( "c_"+boost::lexical_cast<std::string>(i)+"_" + boost::lexical_cast<std::string>(j) +"_" + tag );
				core::scoring::calpha_superimpose_pose(*PoseVec[j+1],*PoseVec[clustercentre[i]+1]);
				PoseVec[j+1]->dump_pdb(fn);
			}
		}
		TR.Info << std::endl;
	}

	TR.Info << "done bb_cluster" << std::endl;

  } catch ( utility::excn::EXCN_Base const & e ) {
		std::cout << "caught exception " << e.msg() << std::endl;
		return -1;
  }
}
