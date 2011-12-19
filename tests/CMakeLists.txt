CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

PROJECT (tests)
ADD_EXECUTABLE (tests
	main.cpp)

PROJECT (db)
ADD_EXECUTABLE (db
	db.cpp)

PROJECT (constraint)
ADD_EXECUTABLE (constraint
	constraint.cpp)
