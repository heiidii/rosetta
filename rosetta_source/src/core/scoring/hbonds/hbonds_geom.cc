// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file
/// @brief
/// @author

// Unit headers
#include <core/scoring/hbonds/hbonds_geom.hh>

// Package headers
#include <core/scoring/hbonds/types.hh>
#include <core/scoring/hbonds/constants.hh>
#include <core/scoring/hbonds/FadeInterval.hh>
#include <core/scoring/hbonds/HBEvalTuple.hh>
#include <core/scoring/hbonds/HBondDatabase.hh>
#include <core/scoring/hbonds/HBondTypeManager.hh>
#include <core/scoring/hbonds/polynomial.hh>
#include <core/scoring/DerivVectorPair.hh>

// Project headers
#include <core/conformation/Residue.hh>
#include <basic/Tracer.hh>

// Numeric headers
#include <numeric/constants.hh>
#include <numeric/conversions.hh>
#include <numeric/deriv/distance_deriv.hh>
#include <numeric/deriv/angle_deriv.hh>
#include <numeric/deriv/dihedral_deriv.hh>

// ObjexxFCL Headers
#include <ObjexxFCL/format.hh>

// C++ Headers
#include <cmath>
#include <iostream>


// option key includes
#include <basic/options/option.hh>
#include <basic/options/keys/OptionKeys.hh>
#include <basic/options/keys/in.OptionKeys.gen.hh>
// AUTO-REMOVED #include <basic/options/keys/score.OptionKeys.gen.hh>
#include <basic/options/keys/corrections.OptionKeys.gen.hh>

// Numeric Headers
#include <numeric/numeric.functions.hh>
#include <numeric/xyz.functions.hh>

//Exception for NaN in hbonds.
#include <utility/excn/Exceptions.hh>

//Utility Headers
#include <utility/basic_sys_util.hh>
#include <utility/PyAssert.hh>

#include <core/chemical/AtomType.hh>
#include <core/chemical/AtomTypeSet.hh>
#include <core/scoring/hbonds/HBondOptions.hh>
#include <core/scoring/hbonds/hbtrie/HBAtom.hh>
#include <utility/vector1.hh>
#include <ObjexxFCL/FArray3D.hh>

namespace ObjexxFCL { namespace fmt { } } using namespace ObjexxFCL::fmt;

namespace core {
namespace scoring {
namespace hbonds {

static basic::Tracer tr("core.scoring.hbonds");

Real DUMMY_DERIV(0.0);
bool DUMMY_BOOL(false);
HBGeoDimType DUMMY_HBGEODIMTYPE(hbgd_NONE);
HBondDerivs DUMMY_DERIVS;
HBondDerivs const ZERO_DERIV2D = { DerivVectorPair(), DerivVectorPair(), DerivVectorPair(), DerivVectorPair(), DerivVectorPair() };


///////////////////////////////////////////////////////////////////////////////
HBDonChemType
get_hb_don_chem_type(
	int const datm,
	conformation::Residue const & don_rsd
)
{
	using namespace chemical;
	std::string const & aname(don_rsd.atom_name(datm)); // NEVER create a string when a string const & will do
	if (don_rsd.atom_is_backbone(datm)){
		if (don_rsd.is_protein()) {
			if(don_rsd.is_lower_terminus()){

				/// WARNING this is set to hbdon_PBA for backwards compatibility only!!!!
				/// but it should be hbdon_AMO because it is actually an amino group
				return hbdon_PBA;
			} else {
				// should backbone prolines be a separate type?
				return hbdon_PBA;
			}
		} else {
			//tr << "WARNING: Unknown Hydrogen Bond donor type for: " + don_rsd.name1() + I(3, don_rsd.seqpos()) + " " + don_rsd.atom_name( datm) + ".  Using hbdon_GENERIC_BB.";
			return hbdon_GENERIC_BB;
		}
	} else {
		switch(don_rsd.aa()){
		case aa_asn: case aa_gln: return hbdon_CXA; break;
		case aa_his:
			if (aname == " ND1"){
				return hbdon_IMD;
			} else {
				assert( aname == " NE2");
				return hbdon_IME;
			} break;
		case aa_trp:
			return hbdon_IND; break;
		case aa_lys:
			return hbdon_AMO; break;
		case aa_arg:
			if (aname == " NE "){
				return hbdon_GDE;
			} else {
				assert(aname == " NH1" || aname == " NH2");
				return hbdon_GDH;
			} break;
		case aa_tyr:
			return hbdon_AHX; break;
		case aa_ser:
		case aa_thr:
			return hbdon_HXL; break;
		case aa_ala: case aa_cys: case aa_asp: case aa_glu: case aa_phe:
		case aa_gly: case aa_ile: case aa_leu: case aa_met: case aa_pro: case aa_val:
			return hbdon_NONE; break;
		case na_ade:
			if (aname == " N6 ") {
				/// WARNING this is set to hbdon_GENERIC_SC for backwards compatibility only!!!
				/// it should actually be sidechain hbdon_CXA.
				return hbdon_CXA;
			} else if (aname == "WN6" || aname == "WN7" || aname == "WN3") { // DNA_MAJOR_GROOVE_WATER ADDUCTS
				return hbdon_H2O;
			} break;
		case na_cyt:
			if (aname == " N4 ") {
				/// WARNING this is set to hbdon_GENERIC_SC for backwards compatibility only!!!
				/// it should actually be sidechain hbdon_CXA.
				return hbdon_CXA;
		//} else if ( aname == " N1 ") {  // while it is an Ntrp it is not protonated to it doesn't donate
		//		return hbdon_IND;
			} else if ( aname == "WN4" || aname == "WO2" ) {
				return hbdon_H2O; // DNA_MAJOR_GROOVE_WATER ADDUCTS
			} break;
		case na_gua:
			if (aname == " N1 ") {
				/// WARNING this is set to hbdon_GENERIC_SC for backwards compatibility only!!!
				/// it should actually be sidechain hbdon_IND.
				return hbdon_IND;
			} else if (aname == " N2 ") {
				/// WARNING this is set to hbdon_GENERIC_SC for backwards compatibility only!!!
				/// it should actually be sidechain hbdon_CXA.
				return hbdon_CXA;
			} else if ( aname == "WO6" || aname == "WN7" || aname == "WN3" ){
				return hbdon_H2O; // DNA_MAJOR_GROOVE_WATER ADDUCT
			} break;
		case na_thy:
			if (aname == " N3 ") {
				/// WARNING this is set to hbdon_GENERIC_SC for backwards compatibility only!!!
				/// it should actually be sidechain hbdon_IND.
				return hbdon_IND;
			} else if ( aname == "WO4" || aname == "WO2" ) {
				return hbdon_H2O; // DNA_MAJOR_GROOVE_WATER ADDUCTS
			} break;
		case na_rad:
			if (aname == " N6 ") {
				/// WARNING this is set to hbdon_GENERIC_SC for backwards compatibility only!!!
				/// it should actually be sidechain hbdon_CXA.
				return hbdon_GENERIC_SC;
			} else if ( aname == "WN6" || aname == "WN7" ){
				return hbdon_H2O; // DNA_MAJOR_GROOVE_WATER ADDUCTS
			} else if ( aname == " O2*" ){
				///. WARNING this is set to hbdon_GENERIC_SC for backwards compatibility only!!!
				/// it should actually be sidechain hbdon_HXL
				return hbdon_GENERIC_SC;
			} else if ( aname == " N1 " &&  don_rsd.has_variant_type("PROTONATED_H1_ADENOSINE") ) { //Parin Sripakdeevong May 03, 2011.
				return hbdon_GENERIC_SC;
			} break;
		case na_rgu:
			if (aname == " N1 ") {
				/// WARNING this is set to hbdon_GENERIC_SC for backwards compatibility only!!!
				/// it should actually be sidechain hbdon_IND.
				return hbdon_GENERIC_SC;
			} else if (aname == " N2 ") {
				/// WARNING this is set to hbdon_GENERIC_SC for backwards compatibility only!!!
				/// it should actually be sidechain hbdon_CXA.
				return hbdon_GENERIC_SC;
			} else if ( aname == " O2*" ){
				///. WARNING this is set to hbdon_GENERIC_SC for backwards compatibility only!!!
				/// it should actually be sidechain hbdon_HXL
				return hbdon_GENERIC_SC;
			} else if ( aname == "WO6" || aname == "WN7"){
				return hbdon_H2O; // DNA_MAJOR_GROOVE_WATER ADDUCT
			} break;
		case na_rcy:
			if (aname == " N4 ") {
				/// WARNING this is set to hbdon_GENERIC_SC for backwards compatibility only!!!
				/// it should actually be sidechain hbdon_CXA.
				return hbdon_GENERIC_SC;
			} else if ( aname == " O2*" ){
				///. WARNING this is set to hbdon_GENERIC_SC for backwards compatibility only!!!
				/// it should actually be sidechain hbdon_HXL
				return hbdon_GENERIC_SC;
			} else if ( aname == "WN4" ){
				return hbdon_H2O; // DNA_MAJOR_GROOVE_WATER ADDUCT
			} break;
		case na_ura:
			if (aname == " N3 ") {
				/// WARNING this is set to hbdon_GENERIC_SC for backwards compatibility only!!!
				/// it should actually be sidechain hbdon_IND.
				return hbdon_GENERIC_SC;
			} else if ( aname == " O2*" ){
				///. WARNING this is set to hbdon_GENERIC_SC for backwards compatibility only!!!
				/// it should actually be sidechain hbdon_HXL
				return hbdon_GENERIC_SC;
			} else if ( aname == "WO4" ){
				return hbdon_H2O; // DNA_MAJOR_GROOVE_WATER ADDUCT
			} break;
		case aa_vrt:
		case aa_unk:
			//tr << "WARNING: Unknown Hydrogen Bond donor type for: " + don_rsd.name1() + I(3, don_rsd.seqpos()) + " " + don_rsd.atom_name( datm) + ".  Using hbdon_GENERIC_SC.";
			return hbdon_GENERIC_SC; break;
		}
	}
	utility_exit_with_message( "ERROR: Unknown Hydrogen Bond donor type for: " + don_rsd.name1() + I(3, don_rsd.seqpos()) + " " + don_rsd.atom_name( datm) + ".  Using hbdon_GENERIC_SC.");
	return hbdon_NONE;
}

///////////////////////////////////////////////////////////////////////////////
HBAccChemType
get_hb_acc_chem_type(
	int const aatm,
	conformation::Residue const & acc_rsd
)
{
	using namespace chemical;
	std::string const & aname(acc_rsd.atom_name(aatm)); // NEVER create a string when a string const & will do

	if( acc_rsd.atom_is_backbone(aatm)){
		if( acc_rsd.is_protein() ) {
			if(acc_rsd.is_upper_terminus()){

				/// WARNING this is set to hbacc_PBA for backwards compatibility!!!!
				/// but it should be hbacc_CXL because it is actually a carboxyl group
				return hbacc_PBA;
			} else {
				return hbacc_PBA;
			}
		} else if (acc_rsd.is_DNA()){
			if (aname == " O1P" || aname == " O2P" ){
				return hbacc_PCA_DNA;
			} else if (aname == " O5*" || aname == " O3*"){
				return hbacc_PES_DNA;
			} else if (aname == " O4*"){
				return hbacc_RRI_DNA;
			}
		}else if ( acc_rsd.is_RNA() ){
			if (aname == " O1P" || aname == " O2P" || aname == " O3P"){
				return hbacc_PCA_RNA;
			} else if (aname == " O5*" || aname == " O3*"){
				return hbacc_PES_RNA;
			} else if (aname == " O4*"){
				return hbacc_RRI_RNA;
			}
		} else {
			// generic types; for backwards compatibility; prefer functional group based chem type
			switch (acc_rsd.atom_type(aatm).hybridization()){
			case SP2_HYBRID:
				//				tr << "WARNING: Unknown hydrogen bond acceptor type for: " + acc_rsd.name1() + I(3, acc_rsd.seqpos()) + " " + acc_rsd.atom_name(aatm) + ".  Using hbdon_GENERIC_SP2BB.";
				return hbacc_GENERIC_SP2BB; break;
			case SP3_HYBRID:
				//tr << "WARNING: Unknown hydrogen bond acceptor type for: " + acc_rsd.name1() + I(3, acc_rsd.seqpos()) + " " + acc_rsd.atom_name(aatm) + ".  Using hbdon_GENERIC_SP3BB.";
				return hbacc_GENERIC_SP3BB; break;
			case RING_HYBRID:
				//tr << "WARNING: Unknown hydrogen bond acceptor type for: " + acc_rsd.name1() + I(3, acc_rsd.seqpos()) + " " + acc_rsd.atom_name(aatm) + ".  Using hbdon_GENERIC_RINGBB.";
				return hbacc_GENERIC_RINGBB; break;
			case UNKNOWN_HYBRID:
				//tr << "WARNING: Unknown hydrogen bond acceptor type for: " + acc_rsd.name1() + I(3, acc_rsd.seqpos()) + " " + acc_rsd.atom_name(aatm) + ".  Using hbdon_GENERIC_RINGBB.";
				return hbacc_NONE; break;
			}
		}
	} else {
		switch(acc_rsd.aa()){
		case aa_asn: case aa_gln: return hbacc_CXA; break;
		case aa_asp: case aa_glu: return hbacc_CXL; break;
		case aa_his:
			if (aname == " ND1"){
				return hbacc_IMD;
			} else {
				return hbacc_IME;
			} break;
		case aa_ala: case aa_cys: case aa_phe: case aa_gly: case aa_ile: case aa_leu:
		case aa_met: case aa_pro: case aa_val: case aa_tyr: return hbacc_AHX; break;
		case aa_ser: case aa_thr: return hbacc_HXL; break;
		case aa_lys: case aa_arg: case aa_trp:
			return hbacc_NONE;
		case na_ade:
			if (aname == " N1 " || aname == " N3 " || aname == " N7 "){
				/// WARNING this is set to hbacc_GENERIC_RINGSC for backwards compatibility only!!!
				/// it should actually be sidechain hbacc_IME.
				return hbacc_IME;
			} else if (aname == "WN6" || aname == "WN7" || aname == "WN3") {
				return hbacc_H2O; // DNA_MAJOR_GROOVE_WATER ADDUCTS
			} break;
		case na_gua:
			if (aname == " N3 " || aname == " N7 "){
				/// WARNING this is set to hbacc_GENERIC_RINGSC for backwards compatibility only!!!
				/// it should actually be sidechain hbacc_IME.
				return hbacc_IME;
			} else if ( aname == " O6 "){
				return hbacc_CXL;
			} else if ( aname == "WO6" || aname == "WN7" || aname == "WN3" ) {
				return hbacc_H2O; // DNA_MAJOR_GROOVE_WATER ADDUCTS
			} break;
		case na_cyt:
			if (aname == " O2 "){
				/// WARNING this is set to hbacc_GENERIC_SP2SC for backwards compatibility only!!!
				/// it should actually be sidechain hbacc_CXA.
				return hbacc_CXA;
			} else if (aname == " N3 "){
				/// WARNING this is set to hbacc_GENERIC_RINGSC for backwards compatibility only!!!
				/// it should actually be sidechain hbacc_IME.
				return hbacc_IME;
			} else if ( aname == "WN4" || aname == "WO2" ) {
				return hbacc_H2O; // DNA_MAJOR_GROOVE_WATER ADDUCTS
			} break;
		case na_thy:
			if (aname == " O2 " || aname == " O4 "){
				/// WARNING this is set to hbacc_GENERIC_SP2SC for backwards compatibility only!!!
				/// it should actually be sidechain hbacc_CXA.
				return hbacc_CXA;
			} else if ( aname == "WO4" || aname == "WO2" ) {
				return hbacc_H2O; // DNA_MAJOR_GROOVE_WATER ADDUCTS
			} break;
		case na_rad:
			if (aname == " N1 " || aname == " N3 " || aname == " N7 "){
				if(aname == " N1 "){
				 	if(acc_rsd.has_variant_type("PROTONATED_H1_ADENOSINE")) utility_exit_with_message("acc_rsd.aa()==na_rad, aname == \" N1 \" and acc_rsd.has_variant_type(\"PROTONATED_H1_ADENOSINE\")!");
				}
				/// WARNING this is set to hbacc_GENERIC_RINGSC for backwards compatibility only!!!
				/// it should actually be sidechain hbacc_IME.
				return hbacc_GENERIC_RINGSC;
			} else if (aname == " O2*") {
				/// WARNING this is set to hbacc_GENERIC_SP3BB for backwards compatibility only!!!
				/// it should actually be backbone hbacc_HXL.
				return hbacc_GENERIC_SP3SC;
			} break;
		case na_rgu:
			if (aname == " N3 " || aname == " N7 "){
				/// WARNING this is set to hbacc_GENERIC_RINGSC for backwards compatibility only!!!
				/// it should actually be sidechain hbacc_IME.
				return hbacc_GENERIC_RINGSC;
			} else if (aname == " O6 "){
				/// WARNING this is set to hbacc_GENERIC_RINGSC for backwards compatibility only!!!
				/// it should actually be sidechain hbacc_CXA.
				return hbacc_GENERIC_SP2SC;
			} else if (aname == " O2*") {
				/// WARNING this is set to hbacc_GENERIC_SP3BB for backwards compatibility only!!!
				/// it should actually be backbone hbacc_HXL.
				return hbacc_GENERIC_SP3SC;
			} break;
		case na_rcy:
			if (aname == " O2 "){
				/// WARNING this is set to hbacc_GENERIC_SP2SC for backwards compatibility only!!!
				/// it should actually be sidechain hbacc_CXA.
				return hbacc_GENERIC_SP2SC;
			} else if (aname == " N3 "){
				/// WARNING this is set to hbacc_GENERIC_RINGSC for backwards compatibility only!!!
				/// it should actually be sidechain hbacc_IME.
				return hbacc_GENERIC_RINGSC;
			} else if (aname == " O2*") {
				/// WARNING this is set to hbacc_GENERIC_SP3BB for backwards compatibility only!!!
				/// it should actually be backbone hbacc_HXL.
				return hbacc_GENERIC_SP3SC;
			} break;
		case na_ura:
			if (aname == " O2 " || aname == " O4 "){
				/// WARNING this is set to hbacc_GENERIC_SP2SC for backwards compatibility only!!!
				/// it should actually be sidechain hbacc_CXA.
				return hbacc_GENERIC_SP2SC;
			} else if (aname == " O2*") {
				/// WARNING this is set to hbacc_GENERIC_SP3BB for backwards compatibility only!!!
				/// it should actually be backbone hbacc_HXL.
				return hbacc_GENERIC_SP3SC;
			} break;
		case aa_vrt:
		case aa_unk:
			// generic types; for backwards compatibility; prefer functional group based chem type
			switch(acc_rsd.atom_type(aatm).hybridization()){
			case SP2_HYBRID:
				return hbacc_GENERIC_SP2SC; break;
			case SP3_HYBRID:
				return hbacc_GENERIC_SP3SC; break;
			case RING_HYBRID:
				return hbacc_GENERIC_RINGSC; break;
			case UNKNOWN_HYBRID:
				return hbacc_NONE; break;
			}
		}
	}
	utility_exit_with_message( "unknown Hydrogen Bond acceptor type for: " + acc_rsd.name1() + I(3,acc_rsd.seqpos()) + " " + acc_rsd.atom_name( aatm) );
	return hbacc_NONE;
}

// jk comment out inline so this can be used elsewhere (exact SHO model)
//inline
HBSeqSep
get_seq_sep(
	HBDonChemType const & don_chem_type,
	HBAccChemType const & acc_chem_type,
	int const & sep
){
	// The logic here is, if it is protein backbone to protein backbone
	// then identify secondary struture sterics by sequence separation.
	// Rhiju notices that down weighting hbonds between adjacent
	// residues where at least one is a backbone improved scientific
	// benchmarks in RNA.  Until more study is done, this is either
	// something RNA specific or an artifact kinimatic sampling, eg it
	// is easier to sample close hydrogen bonds than far hydrogen bonds.
	// If it is the latter then all adjacent BSC hbonds should be split out.
	// Nucleic acid backbone is unfortunately treated as backbone
	// only for the purposes of scoring and only when it is DNA with
	// something that is not protein or DNA or it is RNA.  Yeah it's
	// confusing.  So DNA backbone is for now treated similar to protein
	// sidechain for the purposes of sequence separation.

	// hbonds involving water are always seq_sep_other.

	// I'm not sure if this should code should be done like it is here
	// or if it should be folded into another ~500 cases of the
	// HBEval_lookup table.

	switch(don_chem_type){
	case hbdon_NONE:
	case hbdon_H2O:
		return seq_sep_other;
	case hbdon_PBA:
		switch(acc_chem_type){
		case hbacc_NONE:
		case hbacc_H2O:
			return seq_sep_other;
		case hbacc_PBA:
			switch(sep){
			case -4: return seq_sep_M4; break;
			case -3: return seq_sep_M3; break;
			case -2: return seq_sep_M2; break;
			case -1:
			case 1: return seq_sep_PM1; break;
			case 2: return seq_sep_P2; break;
			case 3: return seq_sep_P3; break;
			case 4: return seq_sep_P4; break;
			default: return seq_sep_other; break;
			}
		default:
			if (sep == 1 || sep == -1) { return seq_sep_PM1;}
			else { return seq_sep_other; }
			break;
		} break;
	case hbdon_CXA:
	case hbdon_IMD:
	case hbdon_IME:
	case hbdon_IND:
	case hbdon_AMO:
	case hbdon_GDE:
	case hbdon_GDH:
	case hbdon_AHX:
	case hbdon_HXL:
		switch(acc_chem_type){
		case hbacc_NONE:
		case hbacc_CXA:
		case hbacc_CXL:
		case hbacc_IMD:
		case hbacc_IME:
		case hbacc_AHX:
		case hbacc_HXL:
		case hbacc_PCA_DNA:
		case hbacc_PES_DNA:
		case hbacc_RRI_DNA:
		case hbacc_H2O:
			return seq_sep_other; break;
		default:
			if (sep == 1 || sep == -1) { return seq_sep_PM1;}
			else { return seq_sep_other; } break;
		}break;
	default:
		if (sep == 1 || sep == -1) { return seq_sep_PM1;}
		else { return seq_sep_other; } break;
	}
	return seq_sep_other; // Make compilers happy.
}

	/// Warning if you use this interface you are responsible for
	/// testing if the residues are on different chains!
hbonds::HBEvalTuple
hbond_evaluation_type(
	hbtrie::HBAtom const & datm,
	int const & don_rsd,
	hbtrie::HBAtom const & aatm,
	int const & acc_rsd
){

	HBDonChemType don_chem_type(datm.hb_don_chem_type());
	HBAccChemType acc_chem_type(aatm.hb_acc_chem_type());
	HBSeqSep seq_sep(get_seq_sep(don_chem_type, acc_chem_type, acc_rsd - don_rsd));  //This should really test if things are on different chains
	//HBEvalType hbe(HBEval_lookup(don_chem_type, acc_chem_type, seq_sep));
	return HBEvalTuple( don_chem_type, acc_chem_type, seq_sep );
}

hbonds::HBEvalTuple
hbond_evaluation_type(
	int const datm,
	conformation::Residue const & don_rsd,
	int const aatm,
	conformation::Residue const & acc_rsd
){
	HBDonChemType don_chem_type(get_hb_don_chem_type(datm, don_rsd));
	HBAccChemType acc_chem_type(get_hb_acc_chem_type(aatm, acc_rsd));
	HBSeqSep seq_sep(get_seq_sep(don_chem_type, acc_chem_type, don_rsd.polymeric_oriented_sequence_distance(acc_rsd)));
	//HBEvalType hbe(HBEval_lookup(don_chem_type, acc_chem_type, seq_sep));
	return HBEvalTuple( don_chem_type, acc_chem_type, seq_sep );
}


void
hbond_compute_energy(
	HBondDatabase const & database,
	HBondOptions const & hbondoptions,
	HBEvalTuple hbt,  // the tuple representing the evaluation information for this hbond
	Real const AHdis, // acceptor proton distance
	Real const xD,    // -cos(180-theta), where theta is defined by Tanja K.
	Real const xH,    // cos(180-phi), where phi is defined by Tanja K.
	Real const chi,   // AB2-AB-A-H dihdral angle for sp2 hybridized acceptors
	Real & energy,
	bool & apply_chi_torsion_penalty, // did this hbond get the chi torsion penalty?
	HBGeoDimType & AHD_geometric_dimension, // measure in angle in cosine space?
	Real & dE_dr,
	Real & dE_dxD,
	Real & dE_dxH,
	Real & dE_dBAH, // the change in the energy wrt the BAH angle
	Real & dE_dchi  // the change in the energy wrt the chi dihedral
)
{
	HBEvalType hbe = hbt.eval_type();
	energy = MAX_HB_ENERGY + 1.0f;
	apply_chi_torsion_penalty = false;
	dE_dr = dE_dxD = dE_dxH = dE_dBAH = dE_dchi = 0.0;

	// These should throw an exection if fail_on_bad_hbond is true
	if ( std::abs(xD) > 1.0 || std::abs(xH) > 1.0 ) {
		if ( true )
			tr << "WARNING:: invalid angle value in hbond_compute_energy:"
				<< " xH = " << ObjexxFCL::fmt::SS( xH ) << " xD = " << ObjexxFCL::fmt::SS( xD ) << std::endl;
		return;
	}
	//std::cout << " hb: " << AHdis << " " << xH << " " << xD << std::endl;
	if ( AHdis > MAX_R || AHdis < MIN_R || xH < MIN_xH || xD < MIN_xD ||
		xH > MAX_xH || xD > MAX_xD ) {
		return;
	}

	// The function takes in single precision and computes in double
	// precision To help numeric stability
	double const dAHdis = static_cast<double>(AHdis);
	double const dxD    = static_cast<double>(xD);
	double const dxH    = static_cast<double>(xH);
	double  Pr(0.0),  PSxD(0.0),  PSxH(0.0),  PLxD(0.0),  PLxH(0.0); // values of polynomials
	double dPr(0.0), dPSxD(0.0), dPSxH(0.0), dPLxD(0.0), dPLxH(0.0); // derivatives of polynomials
	double  FSr(0.0),  FLr(0.0),  FxD(0.0),  FxH(0.0); // values of fading intervals
	double dFSr(0.0), dFLr(0.0), dFxD(0.0), dFxH(0.0); // derivatives of fading intervals

	database.AHdist_short_fade_lookup( hbe )->value_deriv(AHdis, FSr, dFSr);
	database.AHdist_long_fade_lookup( hbe )->value_deriv(AHdis, FLr, dFLr);
	database.cosBAH_fade_lookup( hbe )->value_deriv(xH, FxH, dFxH);
	database.cosAHD_fade_lookup( hbe )->value_deriv(xD, FxD, dFxD);

	if ( FSr == Real(0.0) && FLr == Real(0.0) ) {
		// is dAHdis out of range for both its fade function and its polynnomials?  Then set energy > MAX_HB_ENERGY.
		if ( dAHdis < database.AHdist_poly_lookup( hbe )->xmin() ||
			dAHdis > database.AHdist_poly_lookup( hbe )->xmax() ) {
			energy = MAX_HB_ENERGY + Real(1.0);
			return;
		}
	}

	if ( FxH == Real(0.0) ) {
		// is xH out of range for both its fade function and its polynnomials?  Then set energy > MAX_HB_ENERGY.
		if ( ( dxH < database.cosBAH_short_poly_lookup( hbe )->xmin() && dxH < database.cosBAH_long_poly_lookup( hbe )->xmin() ) ||
			( dxH > database.cosBAH_short_poly_lookup( hbe )->xmax() && dxH > database.cosBAH_long_poly_lookup( hbe )->xmax() ) ) {
			energy = MAX_HB_ENERGY + Real(1.0);
			return;
		}
	}

	// add these checks, of course, to the hbeval reading
	assert( database.cosAHD_short_poly_lookup(hbe)->geometric_dimension() == database.cosAHD_long_poly_lookup(hbe)->geometric_dimension() );
	bool const use_cosAHD = database.cosAHD_short_poly_lookup(hbe)->geometric_dimension() == hbgd_cosAHD;
	AHD_geometric_dimension = use_cosAHD ? hbgd_cosAHD : hbgd_AHD;
	assert( use_cosAHD || database.cosAHD_short_poly_lookup(hbe)->geometric_dimension() == hbgd_AHD );

	Real AHD(-1234);
	if ( ! use_cosAHD ) {
		AHD = numeric::constants::d::pi-acos(dxD); // spare this calculation if were evaluating the polynomial in cosine space
	}

	if ( FxD == Real(0.0) ) {
		// is xD out of range for both its fade function and its polynnomials?  Then set energy > MAX_HB_ENERGY.
		if ( use_cosAHD ) {
			if ( ( dxD < database.cosAHD_short_poly_lookup( hbe )->xmin() && dxD < database.cosAHD_long_poly_lookup( hbe )->xmin() ) ||
				( dxD > database.cosAHD_short_poly_lookup( hbe )->xmax() && dxD > database.cosAHD_long_poly_lookup( hbe )->xmax() ) ) {
				energy = MAX_HB_ENERGY + Real(1.0);
				return;
			}
		} else {
			if ( ( AHD < database.cosAHD_short_poly_lookup( hbe )->xmin() && AHD < database.cosAHD_long_poly_lookup( hbe )->xmin() ) ||
				( AHD > database.cosAHD_short_poly_lookup( hbe )->xmax() && AHD > database.cosAHD_long_poly_lookup( hbe )->xmax() ) ) {
				energy = MAX_HB_ENERGY + Real(1.0);
				return;
			}
		}
	}

	(*database.AHdist_poly_lookup( hbe ))(dAHdis, Pr, dPr);
	(*database.cosBAH_short_poly_lookup( hbe ))(dxH, PSxH, dPSxH);
	(*database.cosBAH_long_poly_lookup( hbe ))(dxH, PLxH, dPLxH);
	if ( use_cosAHD ) {
		(*database.cosAHD_short_poly_lookup( hbe ))(dxD, PSxD, dPSxD);
		(*database.cosAHD_long_poly_lookup( hbe ))(dxD, PLxD, dPLxD);
	} else {
		(*database.cosAHD_short_poly_lookup( hbe ))(AHD, PSxD, dPSxD);
		(*database.cosAHD_long_poly_lookup( hbe ))(AHD, PLxD, dPLxD);
	}


	energy = Pr*FxD*FxH + FSr*(PSxD*FxH + FxD*PSxH) + FLr*(PLxD*FxH + FxD*PLxH);

	bool apply_chi_torsion_penalty_sp2(false);
	if(
		hbondoptions.use_sp2_chi_penalty() &&
		get_hbe_acc_hybrid(hbe) == chemical::SP2_HYBRID
	){
		bah_chi_compute_energy_sp2(
			hbondoptions.sp2_BAH180_rise(),
			hbondoptions.fade_energy() ? 1.6 : 1.5,
			hbondoptions.sp2_outer_width(),
			xH, chi, energy, dE_dBAH, dE_dchi);
		apply_chi_torsion_penalty = true;
	} else if (
		hbondoptions.measure_sp3acc_BAH_from_hvy() &&
		( hbt.acc_type() == hbacc_AHX || hbt.acc_type() == hbacc_HXL )
	){
		bah_chi_compute_energy_sp3(xH, chi, energy, dE_dBAH, dE_dchi);
		apply_chi_torsion_penalty = true;
	}

	// NOTE: if any deriv parameter omitted, we don't compute derivatives.
	if (&dE_dxH == &DUMMY_DERIV) {
		fade_energy(hbondoptions, energy);
		return;
	}

	dE_dr =  dPr*FxD*FxH + dFSr*(PSxD*FxH + FxD*PSxH) + dFLr*(PLxD*FxH + FxD*PLxH);

	if(use_cosAHD){
		dE_dxD = dFxD*(Pr*FxH + FLr*PLxH + FSr*PSxH) + FxH*(FSr*dPSxD + FLr*dPLxD);
	} else {
		/// the fade function is still evaluated in cosine space, so its derivatives have to
		/// be converted to units of dE/dAHD by multiplying dE/dcosAHD by sin(AHD)
		/// the polynomial's derivatives, on the other hand, is already in units of dE/dAHD
		dE_dxD = dFxD*(Pr*FxH + FLr*PLxH + FSr*PSxH)*sin(AHD) + FxH*(FSr*dPSxD + FLr*dPLxD);
	}

	dE_dxH = dFxH*(Pr*FxD + FLr*PLxD + FSr*PSxD) + FxD*(FSr*dPSxH + FLr*dPLxH);

	fade_energy(hbondoptions, energy, dE_dr, dE_dxD, dE_dxH, dE_dBAH, dE_dchi);

}

///@detail Because of the additive functional form, in order to make
///derivative continuous at the boundary of definition, we fade the
///energy function smoothly to zero.
///
/// Check that f(x) = -0.025 + 0.5x - 2.5x^2 satisfies
///     f(-.1) = -0.025 +   0.5*(-.1) - 2.5*(-.1)^2 = -.1
///     f( .1) = -0.025 +    0.5*(.1) -  2.5*(.1)^2 = 0
///    f'(-.1) =  0.5   - 2.5*2*(-.1)               = 1
///     f'(.1) =  0.5   -  2.5*2*(.1)               = 0
void
fade_energy(
	HBondOptions const & hbondoptions,
	Real & energy,
	Real & dE_dr,
	Real & dE_dxD,
	Real & dE_dxH,
	Real & dE_dBAH,
	Real & dE_dchi
) {
	if(hbondoptions.fade_energy() && energy > -0.1) {
		if(&dE_dxH != &DUMMY_DERIV){
			dE_dr  *= 5*(0.1-energy);
			dE_dxD *= 5*(0.1-energy);
			dE_dxH *= 5*(0.1-energy);
			dE_dBAH *= 5*(0.1-energy);
			dE_dchi *= 5*(0.1-energy);
		}
		energy = -0.025 + 0.5*energy - 2.5*energy*energy;
	}
}

void
fade_energy_derivs(
) {

}




void
bah_chi_compute_energy_sp2(
	Real const d,
	Real const m,
	Real const l,
	Real const xH,
	Real const chi,
	Real & energy,
	Real & dE_dBAH,
	Real & dE_dchi
) {

	/// Formula #11
	///
	/// F (chi=0 or chi=pi)               | G (chi=pi/2 or chi=3*pi/2)  |
	///------\                   /--------|-----\_              _/------|-  m - 0.5
	///|      \                 /         |       \_          _/        |-  1
	///m       \               /          |         \_      _/          |
	///|        \     /-\     /        ---|           \----/            |-  d - 0.5
	///|_    |   \___/   \___/         _d_|                             |_  -0.5
	///      |<-l->|                      |                             |
	///      |     |<-BAH=2pi/3
	///      |
	///      |<-BAh=2pi/3 - l
	////
	///
	///BAH := Base-Acceptor-Hydrogen interior Angle
	///       BAH=pi when linear and BAH=pi/2 when perpendicular
	///
	///chi :  Torsion angle defined by ABase2-Base-Acceptor-Hydrogen
	///       The Sp2 orbials are in the ABase2-Base-Acceptor plane
	///       For Backbone acceptors ABase2=C-alpha
	///
	///  d := distance from minimum value of -0.5 at BAH=120 to BAH=180 in f
	///       defined by HBondOptions::sp2_BAH180_rise() which is set by
	///       -corrections:score:hb_sp2_BAH180_rise flag and
	///       defaults to 0.75
	///
	///  m := distance from minimum to maximum values of f
	///       must rise high enough so that
	///           E_fade_max = maxBAH_CHI + minAHD + minAHdist
	///                (0.1) = m + (minBAH_CHI) + (-0.5) + (-0.5)
	///                   m  = 1.6
	///  l := period/2 of the BAH=120 to BAH=60 piece of F
	///       emperically fit to be 0.357
	///
	///  F := d/2 * cos(3(pi-BAH) + d/2 - 0.5                        BAH > 2pi/3
	///       m/2 * cos(pi - (2pi/3 - BAH)/l) + m/2 - 0.5    2pi/3 > BAH > pi(2/3 - l)
	///       m-0.5                                    pi(2/3 - l) > BAH
	///
  ///  G := d - 0.5                                                BAH > 2pi/3
	///       (m-d)/2 * cos(pi - (2pi/3 - BAH)/l) + (m-d)/2 + d + 0.5
	///                                                      2pi/3 > BAH > pi(2/3 - l)
	///       m-0.5                                    pi(2/3 - l) > BAH
	///
	///
	///  H := inteprolate smoothly betwen F and G going around chi
	///       (cos(2*chi) + 1)/2
	///
	///  E := Energy for BAH/CHI term
	///       H*F + (1-H)*G
	///
	/// dE/dchi := dH/dchi*f - dH/dchi*g
	///          = -sin(2*chi)*F + sin(2*chi)*G
	///
	/// dE/dBAH := H*dF/dBAH + (1-H)*dG/dBAH
	///
	/// dF/dBAH := 3 * d/2 * sin(3(pi-BAH))                           BAH > 2pi/3
	///            m/2 * -1/l * sin(pi - (2pi/3 - BAH)/l)     2pi/3 > BAH > pi(2/3 - l)
	///            0                                    pi(2/3 - l) > BAH
	///
	/// dG/dBAH := 0                                                  BAH > 2pi/3
	///            (m-d)/2 * -1/l * sin(pi - (2pi/3 - BAH)/)  2pi/3 > BAH > pi(2/3 - l)
	///            0                                    pi(2/3 - l) > BAH

	using std::cos;
	using std::sin;
	using numeric::constants::d::pi;

	Real const PI_minus_BAH( acos(xH) );
	Real const BAH = pi - ( PI_minus_BAH );

	Real const  H((cos(2*chi) + 1) * 0.5);
	Real F(0), G(0);

	if ( BAH >= pi * 2/3 ) {
		F = d/2 * cos(3 * PI_minus_BAH) + d/2 - 0.5;
		G = d - 0.5;
	} else if ( BAH >= pi * (2/3 - l)) {
		Real const outer_rise(cos(pi - (pi*2/3 -  BAH)/l));
		F = m/2 * outer_rise + m/2 - 0.5;
		G = (m - d)/2 * outer_rise + (m - d)/2 + d - 0.5;
	} else {
		F = m-0.5;
		G = m-0.5;
	}

	energy += H*F + (1-H)*G;

	if(&dE_dchi != &DUMMY_DERIV){
		Real const dH_dchi(-1 * sin(2*chi));
		Real dF_dBAH(0), dG_dBAH(0);
		if ( BAH >= pi * 2/3 ) {
			dF_dBAH = 3 * d/2 * sin(3 * PI_minus_BAH);
		} else if ( BAH >= pi * (2/3 - l)) {
			Real const d_outer_rise_dBAH( -1/l * sin(pi - (2*pi/3 - BAH)/l) );
			dF_dBAH = m/2 * d_outer_rise_dBAH;
			dG_dBAH = (m - d)/2 * d_outer_rise_dBAH;
		}
		dE_dchi = F*dH_dchi - G*dH_dchi;
		dE_dBAH = H*dF_dBAH + (1-H)*dG_dBAH;
	}
}

void bah_chi_compute_energy_sp3(
	Real const /*xH*/,
	Real const chi,
	Real & energy,
	Real & dE_dBAH,
	Real & dE_dchi
) {
		// just add in a penalty directly to the energy sum; the chi-penalty
		// is only multiplied in for the sp2 term.
		Real const max_penalty = 0.25;
		Real cos2ChiShifted = max_penalty * ( 1 + std::cos(chi)) / 2;
		energy += cos2ChiShifted;

		if(&dE_dBAH != &DUMMY_DERIV){
			dE_dchi = -1 * max_penalty * std::sin(chi)/2;
		}

}

////////////////////////////////////////////////////////////////////////////////
/// @begin hb_energy_deriv
///
/// @brief
///car Evaluate the energy and derivative components for a hydrogen bond
///
/// @detailed
///car Energy is assumed to be a sum of independent functions of distance,
///car angle at the donor atom, and angle at the acceptor atom. This form
///car of the hydrogen bond potential was selected by Tanja Kortemme
///car The math for computing the derivative of the angular components of
///car the potential was derived by Bill Wedemeyer.
///
///ora used also to calculate derivatives for RB movements if docking_flag T
///ora important NOTE: if in docking mode minimization is done NOT in tr space, this
///ora       should be specified and the docking_specific part should be skipped
///
/// @param  hbe_type - [in] - hydrogen bond evaluation type from hbonds_ns.h
/// @param  donor_res - [in] -
/// @param  acceptor_res - [in] -
/// @param  Dxyz - [in/out] - donor
/// @param  Hxyz - [in/out] - proton
/// @param  Axyz - [in/out] - acceptor
/// @param  Bxyz - [in/out] - acceptor base
/// @param  B2xyz - [in/out] - 2nd acceptor base for ring acceptors
/// @param  energy - [out] -
/// @param  deriv - [out (optional)] - xyz,f1/f2
/// @param  deriv_type [in (optional)] - deriv is NORMAL(default), DOCK_ACC_DON, DOCK_DON_ACC
///
/// @global_read
///   When docking derivatives are computed (DOCK_ACC_DON or DOCK_DON_ACC), center_of_rotation MUST BE DEFINED!
///
/// @global_write
///
/// @remarks
///
/// @references
///
/// @authors
///
/// @last_modified
////////////////////////////////////////////////////////////////////////////////
// Overload to allow non-standard derivative calculations for geometric solvation
void
hb_energy_deriv_u(
	HBondDatabase const & database,
	HBondOptions const & hbondoptions,
	hbonds::HBEvalTuple const hbt, // hb evalation tuple
	Vector const & Hxyz, // proton coords
	Vector const & Dxyz, // Donor coords -- needed for derivative calculations
	Vector const & HDunit, // unit vector toward donor
	Vector const & Axyz, // acceptor coords
	Vector const & Bxyz, // (acceptor) (pseudo) Base coords -- needed for derivative calculations
	Vector const & BAunit, // unit vector towards base
	Vector const & B2xyz, /// acceptor base 2 coords -- will be needed for derivative evaluation when the torsional term comes online
	Real & energy, // returned energy
	bool const evaluate_deriv,
	HBondDerivs & deriv
) {
	HBDerivType const deriv_type = ( evaluate_deriv ? hbderiv_ABE_GO : hbderiv_NONE );
	hb_energy_deriv_u2( database, hbondoptions, hbt, deriv_type, Hxyz, Dxyz, HDunit, Axyz, Bxyz, BAunit, B2xyz, energy, deriv );
}
///////////////////////////////////////////////////////////////////////////////////////
/// @details Innermost score/derivative evaluation logic in this function; "u" stands for "unit vector"
/// and 2 stands for "the second u function" since the arguments to hbond_energy_deriv_u and the arguments
/// to hbond_energy_deriv_u2 are interchangable (i.e. if we tried to overload the hbond_energy_deriv, we end
/// up with infinite recursion as hbond_energy_deriv calls itself over and over again).
/// In here, we have the logic for evaluating the hbond polynomials and, if deriv_type == hbderiv_ABE_GO,
/// then it also computes the f1/f2 vectors for the 4 (eventually 5!) atoms involved in the hydrogen bond.
void
hb_energy_deriv_u2(
	HBondDatabase const & database,
	HBondOptions const & hbondoptions,
	hbonds::HBEvalTuple const hbt, // hb evalation tuple
	HBDerivType const deriv_type,
	Vector const & Hxyz, // proton coords
	Vector const & Dxyz, // Donor coords
	Vector const & HDunit, // unit vector toward donor
	Vector const & Axyz, // acceptor coords
	Vector const & Bxyz, // (acceptor) (pseudo) Base coords
	Vector const & BAunit, // unit vector from base to acceptor
	Vector const & B2xyz, /// acceptor base 2 coords -- will be needed for derivative evaluation when the torsional term comes online
	Real & energy, // returned energy
	HBondDerivs & deriv
)
{
	using namespace hbonds;

	//  angle definitions:  JSS the angle names are really bad. A-H-D = xD and B-A-H = xH
	//   cos(180-theta) = cos(thetaD) = xD    angle to donor
	//   cos(180-psi) = cos(thetaH) = xH      angle to proton
	//   raw angle in radians for ring nitrogen improper dihedral

	//    energy  - total energy from this hbond
	//    dE_dr   - deriv w/respect to distance
	//    dE_dxD  - deriv w/respect to cos(thetaD)
	//    dE_dxH  - deriv w/respect to cos(thetaH)

	energy = MAX_HB_ENERGY + 1.0f;
	//deriv.first = Vector( 0.0 );
	//deriv.second = Vector( 0.0 );

	//Objexx: Local arrays declared static for speed
	//car A->H unit vector, distance
	Vector AH;
	AH = Hxyz - Axyz;
	//Real const AHdis2 = AH(1) * AH(1) + AH(2) * AH(2) + AH(3) * AH(3);
	Real const AHdis2( AH.length_squared() );

	if ( AHdis2 > MAX_R2 ) return;
	if ( AHdis2 < MIN_R2 ) return;
	Real const AHdis = std::sqrt(AHdis2);
	Real const inv_AHdis = 1.0f / AHdis;
	Vector AHunit;
	AHunit = AH * inv_AHdis;


	//BW cosines of angle at donor, proton
	Real const xD =            /* cos(180-theta) = cos(thetaD) */
		dot( AHunit, HDunit );

	if ( xD < MIN_xD ) return;
	if ( xD > MAX_xD ) return;

	Real const xH =            /* cos(180-psi) = cos(thetaH) */
		dot( BAunit, AHunit );

	if ( xH < MIN_xH ) return;
	if ( xH > MAX_xH ) return;

	Real chi( 0 );
	if ( hbondoptions.use_sp2_chi_penalty() &&
			get_hbe_acc_hybrid( hbt.eval_type() ) == chemical::SP2_HYBRID &&
			B2xyz != Vector(-1.0, -1.0, -1.0) ) {
		chi = numeric::dihedral_radians( Hxyz, Axyz, Bxyz, B2xyz );
	} else if ( hbondoptions.measure_sp3acc_BAH_from_hvy() &&
			( hbt.acc_type() == hbacc_AHX || hbt.acc_type() == hbacc_HXL ) ) {
		/// Bxyz really is the heavy atom base and B2xyz really is the hydroxyl hydrogen
		/// this is guaranteed by the hbond_measure_sp3acc_BAH_from_hvy flag.
		chi = numeric::dihedral_radians( Hxyz, Axyz, Bxyz, B2xyz );
	}
	//std::cout << " hb_energy_deriv_u2" <<
	//	" h  =(" << Hxyz.x() << " " << Hxyz.y() << " " << Hxyz.z() << ")\n" <<
	//	" d  =(" << Dxyz.x() << " " << Dxyz.y() << " " << Dxyz.z() << ")\n" <<
	//	" a  =(" << Axyz.x() << " " << Axyz.y() << " " << Axyz.z() << ")\n" <<
	//	" ab =(" << Bxyz.x() << " " << Bxyz.y() << " " << Bxyz.z() << ")\n" <<
	//	" ab2=(" << B2xyz.x() << " " << B2xyz.y() << " " << B2xyz.z() << ")" <<
	//	std::endl;

	if ( deriv_type == hbderiv_NONE ) {
		// NOTE: early return with energy if no derivatives
		hbond_compute_energy( database, hbondoptions, hbt, AHdis, xD, xH, chi, energy );
		return;
	}

	//JSS the rest happens only if we want deriviative information
	Real dE_dxH, dE_dxD, dE_dr, dE_dBAH, dE_dchi;
	bool apply_chi_torsion_penalty( false );
	HBGeoDimType AHD_geometric_dimension;

	hbond_compute_energy(
		database,
		hbondoptions,
		hbt,
		AHdis,
		xD,
		xH,
		chi,
		energy,
		apply_chi_torsion_penalty,
		AHD_geometric_dimension,
		dE_dr,
		dE_dxD,
		dE_dxH,
		dE_dBAH,
		dE_dchi);

	if (energy >= MAX_HB_ENERGY) return;

	deriv.h_deriv.f1() = deriv.h_deriv.f2() = Vector(0.0);
	deriv.acc_deriv.f1() = deriv.acc_deriv.f2() = Vector(0.0);
	deriv.abase_deriv.f1() = deriv.abase_deriv.f2() = Vector(0.0);
	deriv.abase2_deriv.f1() = deriv.abase2_deriv.f2() = Vector(0.0);

	if( ! hbondoptions.use_incorrect_deriv() ){
		/// APL: replacing the older derivative evaluation logic with calls to the
		/// numeric::deriv functions.
		/// There are five atoms, and therefore five derivative-vector pairs.
		/// D -- H -- A -- AB -- AB2
		/// D gets an angle_p1_deriv for the D-H-A angle
		/// H gets a) an angle_p2_deriv for the D-H-A angle,
		///        b) an angle_p1_deriv for the H-A-AB angle, and
		///        c) a distance_deriv for the H-A distance
		///        d) a chi-penalty deriv for the H-A-AB-AB2 dihedral
		/// A gets a) an angle_p1_deriv for the D-H-A angle,
		///        b) an angle_p2_deriv for the H-A-AB angle, and
		///        c) a distance_deriv for the H-A distance
		///        d) a chi-penalty deriv for the H-A-AB-AB2 dihedral
		/// AB gets a) an angle_p1_deriv for the H-A-AB angle
		///         b) a chi-penalty deriv for the H-A-AB-AB2 dihedral
		/// AB2 gets a chi-penalty deriv for the H-A-AB-AB2 dihedral
		using namespace numeric::deriv;

		Vector f1,f2;

		/// 1. H/A distance
		Real temp_AHdis;
		distance_f1_f2_deriv(Hxyz, Axyz, temp_AHdis, f1, f2);
		deriv.h_deriv.f1() = dE_dr * f1;
		deriv.h_deriv.f2() = dE_dr * f2;
		deriv.acc_deriv.f1() = -1 * dE_dr * f1;
		deriv.acc_deriv.f2() = -1 * dE_dr * f2;

		/// 2. theta derivatives (theta is the D-H-A angle)
		if(AHD_geometric_dimension == hbgd_cosAHD){
			Real theta;
			angle_p1_deriv(  Axyz, Hxyz, Dxyz, theta, f1, f2);
			Real const dE_dxD_sin_theta = dE_dxD*sin( theta );
			deriv.acc_deriv.f1() += dE_dxD_sin_theta * f1;
			deriv.acc_deriv.f2() += dE_dxD_sin_theta * f2;

			angle_p1_deriv(  Dxyz, Hxyz, Axyz, theta, f1, f2);
			deriv.don_deriv.f1() = dE_dxD_sin_theta * f1;
			deriv.don_deriv.f2() = dE_dxD_sin_theta * f2;

			angle_p2_deriv(  Dxyz, Hxyz, Axyz, theta, f1, f2);
			deriv.h_deriv.f1() += dE_dxD_sin_theta * f1;
			deriv.h_deriv.f2() += dE_dxD_sin_theta * f2;
		} else if (AHD_geometric_dimension == hbgd_AHD){
			Real theta;
			angle_p1_deriv(  Axyz, Hxyz, Dxyz, theta, f1, f2);
			deriv.acc_deriv.f1() += dE_dxD * f1;
			deriv.acc_deriv.f2() += dE_dxD * f2;

			angle_p1_deriv(  Dxyz, Hxyz, Axyz, theta, f1, f2);
			deriv.don_deriv.f1() = dE_dxD * f1;
			deriv.don_deriv.f2() = dE_dxD * f2;

			angle_p2_deriv(  Dxyz, Hxyz, Axyz, theta, f1, f2);
			deriv.h_deriv.f1() += dE_dxD * f1;
			deriv.h_deriv.f2() += dE_dxD * f2;
		}


		/// 3. phi derivatives (phi is the H-A-AB angle
		{ // scope
		Real phi;
		Vector f1h(0.0),f2h(0.0);
		angle_p1_deriv( Hxyz, Axyz, Bxyz, phi, f1h, f2h);
		Real const dE_dxH_sin_phi = dE_dxH *sin( phi );
		deriv.h_deriv.f1() += dE_dxH_sin_phi * f1h;
		deriv.h_deriv.f2() += dE_dxH_sin_phi * f2h;

		Vector f1b(0.0),f2b(0.0);
		angle_p1_deriv( Bxyz, Axyz, Hxyz, phi, f1b, f2b);
		deriv.abase_deriv.f1() = dE_dxH_sin_phi * f1b;
		deriv.abase_deriv.f2() = dE_dxH_sin_phi * f2b;

		Vector f1a(0.0),f2a(0.0);
		angle_p2_deriv( Bxyz, Axyz, Hxyz, phi, f1a, f2a);
		deriv.acc_deriv.f1() += dE_dxH_sin_phi * f1a;
		deriv.acc_deriv.f2() += dE_dxH_sin_phi * f2a;
		if ( apply_chi_torsion_penalty ) {
			deriv.acc_deriv.f1()   += dE_dBAH * f1a;
			deriv.acc_deriv.f2()   += dE_dBAH * f2a;
			deriv.abase_deriv.f1() += dE_dBAH * f1b;
			deriv.abase_deriv.f2() += dE_dBAH * f2b;
			deriv.h_deriv.f1()     += dE_dBAH * f1h;
			deriv.h_deriv.f2()     += dE_dBAH * f2h;
		}
		}

		/// 4. The chi derivative, for the chi torsion defined by H -- A -- AB -- AB2,
		/// H gets a p1 deriv
		/// A gets a p2 deriv
		/// AB gets a p2 deriv
		/// and AB2 gets a p1 deriv.
		if ( apply_chi_torsion_penalty ) {
			Vector chi_h_f1(0),   chi_h_f2(0);
			Vector chi_a_f1(0),   chi_a_f2(0);
			Vector chi_ab_f1(0),  chi_ab_f2(0);
			Vector chi_ab2_f1(0), chi_ab2_f2(0);

			dihedral_p1_cosine_deriv( Hxyz,  Axyz, Bxyz, B2xyz, chi, chi_h_f1, chi_h_f2 );
			dihedral_p2_cosine_deriv( Hxyz,  Axyz, Bxyz, B2xyz, chi, chi_a_f1, chi_a_f2 );
			dihedral_p2_cosine_deriv( B2xyz, Bxyz, Axyz, Hxyz,  chi, chi_ab_f1, chi_ab_f2 );
			dihedral_p1_cosine_deriv( B2xyz, Bxyz, Axyz, Hxyz,  chi, chi_ab2_f1, chi_ab2_f2 );

			deriv.h_deriv.f1() += dE_dchi * chi_h_f1;
			deriv.h_deriv.f2() += dE_dchi * chi_h_f2;

			deriv.acc_deriv.f1() += dE_dchi * chi_a_f1;
			deriv.acc_deriv.f2() += dE_dchi * chi_a_f2;

			deriv.abase_deriv.f1() += dE_dchi * chi_ab_f1;
			deriv.abase_deriv.f2() += dE_dchi * chi_ab_f2;

			deriv.abase2_deriv.f1() += dE_dchi * chi_ab2_f1;
			deriv.abase2_deriv.f2() += dE_dchi * chi_ab2_f2;

		}

	} else {
		///APL -- older derivative evaluation logic depricated 2010-8-23

		Vector f1( 0.0 );
		Vector f2( 0.0 );

		//car distance-dependent gradient component.
		//car see also comments in minimize.cc
		//car  dr/dphi = Eab x (V-Vb) . (V' - V)/|V-V'|
		//db  (first cross product is the displacement of V upon a rotation dphi
		//db   around the unit vector Eab, Vb is the coordinates of the second atom
		//db   in the bond)

		//car  dEr/dphi = dEr/dr * (Eab  x (V-Vb) . (V' - V)] / |V-V'|
		//car  dEr/dphi = dEr/dr * (Eab  x (V-Vb) . (V' - V)] / r
		//car  rearrange...
		//car  dEr/dphi = dEr/dr * [Eab X Vb . (V' - V) + Eab . (V' x V)] / r

		//car Eab and Eab X Vb are calulated in dfunc_vdw and dependent on the torison
		//car angle with respect to which the derivative is being taken
		//car f1 and f2 are independent of the torsion angle and here we're precomputing
		//car increments to f1 and f2 for each hbond
		//car f1 = dEr/dr * (V'xV) / r
		//car f2 = dEr/dr * (V'-V) / r
		//car here, V' = H
		//car       V  = A
		//car       dE/dr * 1/r = prefactor

		//car Eab and Eab X Vb are calulated in dfunc_vdw, here

		//car V'x V
		Vector HxA;
		HxA = cross( Hxyz, Axyz );

		// in/decrements to the f1 and f2 of the angle moving donor/acceptor
		Real prefactor = inv_AHdis * dE_dr;
		f1 = prefactor * HxA;
		f2 = prefactor * AH;

		//car gradient component for xD (theta)
		//car (see comments below for xH gradient)
		if (deriv_type != hbderiv_ABE_GO_NO_xD ){
			Vector BD;
			prefactor = inv_AHdis * dE_dxD;
			BD = prefactor * ( HDunit - xD * AHunit );

			Vector BDxA;
			BDxA = cross( BD, Axyz );

			//BW in/decrements to the f1 and f2 of the angle moving donor/acceptor
			f1 += BDxA;
			f2 += BD;
		}

		//BW gradient component for xH (psi)
		//car   (from code and paper by Bill Wedemeyer)
		//car  xH = (BAunit . AH)/AHdis
		//car  dxH/dphi = 1/AHdis * (BAunit . dAH/dphi - xH *dAHdis/dphi)
		//car    (note dBAunit/dphi = 0)
		//car
		//car  dAHdis/dphi = AHunit . dAH/dphi
		//car
		//car  substituting and rearranging....
		//car  dxH/dphi = 1/AHdis * dAH/dphi . (BAunit - xH*AHunit)
		//car           = 1/AHdis * dAH/dphi . BH
		//car
		//car note: BH = (BAunit - xH*AHunit) = component of BAunit that is
		//car       perpendicular to AHunit
		//car
		//car dAH/dphi = Eab x (V-Vb)  . (V' - V)/|V-V'|   from above
		//car dAH/dphi = Eab x (H-Vb)  . AHunit
		//car
		//car dExH/dphi = dExH/dxH * dxH/dphi
		//car           = dExH/dxH * 1/AHdis * dAH/dphi . BH
		//car           = dExH/dxH * 1/AHdis * (Eab x (H-Vb) .(H-A)/AHdis) . BH
		//car
		//car rearrange as with dEr/dr above to get f1 and f2 component

		//car f1 = dE/dxH *(1/AHdis) * BH x H
		//car f2 = dE/dxH *(1/AHdis) * BH

		if ( deriv_type != hbderiv_ABE_GO_NO_xH ){

			Vector BH;
			prefactor = inv_AHdis * dE_dxH;
			BH = prefactor * ( BAunit - xH * AHunit );

			Vector BHxH;
			BHxH = cross( BH, Hxyz );

			//BW in/decrements to the f1 and f2 of the angle moving donor/acceptor
			f1 += BHxH;
			f2 += BH;
		}

		deriv.h_deriv.f1() = f1;
		deriv.h_deriv.f2() = f2;
		deriv.acc_deriv.f1() = -1*f1;
		deriv.acc_deriv.f2() = -1*f2;
	}

}


////////////////////////////////////////////////////////////////////////////////
/// @begin hb_energy_deriv
///
/// @remarks See comments on helper function above.
///
////////////////////////////////////////////////////////////////////////////////
// Overload to allow non-standard derivative calculations for geometric solvation
void
hb_energy_deriv(
	HBondDatabase const & database,
	HBondOptions const & hbondoptions,
	HBEvalTuple const hbt, // hbond evaluation tuple -- determines what scoring function to use
	Vector const & Dxyz, // donor coords
	Vector const & Hxyz, // proton
	Vector const & Axyz, // acceptor
	Vector const & Bxyz, // acceptor base
	Vector const & B2xyz, // 2nd acceptor base for ring & SP3 acceptors
	Real & energy,
	bool const evaluate_deriv, // hb derivative type
	HBondDerivs & deriv
){
	HBDerivType const deriv_type = ( evaluate_deriv ? hbderiv_ABE_GO :  hbderiv_NONE );
	hb_energy_deriv(database, hbondoptions, hbt, Dxyz, Hxyz, Axyz, Bxyz, B2xyz, energy, deriv_type, deriv );
}

void
hb_energy_deriv(
	HBondDatabase const & database,
	HBondOptions const & hbondoptions,
	HBEvalTuple const hbt, // hbond evaluation type -- determines what scoring function to use
	Vector const & Dxyz, // donor coords
	Vector const & Hxyz, // proton
	Vector const & Axyz, // acceptor
	Vector const & Bxyz, // acceptor base
	Vector const & B2xyz, // 2nd acceptor base for ring & SP3 acceptors
	Real & energy,
	HBDerivType const deriv_type, // hb derivative type
	HBondDerivs & deriv
)
{
	using namespace hbonds;

//  angle definitions:  JSS the angle names are really bad. A-H-D = xD and B-A-H = xH
//   cos(180-theta) = cos(thetaD) = xD    angle to donor
//   cos(180-psi) = cos(thetaH) = xH      angle to proton
//   raw angle in radians for ring nitrogen improper dihedral

//    energy  - total energy from this hbond
//    dE_dr   - deriv w/respect to distance
//    dE_dxD  - deriv w/respect to cos(thetaD)
//    dE_dxH  - deriv w/respect to cos(thetaH)

//Objexx: Local arrays declared static for speed
//JSS all early exits are in helper above, so this version of the function is deprecated.
//These unit vectors are invariant for hbonded pairs and can be precalculated.
//car  H->D unit vector, dis2
	Vector HDunit;
	HDunit = Dxyz - Hxyz;
	Real const HDdis2( HDunit.length_squared() );

	// NaN check
	if ( ! numeric::is_a_finitenumber( HDdis2, 1.0, 0.0 ) ) {
		std::string const warning( "NAN occurred in H-bonding calculations!" );
		PyAssert(false, warning); // allows for better error handling from within Python
		tr.Error << warning << std::endl;

#ifndef BOINC
		bool fail_on_bad_hbond = basic::options::option[ basic::options::OptionKeys::in::file::fail_on_bad_hbond ]();
		if ( fail_on_bad_hbond ) {
			throw( utility::excn::EXCN_Msg_Exception( warning ) );
			utility_exit();
		}
#endif
	}

	if ( HDdis2 < 0.64 || HDdis2 > 1.5625 ) { // .8 to 1.25A
		if ( true ) {
			// this warning was runlevel dependent
			if ( tr.visible() )
			tr.Debug << "Warning: hb_energy_deriv has H(" << Hxyz(1) << ","
				<< Hxyz(2)<< "," << Hxyz(3) << ") D(" << Dxyz(1) << "," << Dxyz(2)
				<< "," << Dxyz(3) << ")  distance out of range " << std::sqrt( HDdis2 ) << std::endl;
		}
		energy = 0.0;
		//deriv.first = Vector( 0.0 );
		//deriv.second = Vector( 0.0 );
		deriv = ZERO_DERIV2D; /// overwrite everything as 0
		return;
	}

	Real const inv_HDdis = 1.0f / std::sqrt( HDdis2 );
	HDunit *= inv_HDdis;

	//car  B->A unit vector
	Vector BAunit;
	// the pseudo-base xyz coordinate
	Vector PBxyz;
	chemical::Hybridization acc_hybrid( get_hbe_acc_hybrid( hbt.eval_type() ) );
	make_hbBasetoAcc_unitvector(hbondoptions, acc_hybrid, Axyz, Bxyz, B2xyz, PBxyz, BAunit);
	hb_energy_deriv_u2(database, hbondoptions, hbt, deriv_type, Hxyz, Dxyz, HDunit, Axyz, PBxyz, BAunit, B2xyz, energy, deriv );
}


Vector
create_acc_orientation_vector(
	HBondOptions const & hbondoptions,
	conformation::Residue const & residue,
	int atom_id
)
{
	assert( residue.atom_type_set()[ residue.atom(atom_id).type() ].is_acceptor() );
	chemical::Hybridization acc_hybrid(residue.atom_type(atom_id).hybridization());
	Vector ovect, dummy;
	make_hbBasetoAcc_unitvector(
		hbondoptions,
		acc_hybrid,
		residue.atom( atom_id ).xyz(),
		residue.atom( residue.atom_base( atom_id ) ).xyz(),
		residue.atom( residue.abase2( atom_id ) ).xyz(),
		dummy, ovect );
	return ovect;
}


///////////////////////////////////////////////////////////////////////////////
//
// construct for a hydrogen bond Acceptor, the coordinate of the pseudo-atom
// that controls the H-A-AB angle and the the unit vector from the acceptor
// to this pseudo atom.  Different hybridization types use different pseudo-atom
// geometries
//
void
make_hbBasetoAcc_unitvector(
	HBondOptions const & hbondoptions,
	chemical::Hybridization const & acc_hybrid,
	Vector const & Axyz,
	Vector const & Bxyz,
	Vector const & B2xyz,
	Vector & PBxyz,
	Vector & BAunit
)
{
	using namespace chemical;
	switch(acc_hybrid){
	case SP2_HYBRID:  PBxyz = Bxyz; break;
	case SP3_HYBRID:
		/// If the heavy-atom base of an sp3 hybridized acceptor is to be used
		/// to compute the BAH angle (i.e. CB on Ser/Thr), then give it Bxyz;
		/// else, git it B2xyz (i.e. HG on Ser/Thr).
		if ( hbondoptions.measure_sp3acc_BAH_from_hvy() ) {
			PBxyz = Bxyz;
		} else {
			PBxyz = B2xyz;
		}
		break;
	case RING_HYBRID: PBxyz = Real(0.5) * ( Bxyz + B2xyz ); break;
	default:
		BAunit = 0.0;
		tr << "Unrecognized Hybridization: " << acc_hybrid << std::endl;
		utility_exit();
	}
	BAunit = Axyz - PBxyz;
	BAunit.normalize();
}

/// @details Divide up the f1/f2 contributions calculated for the PBxyz coordinate
/// among the atom(s) that define the location of the PBxyz coordinate.  In the
/// base of ring-hybridized acceptors, half of the derivative goes to the abase,
/// and half of hte derivative goes to the abase2.  This code mirrors the logic
/// in the make_hbBasetoAcc_unitvector code above.
void
assign_abase_derivs(
	HBondOptions const & hbondoptions,
	conformation::Residue const & acc_rsd,
	Size acc_atom,
	HBEvalTuple const hbt,
	DerivVectorPair const & abase_deriv,
	Real weighted_energy,
	utility::vector1< DerivVectorPair > & acc_atom_derivs
)
{
	using namespace chemical;
	Hybridization acc_hybrid( get_hbe_acc_hybrid( hbt.eval_type() ) );
	switch( acc_hybrid ){
		case SP2_HYBRID:  {
			acc_atom_derivs[ acc_rsd.atom_base( acc_atom ) ].f1() += weighted_energy * abase_deriv.f1();
			acc_atom_derivs[ acc_rsd.atom_base( acc_atom ) ].f2() += weighted_energy * abase_deriv.f2(); break;
		}
		case SP3_HYBRID:  {
			if ( ! hbondoptions.measure_sp3acc_BAH_from_hvy() ) {
				acc_atom_derivs[ acc_rsd.abase2( acc_atom ) ].f1() += weighted_energy * abase_deriv.f1();
				acc_atom_derivs[ acc_rsd.abase2( acc_atom ) ].f2() += weighted_energy * abase_deriv.f2();
			} else {
				acc_atom_derivs[ acc_rsd.atom_base( acc_atom ) ].f1() += weighted_energy * abase_deriv.f1();
				acc_atom_derivs[ acc_rsd.atom_base( acc_atom ) ].f2() += weighted_energy * abase_deriv.f2();
			}
			break;
		}
		case RING_HYBRID: {
			acc_atom_derivs[ acc_rsd.atom_base( acc_atom ) ].f1() += 0.5 * weighted_energy * abase_deriv.f1();
			acc_atom_derivs[ acc_rsd.atom_base( acc_atom ) ].f2() += 0.5 * weighted_energy * abase_deriv.f2();
			acc_atom_derivs[ acc_rsd.abase2( acc_atom )    ].f1() += 0.5 * weighted_energy * abase_deriv.f1();
			acc_atom_derivs[ acc_rsd.abase2( acc_atom )    ].f2() += 0.5 * weighted_energy * abase_deriv.f2(); break;
		}
		default:
			tr << "Unrecognized Hybridization: " << acc_hybrid << std::endl;
			utility_exit();
	}

}


/// @brief create a unit vector pointing from the hydrogen toward the donor
/// The atom_id is the atom id of the hydrogen atom
Vector
create_don_orientation_vector(
	conformation::Residue const & residue,
	int atom_id
)
{
	assert( residue.atom_type_set()[ residue.atom( residue.atom_base(atom_id)).type() ].is_donor() );

	Vector HDunit;
	HDunit = residue.atom( residue.atom_base( atom_id ) ).xyz() - residue.atom( atom_id ).xyz();
	Real const HDdis2( HDunit.length_squared() );
	Real const inv_HDdis = 1.0f / std::sqrt( HDdis2 );
	HDunit *= inv_HDdis;
	return HDunit;
}

} // hbonds
} // scoring
} // core
