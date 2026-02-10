#include "SignalProcessing.h"
#include <opencv2/opencv.hpp>

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
    cv::Mat cov = (X * X.t()) / (float)X.cols;
    cv::Mat eigenValues, eigenVectors;
    cv::eigen(cov, eigenValues, eigenVectors);

    cv::Mat D = cv::Mat::zeros(3, 3, CV_32F);
    for (int i = 0; i < 3; i++)
        D.at<float>(i, i) = 1.f / sqrt(eigenValues.at<float>(i));

    return eigenVectors.t() * D * eigenVectors * X;
}

std::vector<float> computeICA(const std::deque<float>& R,
                              const std::deque<float>& G,
                              const std::deque<float>& B)
{
    int N = R.size();
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

    for (int it = 0; it < 200; it++)
    {
        cv::Mat WX = W * Xw;
        cv::Mat gwx, g_wx;

        cv::tanh(WX, gwx);
        g_wx = 1 - gwx.mul(gwx);

        cv::Mat Wnew = (gwx * Xw.t()) / (float)N;
        cv::Mat meanDeriv;
        cv::reduce(g_wx, meanDeriv, 1, cv::REDUCE_AVG);

        for (int i = 0; i < 3; i++)
            Wnew.row(i) -= meanDeriv.at<float>(i) * W.row(i);

        cv::Mat eVal, eVec;
        cv::eigen(Wnew * Wnew.t(), eVal, eVec);
        W = eVec.t() * cv::Mat::diag(1.0 / cv::sqrt(eVal)) * eVec * Wnew;
    }

    cv::Mat S = W * Xw;

    /* Select component with max variance */
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

/* ===== EXISTING FUNCTIONS UNCHANGED BELOW ===== */

float computeTemporalAverage(std::deque<float> vSignal)
{
    return std::accumulate(vSignal.begin(), vSignal.end(), 0.0) / vSignal.size();
}

float computeTemporalStd(std::deque<float> vSignal)
{
    double avg = computeTemporalAverage(vSignal);
    double sq_sum = std::inner_product(vSignal.begin(), vSignal.end(), vSignal.begin(), 0.0);
    return std::sqrt(sq_sum / vSignal.size() - avg * avg);
}

std::vector<float> normalizeTemporalSignal(std::deque<float> vSignal, float avg, float std)
{
    std::vector<float> out;
    for (auto& v : vSignal)
        out.push_back((v - avg) / std);
    return out;
}

