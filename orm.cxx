#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <cassert>

using namespace std;

struct abstract_field
{
	virtual string type() = 0;
	virtual string name() = 0;
};

struct table
{
	table(const std::string& tablename) :
		tablename_(tablename) {}
	
	void add_field(abstract_field* field)
	{
		fields_.push_back(pair<string, string>(field->name(), field->type()));
	}
	
	typedef vector<pair<string, string> > fields_t;
	fields_t fields_;
	string tablename_;
};

/* 
 * okreslanie typu
 * potrzebne do serializacji dokumentu
 */
template <typename T>
struct get_type;

template <>
struct get_type<string> { string value() const { return "TEXT"; } };

template <>
struct get_type<int> { string value() const { return "INTEGER"; } };

template <typename T>
struct field: abstract_field
{
	typedef T value_type;
	
	field(table* parent, string name, const value_type& value = value_type()):
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
	
	friend ostream& operator<<(ostream& out, const field& fld)
	{
		out << fld.value_;
		return out;
	}
	
	string name_;
	value_type value_;
	get_type<T> type_;
	
	virtual string name() { return name_; }
	virtual string type() { return type_.value(); }
};

struct people: table
{
	field<int> people_id;
	field<string> first_name;
	field<string> second_name;
	
	people(int id, string first_name, string second_name) :
		table("people"),
		people_id(this, "people_id", id),
		first_name(this, "first_name", first_name),
		second_name(this, "second_name", second_name) {}
	
	friend ostream& operator<<(ostream& out, const people& p)
	{
		out << "people(" << p.people_id << ", " << p.first_name << ", " << p.second_name << ")";
		return out;
	}
};

template <typename T /* Container */>
struct cursor_impl
{
	cursor_impl(T t):
		container_(t),
		it_(container_.begin())
	{
	}
	
	operator bool()
	{
		return it_ != container_.end();
	}
	
	cursor_impl& operator++()
	{
		++it_;
		return *this;
	}
	
	typename iterator_traits<typename T::iterator>::value_type operator*()
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
	list<T> rows_;
	
	typedef cursor_impl<list<T> > cursor;
	
	dbset()
	{
		
	}
	
	void put(T t)
	{
		rows_.push_back(t);
	}
	
	template <typename F>
	list<T> find(F f)
	{
		list<T> results;
		
		for (typename list<T>::iterator it = rows_.begin(),
			end(rows_.end()); it != end; it++)
		{
			if (f(it))
				results.push_back(*it);
		}
		
		return results;
		
	}
};

struct dbcontext
{
	/* TODO: Kontekst */
};

struct manager: dbcontext
{
	dbset<people> peoples;
};

template <typename T1, typename T2>
struct field_impl
{
	typedef T1 value_type;
	typedef field<T1> T2::* member_field_type;
	typedef field<T1> field_type;
	
	field<T1> T2::* field_;
	field_impl(field<T1> T2::* t): field_(t) {}
	
	template <typename F1>
	T1 operator()(F1 obj)
	{
		return (*obj.*field_).value_;
	}
};

template <typename T1, typename T2>
struct eq_impl
{
	T1 expr_;
	T2 value_;
	
	eq_impl(T1 t, T2 value): expr_(t), value_(value) {}
	
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


/* 
 * TODO: Zamiast eq_impl zrobic field_impl
 * aby mozna bylo budowac cale wyrazenia jak w boost.phoenix.
 * np wyrazenie F(&tabla::pole1) == 123 && F(&tabla::pole2) == 456 
 * powinno zwrocic typ expr<and_impl<eq_impl<...>, eq_impl<...> > >
 */
template <typename T1, typename T2>
field_impl<T1, T2> F(field<T1> T2::* fld)
{
	return field_impl<T1, T2>(fld);
}

//template <typename FieldT>
template <typename FieldT, typename ValueT>
struct expr
{
	FieldT field_;
	ValueT value_;
	
	expr(FieldT fld, ValueT value):
		field_(fld),
		value_(value) {}
	
	template <typename T1>
	bool operator()(T1 object)
	{
		/* 
		 * field_ to implementacja naszego operatora
		 * a wiec przekazmy tam spowrotem nasz obiekt i wartosc
		 * aby nie duplikowac kodu wszystkich operatorów.
		 */
		return expr_(object);
	}
};

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


int
main(int argc, const char* argv[])
{
	{
		people p1(123, "Janusz", "Paweliusz");
		
		F(&people::people_id);
		F(&people::people_id)(&p1);
		
		F(&people::people_id) == 123;
		
		assert( (F(&people::people_id)(&p1) ) == 123);
		assert(! ((F(&people::people_id)(&p1) ) == 124));
		
		F(&people::first_name);
		F(&people::first_name)(&p1);
		F(&people::first_name) == "asdf";
		(F(&people::first_name) == "asdf")(&p1);
		assert(( (F(&people::first_name) == "Janusz")(&p1) ) == true);
	}
	
	{
		people p1(123, "asdf", "zxcv");
		((F(&people::first_name) == "asdf") & (F(&people::second_name) == "zxcv"));
		assert((((F(&people::people_id) == 123) & (F(&people::first_name) == "asdf") & (F(&people::second_name) == "zxcv"))(&p1)) == true);
		assert((((F(&people::first_name) == "asdf") & (F(&people::second_name) == "zxcv"))(&p1)) == true);
		assert(!(((F(&people::first_name) == "asdf") & (F(&people::second_name) == "asdf"))(&p1)));
	}
	
	{
		manager mgr;
	
		mgr.peoples.put(people(1, "Jan", "Kowalski"));
		mgr.peoples.put(people(2, "asdf", "zxcv"));
		mgr.peoples.put(people(3, "qwer", "sgbmkfgmbks"));
		mgr.peoples.put(people(4, "qwer2", "qwererywer"));
		mgr.peoples.put(people(5, "qwer3", "qwererywer"));
		
		assert(mgr.peoples.find(F(&people::people_id) == 1).size() == 1);
		assert(mgr.peoples.find(F(&people::people_id) == 2).size() == 1);
		assert(mgr.peoples.find(F(&people::people_id) == 3).size() == 1);
		
		assert(mgr.peoples.find(F(&people::first_name) == std::string("Jan")).size() == 1);
		assert(mgr.peoples.find(F(&people::first_name) == "zxcv").size() == 0);
		
		assert(mgr.peoples.find(
			(F(&people::people_id) == 5) &
			(F(&people::first_name) == "qwer3") &
			(F(&people::second_name) == "qwererywer")
		).size() == 1);
		
		assert(mgr.peoples.find(
			(F(&people::people_id) > 1)
		).size() == 4);
		
		assert(mgr.peoples.find(
			(F(&people::people_id) > 1) &
			(F(&people::people_id) < 5)
		).size() == 3);
		
		assert(mgr.peoples.find(
			(F(&people::people_id) > 1) &
			(F(&people::people_id) < 5) &
			(F(&people::first_name) == "asdf")
		).size() == 1);
		
		assert(mgr.peoples.find(
			(F(&people::people_id) != 1) &
			(F(&people::people_id) != 2)
		).size() == 3);
		
		for (dbset<people>::cursor c(mgr.peoples.find(F(&people::people_id) > 1)); c; ++c)
		{
			cout << *c << endl;
		}
	}
	return 0;
}
