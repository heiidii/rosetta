// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/antibody/GraftOneCDRLoop.hh
/// @brief grafts a cdr onto the template of an antibody framework
/// @detailed
/// @author Jianqing Xu (xubest@gmail.com)


#ifndef INCLUDED_protocols_antibody_GraftOneCDRLoop_hh
#define INCLUDED_protocols_antibody_GraftOneCDRLoop_hh

#include <protocols/antibody/GraftOneCDRLoop.fwd.hh>

#include <protocols/moves/Mover.hh>
#include <core/pose/Pose.hh>
#include <core/scoring/ScoreFunction.fwd.hh>  
#include <protocols/antibody/AntibodyInfo.hh>
#include <protocols/antibody/Ab_TemplateInfo.fwd.hh>

namespace protocols {
namespace antibody {

/// @brief Grafts only one CDR onto a framework
class GraftOneCDRLoop : public protocols::moves::Mover {
public:
		// default constructor
		GraftOneCDRLoop();

		// constructor with arguments        
		GraftOneCDRLoop(CDRNameEnum const & cdr_name,
							AntibodyInfoOP antibody_info,
                             Ab_TemplateInfoOP ab_t_info);

        
		~GraftOneCDRLoop();
		
		virtual void apply( core::pose::Pose & pose_in );
		
		virtual std::string get_name() const;
		
		
		/// @brief enable benchmark mode
		inline void enable_benchmark_mode( bool setting ) {
			benchmark_ = setting;
		}
		
		///@brief users can pass their own scorefunction for packing after grafting
		void set_scorefxn(core::scoring::ScoreFunctionOP scorefxn){
			scorefxn_ = scorefxn;
		}
		
		///@brief R2 just graft R2 by copying some stem residues, and remove H3 to do
		///       H3 loop modeling later. But the terminal has been changed by this
		///       grafting. Therefore, in R3, an option for not copying h3 stem is
		///       provided.
		void h3_stem_off( bool setting ){
			h3_stem_not_graft_ = setting;
		}
	
		void set_flank_size(Size setting){
			flank_size_=setting;
		}
		
        ///@brief copy ctor
    	GraftOneCDRLoop( GraftOneCDRLoop const & rhs );

    	///@brief assignment operator
    	GraftOneCDRLoop & operator=( GraftOneCDRLoop const & rhs );
	

private:
		void set_default();
		void init();
		void finalize_setup();
		void initForEqualOperatorAndCopyConstructor(GraftOneCDRLoop & lhs, GraftOneCDRLoop const & rhs);
		
		
private:
		
		// Limits of query loop
        core::Size flank_size_;
		core::Size stem_copy_size_;
				
		CDRNameEnum cdr_name_;
		AntibodyInfoOP ab_info_;
		Ab_TemplateInfoOP ab_t_info_;
		
		bool benchmark_;
		bool h3_stem_not_graft_;

		core::scoring::ScoreFunctionCOP scorefxn_;


}; // class GraftOneCDRLoop





} // antibody
} // protocols








#endif
