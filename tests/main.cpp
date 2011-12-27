#include <string>
#include <cassert>
#include <magicunicorns.hpp>

using namespace std;

struct person: table
{
	field<int> id;
	field<string> first_name;
	field<string> second_name;
	person(int id, const string& first_name, const string& second_name) :
		table("person"), id(this, "id", id),
		first_name(this, "first_name", first_name),
		second_name(this, "second_name", second_name)
	{
	}
	
	friend ostream& operator<<(ostream& out, const person& p)
	{
		out << "person(" << p.id << ",\"" <<
			p.first_name << "\", \"" <<
			p.second_name << "\")";
		return out;
	}
	
	bool operator==(person& other)
	{
		return (id == other.id)	&& (first_name == other.first_name) && (second_name == other.second_name);
	}
};

struct manager: dbcontext
{
	dbset<person> persons;
};

int
main(int argc, char* argv[])
{
	manager m;
	m.persons.put(person(1, "John", "Smith"));
	m.persons.put(person(2, "Jan", "Kowalski"));
 	m.persons.put(person(3, "hello", "world"));
	m.persons.put(person(4, "asdf", "zxcv"));
	m.persons.put(person(5, "qwer", "1234"));
	assert( (m.persons.filter(
		(F(&person::id) > 0) &
		(F(&person::first_name) == "John") &
		(F(&person::second_name) == "Smith")).size() == 1));
	return 0;
}
