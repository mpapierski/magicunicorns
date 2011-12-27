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
		id.constraint = auto_increment;
		assert(id.constraint.size() == 1);
		/* TODO: Implement
		 * this->first_name.constraint = unique;
		 * this->second_name.constraint = unique;
		 * assert(this->first_name.constraint.size() == 1);
		 * assert(this->second_name.constraint.size() == 1); */
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
};

int
main(int argc, char* argv[])
{
	context ctx;
	ctx.persons.put(person("aaa", "bbb"));
	ctx.persons.put(person("ccc", "ddd"));
	ctx.persons.put(person("ccc", "ddd"));
	
	{
		dbset<person>::cursor cur(ctx.persons.all());
		assert((*cur).id == 1);
		assert((*(++cur)).id == 2);
		assert((*(++cur)).id == 3);
	}
	return 0;
}
