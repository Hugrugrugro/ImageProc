#include <iostream>

#include "opencv2/highgui.hpp"
#include "opencv2/videoio.hpp"

#include "ImageProcessing.h"
#include "SignalProcessing.h"

#define FPS 15.0
#define BUFFER_DURATION 10 // in seconds
#define BUFFER_SHIFT 1 // in seconds
#define DISCARD_DURATION 5
#define USE_ICA true // Use ICA for improved signal extraction
#define LOW_FREQ 50.0
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
cv::Rect foreheadROI;
cv::Mat frame_face;
cv::Mat frame_forehead;
// buffer - RGB channels for ICA
bool isInProcess = true;
bool isBufferFull = false;
std::deque<float> redSignal;
std::deque<float> greenSignal;
std::deque<float> blueSignal;
std::vector<float> processedSignal;
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
    if (USE_ICA)
        std::cout << "[INFO] Using ICA for improved signal extraction" << std::endl;
        
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
            if (countDiscard == 0)
            {
                std::cout << "[INFO] Discarding data during " << DISCARD_DURATION << " seconds" << std::endl;
                std::cout << "--> Let the camera perform auto white balance... ";
            }
            // 5- Increments the counter 
            countDiscard++;
            // 6- Checks stopping criterion 
            if (countDiscard == DISCARD_DURATION * FPS)
            {
                isDataDiscarded = false;
                std::cout << "DONE!" << std::endl;
            }
            
            cv::putText(frame, "Discarding images during auto white balancing", cv::Point(10, 20), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
            cv::imshow("Raw img", frame);
            
            // Waits the necessary amount of time to achieve the specified frame rate
            if (cv::waitKey(1000.0 / FPS) >= 0)
                isInProcess = false;
        }
        
        // Detects the principal face in the image and determines its location
        while (!isFaceDetected)
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
                // 6- Extracts ROI for forehead
                foreheadROI = extractForeheadROI(faceRectangles);
                
                std::cout << "--> " << faceRectangles.size() << " face(s) detected!" << std::endl;
                std::cout << "--> Forehead ROI extracted" << std::endl;
                
                // 7- Changes boolean value now that the face was detected
                isFaceDetected = true;
            }
            
            // Waits the necessary amount of time to achieve the specified frame rate
            if (cv::waitKey(1000.0 / FPS) >= 0)
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
            frame_forehead = frame(foreheadROI);
            
            // 6- Extract RGB channels from forehead ROI
            cv::Vec3f rgbMean = extractMeanRGB(frame, foreheadROI);
            
            // 7- Store RGB values in separate buffers
            redSignal.push_back(rgbMean[0]);
            greenSignal.push_back(rgbMean[1]);
            blueSignal.push_back(rgbMean[2]);
            
            // 8- Display the current buffer size in %
            int currentProcessingPercentage = static_cast<int>((static_cast<float>(redSignal.size()) / (FPS * BUFFER_DURATION)) * 100);
            if (currentProcessingPercentage != previousProcessingPercentage)
                std::cout << "[INFO] Buffer size = " << currentProcessingPercentage << "%\r" << std::flush; 
            previousProcessingPercentage = currentProcessingPercentage;
            
            // 9- Sets a stopping criterion
            if (redSignal.size() == static_cast<size_t>(FPS * BUFFER_DURATION))
            {
                isBufferFull = true;
            }
            
            // 10- Displays images (face, forehead)
            cv::imshow("Face img", frame_face);
            cv::imshow("Forehead img", frame_forehead);
            
            // 11- Adds face rectangle on raw image
            cv::rectangle(frame, faceRectangles[0], cv::Scalar(0, 255, 0), 2);
            
            // 12- Adds forehead rectangle on raw image
            cv::rectangle(frame, foreheadROI, cv::Scalar(255, 0, 0), 2);
            
            // 13- Adds instructions and results on raw image
            std::string str2Display = "Heart rate: " + std::to_string(heartRateBPM) + " BPM";
            if (heartRateBPM != -1)
                cv::putText(frame, str2Display, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2);
            else
                cv::putText(frame, "Estimating HR, Stay still!", cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
            
            // 14- Displays raw image
            cv::imshow("Raw img", frame);
            
            // Waits the necessary amount of time to achieve the specified frame rate
            if (cv::waitKey(1000.0 / FPS) >= 0)
                isInProcess = false;
        }
        
        std::cout << std::endl; // New line after buffer progress
        
        // Process signal using ICA or single channel
        if (USE_ICA)
        {
            std::cout << "[INFO] Processing signal with ICA..." << std::endl;
            
            // Apply ICA to extract the best component from RGB signals
            processedSignal = computeICA(redSignal, greenSignal, blueSignal);
        }
        else
        {
            std::cout << "[INFO] Processing signal with green channel only..." << std::endl;
            
            // Use green channel only (traditional approach)
            processedSignal.assign(greenSignal.begin(), greenSignal.end());
        }
        
        // Compute avg and std of processed signal
        std::deque<float> processedSignalDeque(processedSignal.begin(), processedSignal.end());
        float avg_signal = computeTemporalAverage(processedSignalDeque);
        float std_signal = computeTemporalStd(processedSignalDeque);
        
        // Normalize the temporal signal
        std::vector<float> normalizedSignal = normalizeTemporalSignal(processedSignalDeque, avg_signal, std_signal);
        
        // Display normalized signal
        int yRange = static_cast<int>(FPS * BUFFER_DURATION);
        cv::Mat signalPlot = plotGraph(normalizedSignal, yRange, 0, -1);
        cv::imshow("Processed Signal", signalPlot);
        
        // Compute the power spectrum of the normalized signal
        std::vector<float> powerSpectrum = computeFourierTransform(normalizedSignal);
        
        if (!powerSpectrum.empty())
        {
            // Display Fourier transform
            int lowIdx = static_cast<int>(LOW_FREQ / 60.0f * powerSpectrum.size() / FPS);
            int highIdx = static_cast<int>(HIGH_FREQ / 60.0f * powerSpectrum.size() / FPS);
            cv::Mat fourierPlot = plotGraph(powerSpectrum, yRange, lowIdx, highIdx);
            cv::imshow("Power Spectrum", fourierPlot);
            
            // Determine the heart rate from power spectrum
            int hr_computed = computeHeartRate(powerSpectrum, LOW_FREQ, HIGH_FREQ, FPS);
            
            if (hr_computed > 0)
            {
                heartRateBPM = hr_computed;
                std::cout << "[INFO] Estimated HR = " << heartRateBPM << " BPM" << std::endl;
            }
            else
            {
                std::cout << "[WARNING] Unable to estimate HR from current data" << std::endl;
            }
        }
        
        // Remove data at the beginning of the deques to update the buffer
        int shiftSize = static_cast<int>(BUFFER_SHIFT * FPS);
        for (int l = 0; l < shiftSize; l++)
        {
            if (!redSignal.empty()) redSignal.pop_front();
            if (!greenSignal.empty()) greenSignal.pop_front();
            if (!blueSignal.empty()) blueSignal.pop_front();
        }
        isBufferFull = false;	
                
        // Waits the necessary amount of time to achieve the specified frame rate
        if (cv::waitKey(1000.0 / FPS) >= 0)
            isInProcess = false;
    }
    
    // Destroys all GUI
    cv::destroyAllWindows();
    
    // Releases the camera
    cap.release();
    
    return 0;
}