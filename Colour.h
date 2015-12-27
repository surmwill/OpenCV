#pragma once
#include <string>
using namespace std;

class Colour
{
public:

	//functions to set and get the x and y position of an element on screen
	void setX(int num);
	void setY(int num);
	int getX(void);
	int getY(void);
private:
	int xPos;
	int yPos;

};

