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
#include <exception>

/* Constraints implementation */

struct constraint_expr
{
	template <typename T>
	T& operator=(T t)
	{
		return *this;
	}
};

template <typename Impl>
class constraint
{
	private:
		constraint(const constraint&);
		
	public:
		constraint() {}
		~constraint() {}
		Impl impl_;

		template <typename T1 /* Field */, typename T2 /* dbset */>
		void operator()(T1& t1, T2& t2)
		{
			t1 = impl_(t1, t2);
		}
};

template <typename T1, typename T2>
struct constraint_or
{
	T1& t1_;
	T2& t2_;
	constraint_or(T1& t1, T2& t2) :
		t1_(t1), t2_(t2) {}
		
	template <typename F1 /* Field */, typename F2 /* dbset */>
	void operator()(F1& f1, F2& f2)
	{
		t1_(f1, f2);
		t2_(f1, f2);
	}
};

template <typename T1, typename T2>
constraint_or<constraint<T1>, constraint<T2> > operator|(constraint<T1>& t1, constraint<T2>& t2)
{
	return constraint_or<constraint<T1>, constraint<T2> >(t1, t2);
}
/* end */

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
	
	bool operator==(const field& other)
	{
		return value_ == other.value_;
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
	constraint_expr constraint_; /* Constraint expr */
	
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
	
	/**
	 * Update rows matching expr... If expr `where` evaluated to true
	 * then evaluate expr `stmt`
	 */
	template <typename F1, typename F2>
	void update(F1 where, F2 stmt)
	{
		for (typename std::list<T>::iterator it = rows_.begin(),
			end(rows_.end()); it != end; it++)
		{
			if (where(it))
				stmt(it);
		}
	}
	
	/**
	 * Update all rows... Really just evaluate stmt with every object
	 * in set.
	 */
	template <typename F>
	void update(F stmt)
	{
		for (typename std::list<T>::iterator it = rows_.begin(),
			end(rows_.end()); it != end; it++)
		{
			stmt(it);
		}
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
 * Chain operator implementation
 * This is not static operator because of possible compilation failure
 */
template <typename T1, typename T2>
struct chain_impl
{
	chain_impl(T1 t1, T2 t2): t1_(t1), t2_(t2) {}
	T1 t1_;
	T2 t2_;
	template <typename F1>
	void operator()(F1 f1)
	{
		t1_(f1);
		t2_(f1);
	}
};

/**
 * Assignment operator implementation
 * Can not be static
 */
template <typename T1, typename T2>
struct assign_impl
{
	assign_impl(T1 t1, T2 t2): t1_(t1), t2_(t2) {}
	T1 t1_;
	T2 t2_;
	
	template <typename F1>
	void operator()(F1 f1)
	{
		t1_(f1) = t2_(f1);
	}
	
	template <typename A1, typename A2>
	chain_impl<assign_impl<T1, T2>, assign_impl<A1, A2> > operator,(assign_impl<A1, A2> f)
	{
		return chain_impl<assign_impl<T1, T2>, assign_impl<A1, A2> >(*this, f);
	}
};

/**
 * This class will be evaluated later to value from member class field.
 */
template <typename T1, typename T2>
struct field_impl
{
	typedef T1 value_type;
	field<T1> T2::* field_;
	field_impl(field<T1> T2::* t): field_(t) {}
	
	template <typename F>
	assign_impl<field_impl<T1, T2>, F> operator=(F f)
	{
		return assign_impl<field_impl<T1, T2>, F>(*this, f);
	}
	
	template <typename F1>
	T1& operator()(F1 obj)
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


template <typename T1>
struct value_impl
{
	/**
	 * Ugly hack to know the type of trapped value
	 */
	typedef T1 value_type;
	
	value_impl(T1 t1): t1_(t1) {}
	T1 t1_;
	
	/**
	 * Return trapped value
	 */
	T1 operator()() { return t1_; }
	
	/**
	 * Return trapped value and act as field_impl.
	 */
	template <typename F>
	T1 operator()(F) { return t1_; }
	
	/**
	 * If accidental implicit conversion happens...
	 */
	operator value_type() { return t1_; }
};

/**
 * Implementation of operator+ (A + B)
 */
template <typename T1, typename T2>
struct plus_impl
{
	typedef T1 value_type; /* plus_impl acts as value_impl too... */
	
	T1 expr_;
	T2 value_;
	
	plus_impl(T1 t, T2 value): expr_(t), value_(value) {}
	
	template <typename F>
	value_impl<typename T1::value_type> operator()(F obj)
	{
		return value_impl<typename T1::value_type>(expr_(obj) + value_(obj));
	}
};

/**
 * Value holder
 */
template <typename T1>
value_impl<T1> val(T1 t1)
{
	return value_impl<T1>(t1);
}

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

template <typename T1, typename T2>
plus_impl<T1, T2> operator+(T1 t1, T2 t2)
{
	return plus_impl<T1, T2>(t1, t2);
}

/*template <typename T1, typename T2, typename Cls>
plus_impl<field_impl<T1, Cls>, value_impl<T2> > operator+(field_impl<T1, Cls> t1, value_impl<T2> t2)
{
	return plus_impl<field_impl<T1, Cls>, value_impl<T2> >(t1, t2);
}

template <typename T1, typename T2, typename Cls>
plus_impl<field_impl<T1, Cls>, field_impl<T2, Cls> > operator+(field_impl<T1, Cls> t1, field_impl<T2, Cls> t2)
{
	return plus_impl<field_impl<T1, Cls>, field_impl<T2, Cls> >(t1, t2);
}*/



/*template <typename T1, typename T2, typename T3, typename T4>
chain_impl<assign_impl<T1, T2>, assign_impl<T3, T4> > operator,(assign_impl<T1, T2>& t1, assign_impl<T3, T4>& t2)
{
	return chain_impl<assign_impl<T1, T2>, assign_impl<T3, T4> >(t1, t2);
}*/

/* Constraints implementations */

struct auto_increment_impl
{
	template <typename T1, typename T2>
	field<T1>& operator()(field<T1>& source, dbset<T2>& set)
	{
		std::cout << "Auto increment from value (" << source << ") new value: " << (set.all().size() + 1) << std::endl;
		source = set.all().size() + 1;
		return source;
	}
};

struct unique_constraint_violation: std::exception
{
	virtual const char* what() const throw()
	{
		return "unique constraint violation";
	}
};

struct unique_impl
{
	template <typename T>
	T& operator()(T& source, dbset<T>& set)
	{
		std::list<T>& all = set.all();
		for (typename std::list<T>::iterator it(all.begin()),
			end_(all.end());
			it != end_;
			++it)
		{
			if (*it == source)
				throw unique_constraint_violation();
		}
		return source;
	}
	
	/**
	 * TODO: Field must be able to return instance of type T2
	 */
	template <typename T1, typename T2>
	field<T1>& operator()(field<T1>& source, dbset<T2>& set)
	{
		return source;
	}	
};

static constraint<auto_increment_impl> auto_increment;
static constraint<unique_impl> unique;
