#include <iostream>
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
	person(int id, const string& first_name, const string& second_name) :
		table("person"), id(this, "id", id),
		first_name(this, "first_name", first_name),
		second_name(this, "second_name", second_name) {}
	friend ostream& operator<<(ostream& out, const person& p)
	{
		out << "person(" << p.id << ",\"" <<
			p.first_name << "\", \"" <<
			p.second_name << "\")";
		return out;
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
	ctx.persons.put(person(1, "first_name1", "second_name2"));
	ctx.persons.put(person(2, "first_name2", "second_name2"));
	ctx.persons.put(person(3, "first_name3", "second_name3"));
	{
		dbset<person>::cursor cur(ctx.persons.all());
		assert((*cur).first_name.value_ == "first_name1");
		assert((*(++cur)).first_name.value_ == "first_name2");
		assert((*(++cur)).first_name.value_ == "first_name3");
	}
	
	ctx.persons.update(
		(F(&person::id) > 1) & (F(&person::id) < 3), ( /* Where */
		F(&person::first_name) = "John", F(&person::second_name) = "Appleseed"
	));
	
	{
		dbset<person>::cursor cur(ctx.persons.all());
		++cur;
		assert((*cur).first_name.value_ == "John");
		assert((*cur).second_name.value_ == "Appleseed");
	}
	
	return 0;
}
