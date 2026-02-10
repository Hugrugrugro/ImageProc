#include "SignalProcessing.h"
#include "FFT.h"
#include <opencv2/opencv.hpp>
#include <algorithm>

/* ===================== ICA CORE ===================== */
static void center(cv::Mat& X)
{
    cv::Mat mean;
    cv::reduce(X, mean, 1, cv::REDUCE_AVG);
    for (int i = 0; i < X.rows; i++)
        X.row(i) -= mean.at<float>(i);
}

static cv::Mat whiten(const cv::Mat& X)
{
    cv::Mat cov = (X * X.t()) / static_cast<float>(X.cols);
    cv::Mat eigenValues, eigenVectors;
    cv::eigen(cov, eigenValues, eigenVectors);

    cv::Mat D = cv::Mat::zeros(3, 3, CV_32F);
    for (int i = 0; i < 3; i++)
        D.at<float>(i, i) = 1.0f / sqrt(eigenValues.at<float>(i) + 1e-6f); // Add small epsilon for stability

    return eigenVectors.t() * D * eigenVectors * X;
}

std::vector<float> computeICA(const std::deque<float>& R,
                              const std::deque<float>& G,
                              const std::deque<float>& B)
{
    int N = static_cast<int>(R.size());
    cv::Mat X(3, N, CV_32F);

    for (int i = 0; i < N; i++)
    {
        X.at<float>(0, i) = R[i];
        X.at<float>(1, i) = G[i];
        X.at<float>(2, i) = B[i];
    }

    center(X);
    cv::Mat Xw = whiten(X);

    cv::Mat W = cv::Mat::eye(3, 3, CV_32F);

    // FastICA iterations
    for (int it = 0; it < 200; it++)
    {
        cv::Mat WX = W * Xw;
        cv::Mat gwx, g_wx;

        cv::tanh(WX, gwx);
        g_wx = 1 - gwx.mul(gwx);

        cv::Mat Wnew = (gwx * Xw.t()) / static_cast<float>(N);
        cv::Mat meanDeriv;
        cv::reduce(g_wx, meanDeriv, 1, cv::REDUCE_AVG);

        for (int i = 0; i < 3; i++)
            Wnew.row(i) -= meanDeriv.at<float>(i) * W.row(i);

        cv::Mat eVal, eVec;
        cv::eigen(Wnew * Wnew.t(), eVal, eVec);
        
        cv::Mat sqrtEval = cv::Mat::zeros(3, 3, CV_32F);
        for (int i = 0; i < 3; i++)
            sqrtEval.at<float>(i, i) = 1.0f / sqrt(eVal.at<float>(i) + 1e-6f);
        
        W = eVec.t() * sqrtEval * eVec * Wnew;
    }

    cv::Mat S = W * Xw;

    /* Select component with max variance in cardiac frequency range */
    int best = 0;
    double maxVar = 0;

    for (int i = 0; i < 3; i++)
    {
        cv::Scalar mean, stddev;
        cv::meanStdDev(S.row(i), mean, stddev);
        if (stddev[0] > maxVar)
        {
            maxVar = stddev[0];
            best = i;
        }
    }

    std::vector<float> out(N);
    for (int i = 0; i < N; i++)
        out[i] = S.at<float>(best, i);

    return out;
}
/* ==================================================== */

/* ===== TEMPORAL STATISTICS ===== */
float computeTemporalAverage(const std::deque<float>& vSignal)
{
    if (vSignal.empty()) return 0.0f;
    return static_cast<float>(std::accumulate(vSignal.begin(), vSignal.end(), 0.0) / vSignal.size());
}

float computeTemporalStd(const std::deque<float>& vSignal)
{
    if (vSignal.empty()) return 0.0f;
    double avg = computeTemporalAverage(vSignal);
    double sq_sum = std::inner_product(vSignal.begin(), vSignal.end(), vSignal.begin(), 0.0);
    return static_cast<float>(std::sqrt(sq_sum / vSignal.size() - avg * avg));
}

std::vector<float> normalizeTemporalSignal(const std::deque<float>& vSignal, float avg, float std)
{
    std::vector<float> out;
    out.reserve(vSignal.size());
    
    if (std < 1e-6f) std = 1.0f; // Avoid division by zero
    
    for (const auto& v : vSignal)
        out.push_back((v - avg) / std);
    return out;
}

/* ===== FFT WRAPPER ===== */
std::vector<float> computeFourierTransform(const std::vector<float>& normalizedSignal)
{
    FFT fftProcessor;
    
    // Convert vector to deque for FFT processing
    std::deque<float> signalDeque(normalizedSignal.begin(), normalizedSignal.end());
    
    // Create a vector of deques (single signal)
    std::vector<std::deque<float>> bufferedSignals;
    bufferedSignals.push_back(signalDeque);
    
    // Set number of signals and FPS (will be overridden if needed)
    fftProcessor.setNbSignals(1);
    fftProcessor.setFps(15.0f); // Default FPS, should match main.cpp
    
    // Compute FFT
    fftProcessor.setBufferedSignalValues(bufferedSignals);
    
    // Get power spectrum
    std::vector<std::deque<float>> powerSpectrum = fftProcessor.getPowerSpectrum();
    
    if (powerSpectrum.size() >= 2)
    {
        // Return the power spectrum (skip first element which is frequency axis)
        std::deque<float>& spectrum = powerSpectrum[1];
        return std::vector<float>(spectrum.begin(), spectrum.end());
    }
    
    return std::vector<float>();
}

/* ===== HEART RATE COMPUTATION ===== */
int computeHeartRate(const std::vector<float>& powerSpectrum, float lowFreq, float highFreq, float fps)
{
    if (powerSpectrum.empty()) return -1;
    
    // Convert BPM to Hz
    float lowFreqHz = lowFreq / 60.0f;
    float highFreqHz = highFreq / 60.0f;
    
    // Find frequency resolution
    int N = static_cast<int>(powerSpectrum.size());
    float freqResolution = fps / (2.0f * N);
    
    // Convert frequency range to indices
    int lowIdx = static_cast<int>(lowFreqHz / freqResolution);
    int highIdx = static_cast<int>(highFreqHz / freqResolution);
    
    // Clamp indices
    lowIdx = std::max(0, std::min(lowIdx, N - 1));
    highIdx = std::max(0, std::min(highIdx, N - 1));
    
    if (lowIdx >= highIdx) return -1;
    
    // Find peak in the range
    int maxIdx = lowIdx;
    float maxValue = powerSpectrum[lowIdx];
    
    for (int i = lowIdx; i <= highIdx; i++)
    {
        if (powerSpectrum[i] > maxValue)
        {
            maxValue = powerSpectrum[i];
            maxIdx = i;
        }
    }
    
    // Convert index to frequency in Hz
    float peakFreqHz = maxIdx * freqResolution;
    
    // Convert to BPM
    int heartRateBPM = static_cast<int>(peakFreqHz * 60.0f);
    
    return heartRateBPM;
}
