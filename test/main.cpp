#include <iostream>
#include <unistd.h>

extern "C" void my_func(int i)
{
    if (i == 0) std::cout << "a";
    else if (i == 3) i++;
    else if (i == 4) i++;
}

int main()
{
	for (int i = 0; i < 3; i++)
	{
	    // sleep(1);
	    my_func(i);
	}
	return 0;
}

