#include "URotomApplication.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
	URotomApplication app;

	if (!app.Setup()) {
		return 0;
	}

	app.Run();

	if (!app.Teardown()) {
		return 0;
	}
}