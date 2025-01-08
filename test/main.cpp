#include <iostream>
#include <unistd.h>

void my_func(int i)
{
    if (i == 0) std::cout << "a";
    else if (i == 3) i = i;
    else if (i == 4) i = i;
}

void another_func(int a, int b, int c)
{
	std::cout << "hey func\n" << a << b << c;
}

int main()
{
	for (int i = 0; i < 3; i++)
	{
	    sleep(1);
	    another_func(0, 1, 2);
	    my_func(i);
	}
	return 0;
}

