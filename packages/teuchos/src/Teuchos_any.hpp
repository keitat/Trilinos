// /////////////////////////////////////////////////////////////
// Teuchos_any.hpp

#ifndef TEUCHOS_ANY_HPP
#define TEUCHOS_ANY_HPP

#include "Teuchos_TestForException.hpp"

//
// This file was taken from the boost library which contained the
// following notice:
//
// *************************************************************
//
// what:  variant type boost::any
// who:   contributed by Kevlin Henney,
//        with features contributed and bugs found by
//        Ed Brey, Mark Rodgers, Peter Dimov, and James Curran
// when:  July 2001
// where: tested with BCC 5.5, MSVC 6.0, and g++ 2.95
//
// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Permission to use, copy, modify, and distribute this software for any
// purpose is hereby granted without fee, provided that this copyright and
// permissions notice appear in all copies and derivatives.
//
// This software is provided "as is" without express or implied warranty.
//
// *************************************************************
//
// RAB modified the file for use in Teuchos.  I changed the nature of
// the any_cast<> to be easier to use.
//

namespace Teuchos {

///
/** The "any" class.
 *
 * ToDo: Finish documentatioin!
 */
class any
{
public:

	any()
		: content(0)
		{}

	template<typename ValueType>
	any(const ValueType & value)
		: content(new holder<ValueType>(value))
		{}

	any(const any & other)
		: content(other.content ? other.content->clone() : 0)
		{}

	~any()
		{
			delete content;
		}

	any & swap(any & rhs)
		{
			std::swap(content, rhs.content);
			return *this;
		}
	
	template<typename ValueType>
	any & operator=(const ValueType & rhs)
		{
			any(rhs).swap(*this);
			return *this;
		}
	
	any & operator=(const any & rhs)
		{
			any(rhs).swap(*this);
			return *this;
		}
	
	bool empty() const
		{
			return !content;
		}
	
	const std::type_info & type() const
		{
			return content ? content->type() : typeid(void);
		}

	void print(std::ostream& os) const
		{
			if (content) content->print(os);
		}

	/** @name Private??? types */
	//@{

	///
	class placeholder
	{
	public:
		///
		virtual ~placeholder() {}
		///
		virtual const std::type_info & type() const = 0;
		///
		virtual placeholder * clone() const = 0;
                ///
                virtual void print(std::ostream & os) const = 0;
	};
	
	///
	template<typename ValueType>
	class holder : public placeholder
	{
	public:
		///
		holder(const ValueType & value)
			: held(value)
			{}
		///
		virtual const std::type_info & type() const
			{ return typeid(ValueType); }
		///
		virtual placeholder * clone() const
			{ return new holder(held); }
                ///
                virtual void print(std::ostream & os) const
                        { os << held; }
		///
		ValueType held;
	};

	//@}

private:

	// /////////////////////////
	// Private data members
	
	placeholder * content;

public:
	// Danger: This is made public to allow any_cast to be non-friend
	placeholder* access_content()
		{ return content; }
	const placeholder* access_content() const
		{ return content; }

};

///
class bad_any_cast : public std::runtime_error
{
public:
	bad_any_cast( const std::string msg ) : std::runtime_error(msg) {}
};

///
template<typename ValueType>
ValueType& any_cast(any &operand)
{
	TEST_FOR_EXCEPTION(
		operand.type() != typeid(ValueType), bad_any_cast
		,"any_cast<" << typeid(ValueType).name() << "(operand): Error, cast to type \'"
		<< typeid(any::holder<ValueType>).name() << "\' failed since the actual underlying type is \'"
		<< typeid(*operand.access_content()).name() << "!"
		);
	any::holder<ValueType>
		*dyn_cast_content = dynamic_cast<any::holder<ValueType>*>(operand.access_content());
	TEST_FOR_EXCEPTION(
		!dyn_cast_content, std::logic_error
		,"any_cast<" << typeid(ValueType).name() << "(operand): Error, cast to type \'"
		<< typeid(any::holder<ValueType>).name() << "\' failed but should not have and the actual underlying type is \'"
		<< typeid(*operand.access_content()).name() << "!"
		);
	return dyn_cast_content->held;
}

///
template<typename ValueType>
const ValueType& any_cast(const any &operand)
{
	return any_cast<ValueType>(const_cast<any&>(operand));
}


inline std::ostream & operator<<(std::ostream & os, const any &rhs)
{
       	rhs.print(os);
	return os;
}

} // namespace Teuchos

#endif // TEUCHOS_ANY_HPP
