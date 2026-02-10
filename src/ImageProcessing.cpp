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

cv::Rect extractForeheadROI(std::vector<cv::Rect>& faceROI)
{
    cv::Rect foreheadROI;
    if (!faceROI.empty())
    {
        foreheadROI = faceROI[0];
        foreheadROI.height *= 0.3;
    }
    return foreheadROI;
}

/* ===================== NEW ===================== */
cv::Vec3f extractMeanRGB(cv::Mat& frame, cv::Rect& roi)
{
    cv::Mat roiImg = frame(roi);
    cv::Scalar meanVal = cv::mean(roiImg);
    return cv::Vec3f(meanVal[2], meanVal[1], meanVal[0]); // R,G,B
}
/* =============================================== */

cv::Mat plotGraph(std::vector<float>& vals, int ySize, int lowerBoundary, int highBoundary)
{
    auto it = minmax_element(vals.begin(), vals.end());
    float scale = 1.f / ceil(*it.second - *it.first);
    float bias = *it.first;

    int rows = ySize + 1;
    cv::Mat image = 255 * cv::Mat::ones(rows, vals.size(), CV_8UC3);

    for (int i = 0; i < (int)vals.size() - 1; i++)
    {
        cv::line(image,
            cv::Point(i, rows - 1 - (vals[i] - bias) * scale * ySize),
            cv::Point(i + 1, rows - 1 - (vals[i + 1] - bias) * scale * ySize),
            cv::Scalar(255, 0, 0), 1);
    }

    cv::resize(image, image, cv::Size(450, 450));
    return image;
}

