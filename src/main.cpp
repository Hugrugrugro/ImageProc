// main.cpp - Standalone version
#include <iostream>
#include <vector>
#include <deque>
#include "opencv2/highgui.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>

#define FPS 15.0
#define BUFFER_DURATION 10
#define BUFFER_SHIFT 1
#define DISCARD_DURATION 5
#define USE_ICA true
#define LOW_FREQ 50.0
#define HIGH_FREQ 150.0

// Forward declarations
bool loadFaceDetector(cv::CascadeClassifier& faceDetector, std::string sFilename);
std::vector<cv::Rect> detectFace(cv::CascadeClassifier& faceDetector, cv::Mat& frame);
cv::Rect extractForeheadROI(const std::vector<cv::Rect>& faceROI);
cv::Vec3f extractMeanRGB(const cv::Mat& frame, const cv::Rect& roi);
cv::Mat plotGraph(const std::vector<float>& vals, int ySize, int lowerBoundary = -1, int highBoundary = -1);

std::vector<float> computeICA(const std::deque<float>& R, const std::deque<float>& G, const std::deque<float>& B);
float computeTemporalAverage(const std::deque<float>& vSignal);
float computeTemporalStd(const std::deque<float>& vSignal);
std::vector<float> normalizeTemporalSignal(const std::deque<float>& vSignal, float avg, float stdDev);
std::vector<float> computeFourierTransform(const std::vector<float>& normalizedSignal);
int computeHeartRate(const std::vector<float>& powerSpectrum, float lowFreq, float highFreq, float fps);

//==GLOBAL VARIABLES==/
cv::CascadeClassifier faceDetector;
std::string sHaarCascadeFilename = "../data/haarcascade_frontalface_alt.xml";
cv::VideoCapture cap;
int deviceID = 0;
int apiID = cv::CAP_ANY;
bool isDataDiscarded = true;
int countDiscard = 0;
bool isFaceDetected = false;
std::vector<cv::Rect> faceRectangles;
cv::Rect foreheadROI;
cv::Mat frame_face;
cv::Mat frame_forehead;
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
    bool bIsFaceDetectorLoaded = loadFaceDetector(faceDetector, sHaarCascadeFilename);
    
    if (!bIsFaceDetectorLoaded)
        return 1;
        
    //==VIDEOCAPTURE==/
    cap.open(deviceID, apiID);
    
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
            cv::Mat frame;
            cap.read(frame);
            if (frame.empty()) 
            {
                std::cerr << "[ERROR] blank frame was grabbed -> skipping it!" << std::endl;
                break;
            }
            if (countDiscard == 0)
            {
                std::cout << "[INFO] Discarding data during " << DISCARD_DURATION << " seconds" << std::endl;
                std::cout << "--> Let the camera perform auto white balance... ";
            }
            countDiscard++;
            if (countDiscard == DISCARD_DURATION * FPS)
            {
                isDataDiscarded = false;
                std::cout << "DONE!" << std::endl;
            }
            
            cv::putText(frame, "Discarding images during auto white balancing", cv::Point(10, 20), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
            cv::imshow("Raw img", frame);
            
            if (cv::waitKey(1000.0 / FPS) >= 0)
                isInProcess = false;
        }
        
        // Detects the principal face in the image and determines its location
        while (!isFaceDetected)
        {
            cv::Mat frame;
            cap.read(frame);
            if (frame.empty()) 
            {
                std::cerr << "[ERROR] blank frame was grabbed -> skipping it!" << std::endl;
                break;
            }
            
            std::cout << "[INFO] Looking for faces..." << std::endl;
            
            faceRectangles = detectFace(faceDetector, frame);
            if (faceRectangles.size() > 0)
            {
                foreheadROI = extractForeheadROI(faceRectangles);
                
                std::cout << "--> " << faceRectangles.size() << " face(s) detected!" << std::endl;
                std::cout << "--> Forehead ROI extracted" << std::endl;
                
                isFaceDetected = true;
            }
            
            if (cv::waitKey(1000.0 / FPS) >= 0)
                isInProcess = false;
        }
        
        // Fills a buffer of data before estimating HR
        int previousProcessingPercentage = 0;
        
        while (!isBufferFull)
        {
            cv::Mat frame; 
            cap.read(frame);
            if (frame.empty()) 
            {
                std::cerr << "[ERROR] blank frame was grabbed -> skipping it!" << std::endl;
                break;
            }
            
            frame_face = frame(faceRectangles[0]);
            frame_forehead = frame(foreheadROI);
            
            cv::Vec3f rgbMean = extractMeanRGB(frame, foreheadROI);
            
            redSignal.push_back(rgbMean[0]);
            greenSignal.push_back(rgbMean[1]);
            blueSignal.push_back(rgbMean[2]);
            
            int currentProcessingPercentage = static_cast<int>((static_cast<float>(redSignal.size()) / (FPS * BUFFER_DURATION)) * 100);
            if (currentProcessingPercentage != previousProcessingPercentage)
                std::cout << "[INFO] Buffer size = " << currentProcessingPercentage << "%\r" << std::flush; 
            previousProcessingPercentage = currentProcessingPercentage;
            
            if (redSignal.size() == static_cast<size_t>(FPS * BUFFER_DURATION))
            {
                isBufferFull = true;
            }
            
            cv::imshow("Face img", frame_face);
            cv::imshow("Forehead img", frame_forehead);
            
            cv::rectangle(frame, faceRectangles[0], cv::Scalar(0, 255, 0), 2);
            cv::rectangle(frame, foreheadROI, cv::Scalar(255, 0, 0), 2);
            
            std::string str2Display = "Heart rate: " + std::to_string(heartRateBPM) + " BPM";
            if (heartRateBPM != -1)
                cv::putText(frame, str2Display, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2);
            else
                cv::putText(frame, "Estimating HR, Stay still!", cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
            
            cv::imshow("Raw img", frame);
            
            if (cv::waitKey(1000.0 / FPS) >= 0)
                isInProcess = false;
        }
        
        std::cout << std::endl;
        
        // Process signal using ICA or single channel
        if (USE_ICA)
        {
            std::cout << "[INFO] Processing signal with ICA..." << std::endl;
            processedSignal = computeICA(redSignal, greenSignal, blueSignal);
        }
        else
        {
            std::cout << "[INFO] Processing signal with green channel only..." << std::endl;
            processedSignal.assign(greenSignal.begin(), greenSignal.end());
        }
        
        std::deque<float> processedSignalDeque(processedSignal.begin(), processedSignal.end());
        float avg_signal = computeTemporalAverage(processedSignalDeque);
        float std_signal = computeTemporalStd(processedSignalDeque);
        
        std::vector<float> normalizedSignal = normalizeTemporalSignal(processedSignalDeque, avg_signal, std_signal);
        
        int yRange = static_cast<int>(FPS * BUFFER_DURATION);
        cv::Mat signalPlot = plotGraph(normalizedSignal, yRange, 0, -1);
        cv::imshow("Processed Signal", signalPlot);
        
        std::vector<float> powerSpectrum = computeFourierTransform(normalizedSignal);
        
        if (!powerSpectrum.empty())
        {
            int lowIdx = static_cast<int>(LOW_FREQ / 60.0f * powerSpectrum.size() / FPS);
            int highIdx = static_cast<int>(HIGH_FREQ / 60.0f * powerSpectrum.size() / FPS);
            cv::Mat fourierPlot = plotGraph(powerSpectrum, yRange, lowIdx, highIdx);
            cv::imshow("Power Spectrum", fourierPlot);
            
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
        
        int shiftSize = static_cast<int>(BUFFER_SHIFT * FPS);
        for (int l = 0; l < shiftSize; l++)
        {
            if (!redSignal.empty()) redSignal.pop_front();
            if (!greenSignal.empty()) greenSignal.pop_front();
            if (!blueSignal.empty()) blueSignal.pop_front();
        }
        isBufferFull = false;	
                
        if (cv::waitKey(1000.0 / FPS) >= 0)
            isInProcess = false;
    }
    
    cv::destroyAllWindows();
    cap.release();
    
    return 0;
}
