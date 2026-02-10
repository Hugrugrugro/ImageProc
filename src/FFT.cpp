// FFT.cpp - Standalone version
#include <vector>
#include <deque>
#include <iostream>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <stdexcept>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class FFT
{
public:
    FFT();
    ~FFT();
    void setNbSignals(int nbSignal);
    void setFps(float fFps);
    void setBufferedSignalValues(std::vector<std::deque<float>> vBufferedSignalValues);
    std::vector<std::deque<float>> getPowerSpectrum();

private:
    void pad(std::vector<float>& vInputSignal, std::vector<float>& vPaddedSignal);
    void hannWindow(std::vector<float>& inSignal, std::vector<float>& outSignal);
    void compute(std::vector<float>& vInputSignal, std::vector<float>& vFFTRealPart, std::vector<float>& vFFTImagPart);
    void transformRadix2(std::vector<float>& real, std::vector<float>& imag);
    std::size_t reverseBits(std::size_t x, unsigned int n);
    void powerSpectrum(std::vector<float>& vFFTRealPart, std::vector<float>& vFFTImagPart, std::vector<float>& vFFTPowerSpectrum);
    void phaseSpectrum(std::vector<float>& vFFTRealPart, std::vector<float>& vFFTImagPart, std::vector<float>& vFFTPhaseSpectrum);
    unsigned long findUpperPowerOfTwo(unsigned long v);
    
    int m_i32NbSignals;
    float m_fFps;
    bool m_bTrigoTablesComputed;
    std::size_t m_i32InSignalLength;
    std::vector<float> m_vf64cosTable;
    std::vector<float> m_vf64sinTable;
    std::vector<std::deque<float>> m_vPowerSpectrumValues;
};

FFT::FFT()
{
    m_bTrigoTablesComputed = false;
    m_i32InSignalLength = 0;
    m_vPowerSpectrumValues.clear();
}

FFT::~FFT()
{

}

void FFT::setNbSignals(int nbSignal)
{
    m_i32NbSignals = nbSignal;
}

void FFT::setFps(float fFps)
{
    m_fFps = fFps;
}

void FFT::setBufferedSignalValues(std::vector<std::deque<float>> vBufferedSignalValues)
{
    std::vector<std::deque<float>> l_vPowerSpectrumValues;

    std::deque<float> l_dSpectrumFrequency;
    unsigned long l_i64NbFrequencies = findUpperPowerOfTwo(vBufferedSignalValues[0].size());
    
    for (unsigned long l_freq = 0; l_freq < l_i64NbFrequencies; l_freq++)
    {
        l_dSpectrumFrequency.push_back(static_cast<float>(l_freq) * m_fFps / l_i64NbFrequencies);
    }
    
    l_vPowerSpectrumValues.push_back(l_dSpectrumFrequency);

    for (int l_signal = 0; l_signal < m_i32NbSignals; l_signal++)
    {
        std::vector<float> l_vCurrentBufferedSignal(vBufferedSignalValues[l_signal].begin(), 
                                                     vBufferedSignalValues[l_signal].end());
        
        std::vector<float> l_vCurrentPaddedSignal;
        pad(l_vCurrentBufferedSignal, l_vCurrentPaddedSignal);
        
        std::vector<float> l_vCurrentWindowedSignal;
        hannWindow(l_vCurrentPaddedSignal, l_vCurrentWindowedSignal);
        
        std::vector<float> l_vFFTRealPart, l_vFFTImagPart;
        compute(l_vCurrentWindowedSignal, l_vFFTRealPart, l_vFFTImagPart);
        
        std::vector<float> l_vFFTPowerSpectrum;
        powerSpectrum(l_vFFTRealPart, l_vFFTImagPart, l_vFFTPowerSpectrum);
        
        std::vector<float> l_vFFTPowerSpectrumHalf(l_vFFTPowerSpectrum.begin(), 
                                                    l_vFFTPowerSpectrum.begin() + l_vFFTPowerSpectrum.size() / 2);
        
        std::deque<float> l_dPowerSpectrum(l_vFFTPowerSpectrumHalf.begin(), l_vFFTPowerSpectrumHalf.end());
        l_vPowerSpectrumValues.push_back(l_dPowerSpectrum);
    }

    m_vPowerSpectrumValues = l_vPowerSpectrumValues;
}

std::vector<std::deque<float>> FFT::getPowerSpectrum()
{
    return m_vPowerSpectrumValues;
}

void FFT::pad(std::vector<float>& vInputSignal, std::vector<float>& vPaddedSignal)
{
    if (vInputSignal.empty())
    {
        vPaddedSignal.clear();
        return;
    }
    
    if ((vInputSignal.size() & (vInputSignal.size() - 1)) == 0)
    {
        vPaddedSignal = vInputSignal;
    }
    else
    {
        unsigned long paddedSize = findUpperPowerOfTwo(vInputSignal.size());
        vPaddedSignal.resize(paddedSize, 0.0f);
        std::copy(vInputSignal.begin(), vInputSignal.end(), vPaddedSignal.begin());
    }
}

unsigned long FFT::findUpperPowerOfTwo(unsigned long v)
{
    if (v == 0) return 1;
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

void FFT::compute(std::vector<float>& vInputSignal, std::vector<float>& vFFTRealPart, std::vector<float>& vFFTImagPart)
{
    std::vector<float> l_vf64SignalReal(vInputSignal);
    std::vector<float> l_vf64SignalImag(l_vf64SignalReal.size(), 0.0f);

    std::size_t n = l_vf64SignalReal.size();
    if (n == 0)
    {
        vFFTRealPart.clear();
        vFFTImagPart.clear();
        return;
    }
    
    if ((n & (n - 1)) == 0)
        transformRadix2(l_vf64SignalReal, l_vf64SignalImag);
    else
        std::cerr << "[ERROR] (FFT::compute) : input signal is not padded!" << std::endl;

    vFFTRealPart = l_vf64SignalReal;
    vFFTImagPart = l_vf64SignalImag;
}

void FFT::transformRadix2(std::vector<float>& real, std::vector<float>& imag)
{
    if (real.size() != imag.size())
        throw std::runtime_error("Mismatched lengths");

    std::size_t n = real.size();
    
    unsigned int levels = 0;
    {
        std::size_t temp = n;
        while (temp > 1) {
            levels++;
            temp >>= 1;
        }
        if (1u << levels != n)
            throw std::runtime_error("Length is not a power of 2");
    }

    if (real.size() != m_i32InSignalLength)
    {
        m_bTrigoTablesComputed = false;
        m_i32InSignalLength = real.size();
    }

    if (!m_bTrigoTablesComputed)
    {
        m_vf64cosTable.resize(n / 2);
        m_vf64sinTable.resize(n / 2);

        for (std::size_t i = 0; i < n / 2; i++)
        {
            m_vf64cosTable[i] = static_cast<float>(cos(2.0 * M_PI * i / n));
            m_vf64sinTable[i] = static_cast<float>(sin(2.0 * M_PI * i / n));
        }

        m_bTrigoTablesComputed = true;
    }

    for (std::size_t i = 0; i < n; i++)
    {
        std::size_t j = reverseBits(i, levels);
        if (j > i)
        {
            std::swap(real[i], real[j]);
            std::swap(imag[i], imag[j]);
        }
    }

    for (std::size_t size = 2; size <= n; size *= 2)
    {
        std::size_t halfsize = size / 2;
        std::size_t tablestep = n / size;
        for (std::size_t i = 0; i < n; i += size)
        {
            for (std::size_t j = i, k = 0; j < i + halfsize; j++, k += tablestep)
            {
                float tpre = real[j + halfsize] * m_vf64cosTable[k] + imag[j + halfsize] * m_vf64sinTable[k];
                float tpim = -real[j + halfsize] * m_vf64sinTable[k] + imag[j + halfsize] * m_vf64cosTable[k];
                real[j + halfsize] = real[j] - tpre;
                imag[j + halfsize] = imag[j] - tpim;
                real[j] += tpre;
                imag[j] += tpim;
            }
        }
        if (size == n)
            break;
    }
}

std::size_t FFT::reverseBits(std::size_t x, unsigned int n)
{
    std::size_t result = 0;
    for (unsigned int i = 0; i < n; i++, x >>= 1)
        result = (result << 1) | (x & 1);
    return result;
}

void FFT::powerSpectrum(std::vector<float>& vFFTRealPart, std::vector<float>& vFFTImagPart, std::vector<float>& vFFTPowerSpectrum)
{
    std::size_t n = vFFTRealPart.size();
    if (n == 0)
    {
        vFFTPowerSpectrum.clear();
        return;
    }

    vFFTPowerSpectrum.resize(n);
    for (std::size_t i = 0; i < n; i++)
    {
        vFFTPowerSpectrum[i] = sqrt(vFFTRealPart[i] * vFFTRealPart[i] + vFFTImagPart[i] * vFFTImagPart[i]);
    }
}

void FFT::phaseSpectrum(std::vector<float>& vFFTRealPart, std::vector<float>& vFFTImagPart, std::vector<float>& vFFTPhaseSpectrum)
{
    std::size_t n = vFFTRealPart.size();
    if (n == 0)
    {
        vFFTPhaseSpectrum.clear();
        return;
    }

    vFFTPhaseSpectrum.resize(n);
    for (std::size_t i = 0; i < n; i++)
    {
        vFFTPhaseSpectrum[i] = atan2(vFFTImagPart[i], vFFTRealPart[i]);
    }
}

void FFT::hannWindow(std::vector<float>& inSignal, std::vector<float>& outSignal)
{
    std::size_t n = inSignal.size();
    if (n == 0)
    {
        outSignal.clear();
        return;
    }

    outSignal.clear();
    outSignal.reserve(n);
    
    for (std::size_t i = 0; i < n; i++)
    {
        float multiplier = 0.5f * (1.0f - cos(2.0f * M_PI * i / (n - 1)));
        outSignal.push_back(multiplier * inSignal[i]);
    }
}
