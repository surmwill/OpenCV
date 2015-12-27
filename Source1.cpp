#include <sstream>
#include <string>
#include <iostream>
#include <opencv\highgui.h>
#include <opencv\cv.h>
#include "Colour.h"
#include <vector>
#include <fstream>

using namespace cv;

//default capture width and height
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;

//max number of objects to be detected in frame
const int MAX_NUM_OBJECTS = 50;

//minimum and maximum object area
const int MIN_OBJECT_AREA = 40 * 40;
const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH / 1.5;

//our hue, saturation, and value variables, we can change theese via slider
int lowH = 52;
int highH = 88;
int lowS = 91;
int highS = 255;
int lowV = 0;
int highV = 255;



void on_trackbar(int, void*)
{
}

//integer to string
string intToString(int number){

	std::stringstream ss;
	ss << number;
	return ss.str();
}

//writes the data to a file
void writeToFile(int x, int y, int numDetected)
{
	ofstream file;
	file.open("data.txt");

	file << x << "\n" << y << "\n" << numDetected << endl;
	file.close();
}

//creates our trackbars or sliders for changing the variables
void createTrackbars(){

	namedWindow("Control", CV_WINDOW_AUTOSIZE);

	//Create trackbars in "Control" window
	cvCreateTrackbar("LowH", "Control", &lowH, 255); 
	cvCreateTrackbar("HighH", "Control", &highH, 255);

	cvCreateTrackbar("LowS", "Control", &lowS, 255); 
	cvCreateTrackbar("HighS", "Control", &highS, 255);

	cvCreateTrackbar("LowV", "Control", &lowV, 255); 
	cvCreateTrackbar("HighV", "Control", &highV, 255);


}

//Draw the circle in the center of the detected object and print out the pixel value, additionally check if it is centered 
void drawObject(int x, int y, Mat &frame, bool highest){

	if (highest == true)
	{
		cv::circle(frame, cv::Point(x, y), 10, cv::Scalar(0, 255, 0));
		if (x > 340) putText(frame, "move right", Point(0, 25), 1, 2, Scalar(0, 0, 255));
		else if (x < 300) putText(frame, "move left", Point(0, 24), 1, 2, Scalar(0, 0, 255));
		else putText(frame, "perfect", Point(0, 25), 1, 2, Scalar(0, 255, 0));
		
	}
	else
	{
		cv::circle(frame, cv::Point(x, y), 10, cv::Scalar(0, 0, 255));
	}

	cv::putText(frame, intToString(x) + " , " + intToString(y), cv::Point(x, y + 20), 1, 1, Scalar(0, 255, 0));

}

//erodes and dialates the image so we can get rid of noise
void morphOps(Mat &thresh){

	Mat erodeElement = getStructuringElement(MORPH_RECT, Size(3, 3));

	Mat dilateElement = getStructuringElement(MORPH_RECT, Size(9, 9));

	erode(thresh, thresh, erodeElement);
	erode(thresh, thresh, erodeElement);


	dilate(thresh, thresh, dilateElement);
	dilate(thresh, thresh, dilateElement);

}

void trackFilteredObject(Mat threshold, Mat HSV, Mat &cameraFeed){

	vector <Colour> greenP;
	bool highest = false;

	Mat temp;
	threshold.copyTo(temp);

	//these two vectors needed for output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;

	//find contours of filtered image using openCV findContours function
	findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

	//use moments method to find our filtered object
	double refArea = 0;
	bool objectFound = false;

	if (hierarchy.size() > 0) {
		int numObjects = hierarchy.size();

		//if number of objects greater than MAX_NUM_OBJECTS we have a noisy filter
		if (numObjects < MAX_NUM_OBJECTS){

			//go through each element in our frame
			for (int index = 0; index >= 0; index = hierarchy[index][0]) {

				Colour green;

				Moments moment = moments((cv::Mat)contours[index]);
				double area = moment.m00;

				//if the area is less than 20 px by 20px then it is probably just noise
				//if the area is the same as the 3/2 of the image size, probably just a bad filter
				//we only want the object with the largest area so we safe a reference area each
				//iteration and compare it to the area in the next iteration.
				if (area > MIN_OBJECT_AREA){

					green.setX(moment.m10 / area);
					green.setY(moment.m01 / area);

					greenP.push_back(green);


					objectFound = true;

				}
				else objectFound = false;


			}
			//let user know you found an object
			if (objectFound == true)
			{
				int leastY = greenP[0].getY();

				//check if it is the highest object in our frame
				for (int i = 0; i < greenP.size(); i++)
				{
					int currentY = greenP[i].getY();

					if (currentY < leastY || greenP.size() == 1)
					{
						leastY = currentY;
					}
				}

				//draw on our circle detecting the target as well as write it's properties to a file
				for (int i = 0; i < greenP.size(); i++)
				{
					if (leastY == greenP[i].getY())
					{
						drawObject(greenP[i].getX(), greenP[i].getY(), cameraFeed, true);
						writeToFile(greenP[i].getX(), greenP[i].getY(), greenP.size());
					}
					//draw object location on screen
					else drawObject(greenP[i].getX(), greenP[i].getY(), cameraFeed, false);
				}
			}

		}
		else putText(cameraFeed, "TOO MUCH NOISE! ADJUST FILTER", Point(0, 50), 1, 2, Scalar(0, 0, 255), 1);
	}
}

int main(int argc, char* argv[])
{

	//basic setup
	bool calibrationMode = true;
	
	Mat cameraFeed;
	Mat threshold;
	Mat HSV;

	if (calibrationMode){
		
		createTrackbars();
	}
	
	VideoCapture capture;

	capture.open(0);
	capture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
	
	while (1){
		
		//grabs our frame from the webcam 
		capture.read(cameraFeed);
		
		//the rest just creates windows for filtering and calls the functions above
		cvtColor(cameraFeed, HSV, COLOR_BGR2HSV);

		if (calibrationMode == true){

			cvtColor(cameraFeed, HSV, COLOR_BGR2HSV);

			inRange(HSV, Scalar(lowH, lowS, lowV), Scalar(highH, highS, highV), threshold);
			morphOps(threshold);
			imshow("Thresholded Image", threshold);

			trackFilteredObject(threshold, HSV, cameraFeed);
		}

		imshow("Original Image", cameraFeed);

		int c = cvWaitKey(10);
		if ((char)c == 27) break;

		waitKey(30);
	}


	cvDestroyAllWindows();



	return 0;
}
