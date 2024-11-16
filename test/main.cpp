#include <iostream>
#include <unistd.h>

extern "C" void my_func() { std::cout << "a"; }

int main()
{
	sleep(1);
	my_func();
	return 0;
}

