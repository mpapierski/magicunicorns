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
			ctx.persons.put(person(ctx.persons.all().size() + 1, first_name, second_name));	
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
					F(&person::first_name) == first_name & F(&person::second_name) == second_name /* Magic! */
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
