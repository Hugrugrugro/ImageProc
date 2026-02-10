#include <iostream>

#include "opencv2/highgui.hpp"
#include "opencv2/videoio.hpp"

#include "ImageProcessing.h"
#include "SignalProcessing.h"

#define FPS 15.0
#define BUFFER_DURATION 10 // in seconds
#define BUFFER_SHIFT 1 // in seconds
#define DISCARD_DURATION 5
#define CHANNEL_OF_INTEREST 1
# define LOW_FREQ 50.0
#define HIGH_FREQ 150.0

//==GLOBAL VARIABLES==/
// face detector
cv::CascadeClassifier faceDetector; // face detector based on Haar Cascade
std::string sHaarCascadeFilename = "../data/haarcascade_frontalface_alt.xml";
// video capture
cv::VideoCapture cap; // see https://docs.opencv.org/3.4/d8/dfe/classcv_1_1VideoCapture.html
int deviceID = 0;             // 0 = open default camera
int apiID = cv::CAP_ANY;      // 0 = autodetect default API
// discard data because of white auto-balancing
bool isDataDiscarded = true;
int countDiscard = 0;
// face detection -> ROI
bool isFaceDetected = false;
std::vector<cv::Rect> faceRectangles;
cv::Mat frame_face;
// buffer
bool isInProcess = true;
bool isBufferFull = false;
std::deque<float> faceSignal;
std::vector<float> faceSignalNormalized;
int heartRateBPM = -1;


int main()
{	
	//=========INITIALISATION=========//
	
	//==FACE DETECTOR==/
	// 1- Creates a face detector based on Haar Cascade
	bool bIsFaceDetectorLoaded = loadFaceDetector(faceDetector, sHaarCascadeFilename);
	
	// 2- Checks if the face detector was successfully created
	if (!bIsFaceDetectorLoaded)
		return 1;
		
	//==VIDEOCAPTURE==/
	// 1- Opens selected camera using selected API
	cap.open(deviceID, apiID);
	
	// 2- Check if the cam was successfully opened
	if (!cap.isOpened()) 
	{
		std::cerr << "[ERROR] Unable to open camera!" << std::endl;
		return 2;
	}
	
	//--- GRAB AND WRITE LOOP
	std::cout << "[INFO] Start grabbing images" << std::endl;
	std::cout << "--> Stay still!" << std::endl;
	std::cout << "--> Press any key to terminate" << std::endl;
		
	while (isInProcess)
	{
		// Discards data while the camera is performing auto white balancing
		while (isDataDiscarded)
		{
			// 1- Creates a matrix to store the image from the cam
			cv::Mat frame;
			// 2- Waits for a new frame from camera and store it into 'frame'
			cap.read(frame);
			// 3- Checks if the frame is not empty
			if (frame.empty()) 
			{
				std::cerr << "[ERROR] blank frame was grabbed -> skipping it!" << std::endl;
				break;
			}
			// 4- During the 1st frame, display some info on the screen
			if (countDiscard==0)
			{
				std::cout << "[INFO] Discarding data during " << DISCARD_DURATION << " seconds" << std::endl;
				std::cout << "--> Let the camera perform auto white balance... ";
			}
			// 5- Increments the counter 
			countDiscard++;
			// 6- Checks stopping criterion 
			if (countDiscard == DISCARD_DURATION*FPS)
			{
				isDataDiscarded = false;
				std::cout << "DONE!" << std::endl;
			}
			
			cv::putText(frame, "Discarding images during auto white balancing", cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
			cv::imshow("Raw img", frame);
			
			// Waits the necessary amount of time to achieve the specified frame rate
			if (cv::waitKey(1000.0/FPS) >= 0)
				isInProcess = false;
		}
		
		// Detects the principal face in the image and determines its location
		while(!isFaceDetected)
		{
			// 1- Creates a matrix to store the image from the cam
			cv::Mat frame;
			// 2- Waits for a new frame from camera and store it into 'frame'
			cap.read(frame);
			// 3- Checks if the frame is not empty
			if (frame.empty()) 
			{
				std::cerr << "[ERROR] blank frame was grabbed -> skipping it!" << std::endl;
				break;
			}
			
			std::cout << "[INFO] Looking for faces..." << std::endl;
			
			// 4- Detect face(s)
			faceRectangles = detectFace(faceDetector, frame);
			// 5- Checks if a face was detected
			if (faceRectangles.size() > 0)
			{
				// 6- Extracts image around the forehead
				// TO BE DONE
				
				std::cout << "--> " << faceRectangles.size() << " face(s) detected!" << std::endl;
				
				// 7- Changes boolean value now that the face was detected
				isFaceDetected = true;
			}
			
			// Waits the necessary amount of time to achieve the specified frame rate
			if (cv::waitKey(1000.0/FPS) >= 0)
				isInProcess = false;
		}
		
		// Fills a buffer of data before estimating HR
		int previousProcessingPercentage = 0;
		
		while (!isBufferFull)
		{
			// 1- Creates a matrix to store the image from the cam
			cv::Mat frame; 
			// 2- Waits for a new frame from camera and store it into 'frame'
			cap.read(frame);
			// 3- Checks if the frame is not empty
			if (frame.empty()) 
			{
				std::cerr << "[ERROR] blank frame was grabbed -> skipping it!" << std::endl;
				break;
			}
			// 4- Creates a cropped image around the face
			frame_face = frame(faceRectangles[0]);
			// 5- Creates a cropped image around the forehead
			// TO BE DONE
			// 6- Computes the average value of the face ROI
			cv::Scalar avg_face = mean(frame_face);
			// 7- Computes the average value of the forehead ROI
			// TO BE DONE
			// 8- Saves the average value for the face/forehead for a given channel (red=2,green=1, blue=0)
			faceSignal.push_back(avg_face[CHANNEL_OF_INTEREST]);
			// TO BE DONE
			// 10- Displays the current buffer size in %
			int currentProcessingPercentage = (float)faceSignal.size() / (FPS*BUFFER_DURATION) * 100;
			if (currentProcessingPercentage != previousProcessingPercentage)
				std::cout << "[INFO] Buffer size = " << currentProcessingPercentage << "%\r"; 
			previousProcessingPercentage = currentProcessingPercentage;
			
			//std::cout << "faceSignal.size() = " << faceSignal.size() << "/" << FPS*BUFFER_DURATION<< std::endl;
			
			// 11- Sets a stopping criterion
			if (faceSignal.size() == FPS*BUFFER_DURATION)
			{
				isBufferFull = true;
			}
			
			// 12- Displays images (face, forehad)
			cv::imshow("Face img", frame_face);
			// TO BE DONE
			
			// 13- Adds face rectangle on raw image
			cv::rectangle(frame, faceRectangles[0], cv::Scalar(0, 0, 255), 1, 1, 0);
			// 14- Adds forehead rectangle on raw image
			// TO BE DONE
			// 15- Adds instructions and results on raw image
			std::string str2Display= "Heart rate: " + std::to_string(heartRateBPM);
			if (heartRateBPM != -1)
				cv::putText(frame, str2Display, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
			else
				cv::putText(frame, "Estimating HR, Stay still!", cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
			// 16- Displays images raw
			cv::imshow("Raw img", frame);
			
			// Waits the necessary amount of time to achieve the specified frame rate
			if (cv::waitKey(1000.0/FPS) >= 0)
				isInProcess = false;
		}
		
		// Computes avg and std
		float avg_face = computeTemporalAverage(faceSignal);
		// TO BE DONE
		float std_face = computeTemporalStd(faceSignal);
		// TO BE DONE
		
		// Normalizes the temporal signal
		std::vector<float> faceSignalNormalized = normalizeTemporalSignal(faceSignal, avg_face, std_face);
		// TO BE DONE
		
		// Displays normalized signal
		int yRange = FPS*BUFFER_DURATION;
		cv::imshow("Face signal", plotGraph(faceSignalNormalized, yRange));
		// TO BE DONE
		
		// Computes the power spectrum of the normalized signals
		std::vector<float> powerSpectrum_face = computeFourierTransform(faceSignalNormalized);
		// TO BE DONE
		
		// Displays Fourier transform
		cv::imshow("Face Fourier", plotGraph(powerSpectrum_face, yRange, LOW_FREQ/60*powerSpectrum_face.size()/FPS, HIGH_FREQ/60*powerSpectrum_face.size()/FPS));
		// TO BE DONE
		
		// Determines the maximum of the power spectrum in a given frequency range
		// Deduces the heart rate value
		int hr_face = computeHeartRate(powerSpectrum_face, LOW_FREQ, HIGH_FREQ, FPS);
		// TO BE DONE
		heartRateBPM = hr_face;
		
		//std::cout << "HR_face = " << hr_face  << std::endl;
		
		// Removes data at the beginning of the deque to update the buffer
		for (int l = 0; l < BUFFER_SHIFT*FPS; l++)
		{
			faceSignal.pop_front();
			// TO BE DONE
		}
		isBufferFull = false;	
				
		// Waits the necessary amount of time to achieve the specified frame rate
		if (cv::waitKey(1000.0/FPS) >= 0)
			isInProcess = false;
	}
	
	// Destroys all GUI
	cv::destroyAllWindows();
	
	// Releases the camera
	cap.release();
	
	return 0;
}
