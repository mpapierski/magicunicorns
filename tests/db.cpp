#include <iostream>
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
		this->first_name.constraint = ::uppercase;
		this->second_name.constraint = ::lowercase;
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

void menu()
{
	cout << "add\tadd new person" << endl <<
		"list\tlist persons" << endl <<
		"filter\tfilter by name" << endl;
}

int
main(int argc, char* argv[])
{
	context ctx;

	cout << "magicunicorns testdb" << endl;

	while (true)
	{
		menu();
		string input;
		getline(cin, input);
		if (input == "add")
		{
			/* Ask for new data */
			string first_name, second_name;
			cout << "first_name: ";
			getline(cin, first_name);
			cout << "second_name: ";
			getline(cin, second_name);
			ctx.persons.put(person(first_name, second_name));	
		}
		else if (input == "list")
		{
			/* List all data */
			cout << "---" << endl;
			for (dbset<person>::cursor cur(ctx.persons.all()); cur; ++cur)
			{
				cout << (*cur) << endl;
			}
			cout << "---" << endl <<
				"total: " << ctx.persons.all().size() << endl;
		}
		else if (input == "filter")
		{
			/* Filter by query */
			string first_name, second_name;
			cout << "first_name: ";
			getline(cin, first_name);
			cout << "second_name: ";
			getline(cin, second_name);
			cout << "---" << endl;
			unsigned int total = 0;
			for (dbset<person>::cursor cur(ctx.persons.filter(
					(F(&person::first_name) == first_name) & (F(&person::second_name) == second_name) /* Magic! */
				)); cur; ++cur)
			{
				cout << (*cur) << endl;
				total++; /* TODO: Cursor should be able to know result set size */
			}
			cout << "---" << endl <<
				"total: " << total << endl;
		}

		cout << endl;
	}
	return 0;
}
