#include "ImageProcessing.h"

bool loadFaceDetector(cv::CascadeClassifier& faceDetector, std::string sFilename)
{
    if (!faceDetector.load(sFilename))
    {
        std::cerr << "[ERROR] Unable to load face cascade" << std::endl;
        return false;
    }
    return true;
}

std::vector<cv::Rect> detectFace(cv::CascadeClassifier& faceDetector, cv::Mat& frame)
{
    std::vector<cv::Rect> faceRectangles;
    cv::Mat frame_gray;
    cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
    faceDetector.detectMultiScale(frame_gray, faceRectangles, 1.1, 3, 0, cv::Size(20, 20));
    return faceRectangles;
}

cv::Rect extractForeheadROI(const std::vector<cv::Rect>& faceROI)
{
    cv::Rect foreheadROI;
    if (!faceROI.empty())
    {
        foreheadROI = faceROI[0];
        // Position forehead in upper third of face
        foreheadROI.y += static_cast<int>(foreheadROI.height * 0.1);  // Start slightly below top
        foreheadROI.height = static_cast<int>(foreheadROI.height * 0.25);  // Use upper 25% of face
        foreheadROI.x += static_cast<int>(foreheadROI.width * 0.25);  // Center horizontally
        foreheadROI.width = static_cast<int>(foreheadROI.width * 0.5);  // Use middle 50% width
    }
    return foreheadROI;
}

cv::Vec3f extractMeanRGB(const cv::Mat& frame, const cv::Rect& roi)
{
    cv::Mat roiImg = frame(roi);
    cv::Scalar meanVal = cv::mean(roiImg);
    return cv::Vec3f(static_cast<float>(meanVal[2]), 
                     static_cast<float>(meanVal[1]), 
                     static_cast<float>(meanVal[0])); // R,G,B
}

cv::Mat plotGraph(const std::vector<float>& vals, int ySize, int lowerBoundary, int highBoundary)
{
    if (vals.empty())
        return cv::Mat();
    
    // Determine range for plotting
    int startIdx = (lowerBoundary >= 0) ? lowerBoundary : 0;
    int endIdx = (highBoundary >= 0 && highBoundary < static_cast<int>(vals.size())) ? highBoundary : vals.size() - 1;
    
    if (startIdx >= endIdx || startIdx >= static_cast<int>(vals.size()))
    {
        startIdx = 0;
        endIdx = vals.size() - 1;
    }
    
    // Find min and max in the range
    float minVal = vals[startIdx];
    float maxVal = vals[startIdx];
    for (int i = startIdx; i <= endIdx; i++)
    {
        if (vals[i] < minVal) minVal = vals[i];
        if (vals[i] > maxVal) maxVal = vals[i];
    }
    
    float range = maxVal - minVal;
    if (range < 1e-6f) range = 1.0f; // Avoid division by zero
    
    float scale = static_cast<float>(ySize) / range;
    float bias = minVal;

    int rows = ySize + 1;
    int cols = endIdx - startIdx + 1;
    cv::Mat image = cv::Mat(rows, cols, CV_8UC3, cv::Scalar(255, 255, 255));

    for (int i = 0; i < cols - 1; i++)
    {
        int y1 = rows - 1 - static_cast<int>((vals[startIdx + i] - bias) * scale);
        int y2 = rows - 1 - static_cast<int>((vals[startIdx + i + 1] - bias) * scale);
        
        // Clamp values
        y1 = std::max(0, std::min(rows - 1, y1));
        y2 = std::max(0, std::min(rows - 1, y2));
        
        cv::line(image,
            cv::Point(i, y1),
            cv::Point(i + 1, y2),
            cv::Scalar(255, 0, 0), 1);
    }

    cv::resize(image, image, cv::Size(450, 450));
    return image;
}
