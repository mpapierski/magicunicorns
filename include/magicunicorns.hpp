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

struct abstract_field;
struct table;

struct abstract_dbset
{
	virtual unsigned int size() const = 0;

	/* Check if object exists in set */
	virtual bool exists(table* obj) = 0;
};

/* Constraints implementation */
struct abstract_constraint
{
	/**
	 * Check field, table, or whole dataset.
	 */
	virtual void operator()(abstract_field*, table*, abstract_dbset*) = 0;
};

struct constraint_expr: std::list<abstract_constraint*>, abstract_constraint
{
	constraint_expr() {}
	
	constraint_expr(abstract_constraint* first)
	{
		this->push_back(first);
	}
	
	constraint_expr& operator=(abstract_constraint& impl)
	{
		this->push_back(&impl);
		return *this;
	}
	
	constraint_expr& operator+ (abstract_constraint& other)
	{
		this->push_back(&other);
		return *this;
	}
	
	constraint_expr& operator+ (abstract_constraint* other)
	{
		this->push_back(other);
		return *this;
	}
	
	/* Evaluate */
	virtual void operator()(abstract_field* fld, table* tbl, abstract_dbset* set)
	{
		for (list<abstract_constraint*>::iterator it(this->begin()), end_(end()); it != end_; ++it)
		{
			(*it)->operator()(fld, tbl, set);
		}
	}
};

/* Interfaces */
struct abstract_field
{
	virtual std::string type() = 0;
	virtual std::string name() = 0;
	virtual bool operator==(abstract_field& other) = 0;
	
	bool operator==(abstract_field* other)
	{
		return this->operator==(*other);
	}
	
	bool operator!=(abstract_field& other)
	{
		return !this->operator==(other);
	}
	
	constraint_expr constraint; /* Constraint expr */
};


/* You can assign even complicated expressions into single object */

struct expression_functor
{
	virtual bool operator()(table*) = 0;
};

template <typename CtorType, typename Obj>
struct expression_functor_wrapper: expression_functor
{
	CtorType ff_;
	expression_functor_wrapper(CtorType ff): ff_(ff) {}
	
	virtual bool operator()(table* tbl)
	{
		return ff_(static_cast<Obj*>(tbl));
	}
};

template <typename ObjT>
struct expression
{
		typedef ObjT object_type;
		expression_functor* ctor_;
		
		expression() :
			ctor_(NULL) {}
		
		template <typename F>
		expression(F f) :
			ctor_(new expression_functor_wrapper<F, object_type>(f)) {}
		
		template <typename F>
		expression& operator=(F f)
		{
			if (!empty())
				delete ctor_;
			ctor_ = new expression_functor_wrapper<F, object_type>(f);
			return *this;
		}
			
		~expression()
		{
			if (empty())
				return;
			delete ctor_;
		}
		
		/**
		 * Call wrapped expression
		 */
		bool operator()(object_type* tbl)
		{
			return (*ctor_)(tbl);
		}
		
		/* No expression assigned? */
		bool empty() const { return !ctor_; }
};

struct table
{
	table(const std::string& tablename) :
		tablename_(tablename) {}
	
	void add_field(abstract_field* field)
	{
		fields_.push_back(field);
	}
	
	typedef std::vector<abstract_field*> fields_t;
	
	fields_t fields_;
	std::string tablename_;
	
	/* Trigger list 
	 * When expr1 evaluates to true, then evaluate second expression */	
	typedef std::list<std::pair<expression_functor*, expression_functor*> > triggers_t;
	triggers_t triggers;
	
	template <typename Cond, typename Stmt>
	void addTrigger(const Cond cond, const Stmt stmt)
	{
		triggers.push_back(triggers_t::value_type(
			new expression_functor_wrapper<Cond, typename Cond::object_type>(cond),
			new expression_functor_wrapper<Stmt, typename Stmt::object_type>(stmt)
		));
	}
};

/**
 * Why constraint is just a proxy to the actual implementation?
 * Because we can force to compile-time analysis on template parameters,
 * and force them to be constraint of any implementation.
 * For example in operators.
 */
template <typename Impl>
struct constraint: abstract_constraint
{
	Impl impl_;
	
	virtual void operator()(abstract_field* fld, table* tbl, abstract_dbset* set)
	{
		impl_(fld, tbl, set);
	};
	
	template <typename T>
	constraint_expr operator| (constraint<T>& other)
	{
		return constraint_expr(this) + other;
	}
};

/* end */

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

	virtual bool operator==(abstract_field& other)
	{
		return this->value_ == dynamic_cast<field&>(other).value_;
	}
	
	bool operator==(value_type value)
	{
		return value_ == value;
	}
	
	operator T() const
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
struct dbset: abstract_dbset
{
	std::list<T> rows_;
	
	typedef cursor_impl<std::list<T> > cursor;
	
	dbset()
	{
		
	}
	
	void put(T t)
	{
		for (typename T::fields_t::iterator it(t.fields_.begin()),
			end(t.fields_.end()); it != end; ++it)
		{
			(*it)->constraint(*it, &t, this);
		}
		
		for (typename T::triggers_t::iterator it(t.triggers.begin()),
			end(t.triggers.end()); it != end; ++it)
		{
			if ((*it->first)(&t))
				((*it->second)(&t));
		}
		
		
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
		for (typename std::list<T>::iterator it(rows_.begin()),
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
		for (typename std::list<T>::iterator it(rows_.begin()),
			end(rows_.end()); it != end; it++)
		{
			stmt(it);
		}
	}
	
	std::list<T>& all()
	{
		return rows_;
	}
	
	virtual unsigned int size() const { return rows_.size(); }
	
	/* Object exists in set? */
	virtual bool exists(table* obj)
	{
		bool found = false;
		T* evaluated = static_cast<T*>(obj);
		for (typename std::list<T>::iterator it(rows_.begin()),
			end(rows_.end()); it != end; ++it)
		{
			if (*it == *evaluated)
			{
				found = true;
				break;
			}
		}
		return found;
	}
};

struct dbcontext
{
	/* TODO: database context here */
};

/* Useful macros 
 * @note evaluated_type is not an macro, its actual type of struct
 * with all template parameters typed in.
 * 
 * template <typename A1, typename A2, ..., typename An>
 * struct dummy_impl
 * {
 *     typedef dummy_impl<A1, A2, ..., An> evaluated_type;
 * };
 */
#define __CAT(a, b) a##b
#define _CAT(a, b) __CAT(a, b)
#define IMPLEMENT_OPERATOR_UNIQ(operator_impl, canonical_name, uniq) \
	template <typename _CAT(F, uniq)> \
	operator_impl<evaluated_type, _CAT(F, uniq)> operator canonical_name(_CAT(F, uniq) f) \
	{ \
		return operator_impl<evaluated_type, _CAT(F, uniq)>(*this, f); \
	}
	
/* In case of already used template names we use __COUNTER__ to
 * ensure the template name will be unique. */
#define IMPLEMENT_OPERATOR(operator_impl, canonical_name) \
	IMPLEMENT_OPERATOR_UNIQ(operator_impl, canonical_name, __COUNTER__)

/**
 * Chain operator implementation
 * This is not static operator because of possible compilation failure
 */
template <typename T1, typename T2>
struct chain_impl
{
	typedef chain_impl<T1, T2> evaluated_type;
	typedef typename T1::object_type object_type;
	
	chain_impl(T1 t1, T2 t2): t1_(t1), t2_(t2) {}
	T1 t1_;
	T2 t2_;
	template <typename F1>
	bool operator()(F1 f1)
	{
		t1_(f1);
		t2_(f1);
		return true;
	}
};

/**
 * Implementation of operator& (logical AND)
 */
template <typename T1, typename T2>
struct and_impl
{
	typedef and_impl<T1, T2> evaluated_type;
	
	T1 expr_;
	T2 value_;
	
	and_impl(T1 t, T2 value): expr_(t), value_(value) {}
	
	template <typename T>
	bool operator()(T obj)
	{
		return expr_(obj) && value_(obj);
	}
	
	/* Ops */
	IMPLEMENT_OPERATOR(and_impl, &)
};

/**
 * Assignment operator implementation
 * Can not be static
 */
template <typename T1, typename T2>
struct assign_impl
{
	typedef assign_impl<T1, T2> evaluated_type;
	typedef typename T1::object_type object_type;
	
	assign_impl(T1 t1, T2 t2): t1_(t1), t2_(t2) {}
	T1 t1_;
	T2 t2_;
	
	template <typename F1>
	bool operator()(F1 f1)
	{
		t1_(f1) = t2_(f1);
		return true;
	}
	
	template <typename A1, typename A2>
	chain_impl<assign_impl<T1, T2>, assign_impl<A1, A2> > operator,(assign_impl<A1, A2> f)
	{
		return chain_impl<assign_impl<T1, T2>, assign_impl<A1, A2> >(*this, f);
	}
};

/**
 * Implementation of operator== (logical AND)
 */
template <typename T1, typename T2>
struct eq_impl
{
	typedef eq_impl<T1, T2> evaluated_type;
	typedef typename T1::object_type object_type;
	
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
	
	/* Ops */
	IMPLEMENT_OPERATOR(and_impl, &)
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
	typedef gt_impl<T1, T2> evaluated_type;
	
	typedef typename T1::object_type object_type;
	
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
	
	/* Ops */
	IMPLEMENT_OPERATOR(and_impl, &)
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
 * This class will be evaluated later to value from member class field.
 */
template <typename T1, typename T2>
struct field_impl
{
	typedef field_impl<T1, T2> evaluated_type;
	typedef T2 object_type;
	
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
	
	/* Ops */
	IMPLEMENT_OPERATOR(eq_impl, ==)
	IMPLEMENT_OPERATOR(and_impl, &)
	IMPLEMENT_OPERATOR(lt_impl, <)
	IMPLEMENT_OPERATOR(gt_impl, >)
	IMPLEMENT_OPERATOR(plus_impl, +)
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

/* Constraints implementations */

struct auto_increment_impl: abstract_constraint
{
	virtual void operator()(abstract_field* fld, table*, abstract_dbset* set)
	{
		*dynamic_cast<field<int>*>(fld) = set->size() + 1;
	}
};

static constraint<auto_increment_impl> auto_increment;
