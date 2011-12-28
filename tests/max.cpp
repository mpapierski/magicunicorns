#include <iostream>
#include <string>
#include <cassert>
#include <magicunicorns.hpp>

using namespace std;

/**
 * Person
 */
struct person: table
{
	field<int> id;
	field<string> first_name;
	field<string> second_name;
	person(const string& first_name, const string& second_name) :
		table("person"), id(this, "id"),
		first_name(this, "first_name", first_name),
		second_name(this, "second_name", second_name)
	{
		addTrigger(F(&person::id) == 0, F(&person::id) = MAX(F(&person::id)) + val(1));
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

struct context: dbcontext
{
	dbset<person> persons;
	context(): persons(this) {}
};

static context ctx;

int
main(int argc, char *argv[])
{
	ctx.persons.put(person("null", "null"));
	ctx.persons.put(person("null", "null"));
	ctx.persons.put(person("null", "null"));
	dbset<person>::cursor cur(ctx.persons.all());
	assert((*cur).id == 1);
	++cur;
	assert((*cur).id == 2);
	++cur;
	assert((*cur).id == 3);
	return 0;
}

