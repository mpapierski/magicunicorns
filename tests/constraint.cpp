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
	/*{
		context ctx;
		person p(1, "asdf", "zxcv");
		(unique | auto_increment)(p.id, ctx.persons);
		assert(p.id == 1);
		ctx.persons.put(p);
	}
	{
		context ctx;
		ctx.persons.put(person(1, "aaa", "bbb"));
		ctx.persons.put(person(2, "aaa", "bbb"));
		person not_uniq(2, "aaa", "bbb");
		try
		{
			(unique)(not_uniq, ctx.persons);
			assert(false);
		} catch (unique_constraint_violation& e)
		{
			assert(true);
		}
	}*/
	return 0;
}
