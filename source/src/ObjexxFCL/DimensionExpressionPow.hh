#ifndef INCLUDED_ObjexxFCL_DimensionExpressionPow_hh
#define INCLUDED_ObjexxFCL_DimensionExpressionPow_hh


// DimensionExpressionPow: DimensionExpression Power Function: expression1^expression2
//
// Project: Objexx Fortran Compatibility Library (ObjexxFCL)
//
// Version: 3.0.0
//
// Language: C++
//
// Copyright (c) 2000-2009 Objexx Engineering, Inc. All Rights Reserved.
// Use of this source code or any derivative of it is restricted by license.
// Licensing is available from Objexx Engineering, Inc.:  http://objexx.com  Objexx@objexx.com


// ObjexxFCL Headers
#include <ObjexxFCL/DimensionExpression.hh>
#include <ObjexxFCL/DimensionExpressionCon.hh>
#include <ObjexxFCL/Fmath.hh>

// C++ Headers
#include <cmath>


namespace ObjexxFCL {


/// @brief DimensionExpressionPow: DimensionExpression Power Function: expression1^expression2
class DimensionExpressionPow :
	public DimensionExpression
{


private: // Types


	typedef  DimensionExpression  Super;


public: // Creation


	/// @brief Copy Constructor
	inline
	DimensionExpressionPow( DimensionExpressionPow const & exp ) :
		Super(),
		exp1_p_( exp.exp1_p_ ? exp.exp1_p_->clone() : static_cast< DimensionExpression * >( 0 ) ),
		exp2_p_( exp.exp2_p_ ? exp.exp2_p_->clone() : static_cast< DimensionExpression * >( 0 ) )
	{
		assert( exp1_p_ );
		assert( exp2_p_ );
	}


	/// @brief Expression Constructor
	inline
	DimensionExpressionPow( DimensionExpression const & exp1, DimensionExpression const & exp2 ) :
		exp1_p_( exp1.clone() ),
		exp2_p_( exp2.clone() )
	{
		assert( exp1_p_ );
		assert( exp2_p_ );
	}


	/// @brief Expression Pointer Constructor (Ownership Transfer)
	inline
	DimensionExpressionPow( DimensionExpression * exp1_p_a, DimensionExpression * exp2_p_a ) :
		exp1_p_( exp1_p_a ),
		exp2_p_( exp2_p_a )
	{
		assert( exp1_p_ );
		assert( exp2_p_ );
	}


	/// @brief Clone
	inline
	DimensionExpression *
	clone() const override
	{
		assert( exp1_p_ );
		assert( exp2_p_ );
		if ( constant() ) {
			if ( integer() ) {
				return new DimensionExpressionCon( nint( std::pow( exp1_p_->value(), exp2_p_->ivalue() ) ) );
			} else {
				return new DimensionExpressionCon( std::pow( exp1_p_->value(), exp2_p_->value() ) );
			}
		} else {
			return new DimensionExpressionPow( exp1_p_->clone(), exp2_p_->clone() );
		}
	}


	/// @brief Clone with Dimension Substitution
	inline
	DimensionExpression *
	clone( Dimension const & dim ) const override
	{
		assert( exp1_p_ );
		assert( exp2_p_ );
		if ( constant() ) {
			if ( integer() ) {
				return new DimensionExpressionCon( nint( std::pow( exp1_p_->value(), exp2_p_->ivalue() ) ) );
			} else {
				return new DimensionExpressionCon( std::pow( exp1_p_->value(), exp2_p_->value() ) );
			}
		} else {
			return new DimensionExpressionPow( exp1_p_->clone( dim ), exp2_p_->clone( dim ) );
		}
	}


	/// @brief Destructor
	inline
	~DimensionExpressionPow() override
	{
		assert( exp1_p_ );
		assert( exp2_p_ );
		delete exp1_p_;
		delete exp2_p_;
	}


public: // Inspector


	/// @brief Initialized?
	inline
	bool
	initialized() const override
	{
		assert( exp1_p_ );
		assert( exp2_p_ );
		return ( ( exp1_p_->initialized() ) && ( exp2_p_->initialized() ) );
	}


	/// @brief Integer?
	inline
	bool
	integer() const override
	{
		assert( exp1_p_ );
		assert( exp2_p_ );
		return ( ( exp1_p_->integer() ) && ( exp2_p_->integer() ) );
	}


	/// @brief Constant?
	inline
	bool
	constant() const override
	{
		assert( exp1_p_ );
		assert( exp2_p_ );
		return ( ( exp1_p_->constant() ) && ( exp2_p_->constant() ) );
	}


	/// @brief Reference?
	inline
	bool
	reference() const override
	{
		assert( exp1_p_ );
		assert( exp2_p_ );
		return ( ( exp1_p_->reference() ) || ( exp2_p_->reference() ) );
	}


	/// @brief Reducible?
	inline
	bool
	reducible() const override
	{
		assert( exp1_p_ );
		assert( exp2_p_ );
		return ( ( constant() ) || ( exp1_p_->reducible() ) || ( exp2_p_->reducible() ) );
	}


	/// @brief Value
	inline
	double
	operator ()() const override
	{
		assert( exp1_p_ );
		assert( exp2_p_ );
		return ( std::pow( exp1_p_->operator ()(), exp2_p_->operator ()() ) );
	}


	/// @brief Value
	inline
	double
	value() const override
	{
		assert( exp1_p_ );
		assert( exp2_p_ );
		return ( std::pow( exp1_p_->value(), exp2_p_->value() ) );
	}


	/// @brief Integer Value
	inline
	int
	ivalue() const override
	{
		return ( integer() ? nint( value() ) : static_cast< int >( value() ) );
	}


	/// @brief Integer Value: Zero if Uninitialized
	inline
	int
	zvalue() const override
	{
		return ( initialized() ? ivalue() : 0 );
	}


	/// @brief Insert an Observer
	inline
	void
	insert_observer( Observer & observer ) const override
	{
		assert( exp1_p_ );
		assert( exp2_p_ );
		exp1_p_->insert_observer( observer );
		exp2_p_->insert_observer( observer );
	}


	/// @brief Remove an Observer
	inline
	void
	remove_observer( Observer & observer ) const override
	{
		assert( exp1_p_ );
		assert( exp2_p_ );
		exp1_p_->remove_observer( observer );
		exp2_p_->remove_observer( observer );
	}


public: // Modifier


	/// @brief Update for Destruction of a Subject
	inline
	void
	destructed( Subject const & subject ) override
	{
		assert( exp1_p_ );
		assert( exp2_p_ );
		exp1_p_->destructed( subject );
		exp2_p_->destructed( subject );
	}


private: // Data


	/// @brief Pointer to expression 1
	DimensionExpression * exp1_p_;

	/// @brief Pointer to expression 2
	DimensionExpression * exp2_p_;


}; // DimensionExpressionPow


} // namespace ObjexxFCL


#endif // INCLUDED_ObjexxFCL_DimensionExpressionPow_HH
