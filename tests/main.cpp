#include <string>
#include <magicunicorns.hpp>

using namespace std;

struct person: table
{
	field<int> id_;
	field<string> first_name_;
	field<string> second_name_;
	person(int id, const std::string& first_name, const std::string& second_name) :
		table("person"), id_(this, "id", id),
		first_name_(this, "first_name", first_name),
		second_name_(this, "second_name", second_name) {}
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
	return 0;
}
