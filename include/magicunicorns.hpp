/*  
 * Copyright (C) 2011 magicunicorns authors,
 *
 * This file is part of magicunicorns.
 *   
 * magicunicorns is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * magicunicorns is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>

struct abstract_field
{
	virtual std::string type() = 0;
	virtual std::string name() = 0;
};

struct table
{
	table(const std::string& tablename) :
		tablename_(tablename) {}
	
	void add_field(abstract_field* field)
	{
		fields_.push_back(std::pair<std::string, std::string>(field->name(), field->type()));
	}
	
	typedef std::vector<std::pair<std::string, std::string> > fields_t;
	fields_t fields_;
	std::string tablename_;
};

/* 
 * String representation of type.
 * To be used with data serialization and also we can forbid weird
 * data types at compile time.
 */
template <typename T>
struct get_type;

template <>
struct get_type<std::string> { std::string value() const { return "TEXT"; } };

template <>
struct get_type<int> { std::string value() const { return "INTEGER"; } };

/**
 * Field. Actually a POD variable wrapper.
 */
template <typename T>
struct field: abstract_field
{
	typedef T value_type;
	
	field(table* parent, std::string name, const value_type& value = value_type()):
		name_(name),
		value_(value)
	{
		parent->add_field(this);
	}
	
	field& operator=(T new_value)
	{
		value_ = new_value;
		return *this;
	}
	
	bool operator==(value_type val)
	{
		return value_ == val;
	}
	
	operator T()
	{
		return value_;
	}
	operator const T()
	{
		return value_;
	}
	
	friend std::ostream& operator<<(std::ostream& out, const field& fld)
	{
		out << fld.value_;
		return out;
	}
	
	std::string name_;
	value_type value_;
	get_type<T> type_;
	
	virtual std::string name() { return name_; }
	virtual std::string type() { return type_.value(); }
};

/**
 * Iterator wrapper. Easy iterating over result sets.
 */
template <typename T /* Container */>
struct cursor_impl
{
	cursor_impl(T t):
		container_(t),
		it_(container_.begin())
	{
	}
	
	/**
	 * Safe bool idiom
	 */	
	operator bool()
	{
		return it_ != container_.end();
	}
	
	cursor_impl& operator++()
	{
		++it_;
		return *this;
	}
	
	typename std::iterator_traits<typename T::iterator>::value_type operator*()
	{
		return *it_;	
	}
	
	T container_;
	typename T::iterator it_;
	typename T::iterator end_;
};

template <typename T>
struct dbset
{
	std::list<T> rows_;
	
	typedef cursor_impl<std::list<T> > cursor;
	
	dbset()
	{
		
	}
	
	void put(T t)
	{
		rows_.push_back(t);
	}
	
	/**
	 * This thing simply iterates over rows,
	 * evaluates expression with value, and if it evaluates to true
	 * then copy it to 'result set'.
	 * @param f operator instance with type of operator_impl.
	 */
	template <typename F>
	std::list<T> filter(F f)
	{
		std::list<T> results;
		
		for (typename std::list<T>::iterator it = rows_.begin(),
			end(rows_.end()); it != end; it++)
		{
			if (f(it))
				results.push_back(*it);
		}
		
		return results;
	}
	
	std::list<T>& all()
	{
		return rows_;
	}
};

struct dbcontext
{
	/* TODO: database context here */
};

/**
 * This class will be evaluated later to value from member class field.
 */
template <typename T1, typename T2>
struct field_impl
{
	field<T1> T2::* field_;
	field_impl(field<T1> T2::* t): field_(t) {}
	
	template <typename F1>
	T1 operator()(F1 obj)
	{
		return (*obj.*field_).value_;
	}
};

/**
 * Implementation of operator== (logical AND)
 */
template <typename T1, typename T2>
struct eq_impl
{
	T1 expr_;
	T2 value_;
	
	eq_impl(T1 t, T2 value):
		expr_(t), value_(value) {}
	
	template <typename F1>
	bool operator()(F1 obj)
	{
		return expr_(obj) == value_;
	}
	
	template <typename F1, typename F2>
	F1 operator()(F1 obj, F2 val)
	{
		return expr_(obj, val) == value_;
	}
};

/**
 * Implementation of operator!= (logical NOT EQUAL)
 */
template <typename T1, typename T2>
struct neq_impl
{
	T1 expr_;
	T2 value_;
	
	neq_impl(T1 t, T2 value): expr_(t), value_(value) {}
	
	template <typename F1>
	bool operator()(F1 obj)
	{
		return expr_(obj) != value_;
	}
	
	template <typename F1, typename F2>
	F1 operator()(F1 obj, F2 val)
	{
		return expr_(obj, val) != value_;
	}
};

/**
 * Implementation of operator> (logical GREATER THAN)
 */
template <typename T1, typename T2>
struct gt_impl
{
	T1 expr_;
	T2 value_;
	
	gt_impl(T1 t, T2 value): expr_(t), value_(value) {}
	
	template <typename F1>
	bool operator()(F1 obj)
	{
		return expr_(obj) > value_;
	}
	
	template <typename F1, typename F2>
	F1 operator()(F1 obj, F2 val)
	{
		return expr_(obj, val) > value_;
	}
};

/**
 * Implementation of operator< (logical LOWER THAN)
 */
template <typename T1, typename T2>
struct lt_impl
{
	T1 expr_;
	T2 value_;
	
	lt_impl(T1 t, T2 value): expr_(t), value_(value) {}
	
	template <typename F1>
	bool operator()(F1 obj)
	{
		return expr_(obj) < value_;
	}
	
	template <typename F1, typename F2>
	F1 operator()(F1 obj, F2 val)
	{
		return expr_(obj, val) < value_;
	}
};

/**
 * Implementation of operator& (logical AND)
 */
template <typename T1, typename T2>
struct and_impl
{
	T1 expr_;
	T2 value_;
	
	and_impl(T1 t, T2 value): expr_(t), value_(value) {}
	
	template <typename T>
	bool operator()(T obj)
	{
		return expr_(obj) && value_(obj);
	}
};

/**
 * As I can not force static operator to accept type of field<T1> T2::*
 * I had to do this.
 * This thing actually spawns new instance of field_impl which just
 * holds fld value.
 */
template <typename T1, typename T2>
field_impl<T1, T2> F(field<T1> T2::* fld)
{
	return field_impl<T1, T2>(fld);
}

template <typename T1, typename T2>
eq_impl<T1, T2> operator==(T1 t1, T2 t2)
{
	return eq_impl<T1, T2>(t1, t2);
}

template <typename T1, typename T2>
neq_impl<T1, T2> operator!=(T1 t1, T2 t2)
{
	return neq_impl<T1, T2>(t1, t2);
}

template <typename T1, typename T2>
and_impl<T1, T2> operator&(T1 t1, T2 t2)
{
	return and_impl<T1, T2>(t1, t2);
}

template <typename T1, typename T2>
gt_impl<T1, T2> operator>(T1 t1, T2 t2)
{
	return gt_impl<T1, T2>(t1, t2);
}

template <typename T1, typename T2>
lt_impl<T1, T2> operator<(T1 t1, T2 t2)
{
	return lt_impl<T1, T2>(t1, t2);
}
