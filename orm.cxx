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
};

template <typename T>
struct dbset
{
	list<T> rows_;
	typedef typename list<T>::iterator iterator;
	
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
struct eq_impl
{
	field<T1> T2::* field_; /* field data type */
	typedef T2 class_type_t; /* class type */
	
	eq_impl(field<T1> T2::* t): field_(t) {}
	
	template <typename F1, typename F2>
	bool operator()(F1 obj, F2 val)
	{
		/* wlasciwa implementacja operatora */
		return *obj.*field_ == val;		
	}
};

/* 
 * TODO: Zamiast eq_impl zrobic field_impl
 * aby mozna bylo budowac cale wyrazenia jak w boost.phoenix.
 * np wyrazenie F(&tabla::pole1) == 123 && F(&tabla::pole2) == 456 
 * powinno zwrocic typ expr<and_impl<eq_impl<...>, eq_impl<...> > >
 */
template <typename T1, typename T2>
eq_impl<T1, T2> F(field<T1> T2::* fld)
{
	return eq_impl<T1, T2>(fld);
}

template <typename FieldT, typename ValueT>
struct expr
{
	FieldT field_;
	ValueT value_;
	
	expr(FieldT fld, ValueT value):
		field_(fld),
		value_(value) {}
	
	template <typename T>
	bool operator()(T object)
	{
		/* 
		 * field_ to implementacja naszego operatora
		 * a wiec przekazmy tam spowrotem nasz obiekt i wartosc
		 * aby nie duplikowac kodu wszystkich operatorów.
		 */
		return field_(object, value_);
	}
};

template <typename T1, typename T2, typename T3>
expr<eq_impl<T1, T2>, T3> operator==(eq_impl<T1, T2> fld, T3 value)
{
	/* zwroc wyrazenie z wlasciwym operatorem */
	return expr<eq_impl<T1, T2>, T3>(fld, value);
}

int
main(int argc, const char* argv[])
{
	manager mgr;
	
	mgr.peoples.put(people(1, "Jan", "Kowalski"));
	mgr.peoples.put(people(2, "asdf", "zxcv"));
	
	assert(mgr.peoples.find(F(&people::people_id) == 1).size() == 1);
	assert(mgr.peoples.find(F(&people::people_id) == 2).size() == 1);
	assert(mgr.peoples.find(F(&people::people_id) == 3).size() == 0);
	
	assert(mgr.peoples.find(F(&people::first_name) == "Jan").size() == 1);
	assert(mgr.peoples.find(F(&people::first_name) == "zxcv").size() == 0);
	return 0;
}
