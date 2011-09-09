// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
// :noTabs=false:tabSize=4:indentSize=4:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   rosetta/benchmark/pdb_io.bench.cc
///
/// @brief  Performance benchmark for PDB input and output
/// @author Matthew O'Meara

#include "benchmark.hh"

#include <core/pose/Pose.hh>
#include <core/import_pose/import_pose.hh>

#include <utility/io/izstream.hh>
#include <iostream>
#include <fstream>

using namespace core;

class PDB_IOBenchmark : public Benchmark
{
public:
	pose::Pose pose;

	PDB_IOBenchmark(std::string name) : Benchmark(name) {};

	virtual void setUp() {

		std::ifstream pdb("test_in.pdb");

		pdb.seekg(0, std::ios::end);
		Size length = pdb.tellg();
		pdb.seekg(0, std::ios::beg);

		pdb_string_.resize(length);
		pdb.read(&pdb_string_[0], length);
	}

	virtual void run(core::Real scaleFactor) {

		for(int i=0; i<100*scaleFactor; i++) {
			core::import_pose::pose_from_pdbstring(pose, pdb_string_);
		}
	};

	virtual void tearDown() {};

	std::string pdb_string_;
};
