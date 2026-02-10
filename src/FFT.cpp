#include "FFT.h"

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
    //m_vBufferedSignals.resize(m_i32NbSignals);
    //m_vBufferedFilteredSignals.resize(m_i32NbSignals);
}

void FFT::setFps(float fFps)
{
    m_fFps = fFps;
}

void FFT::setBufferedSignalValues(std::vector<std::deque<float>> vBufferedSignalValues)
{
    std::vector<std::deque<float>> l_vPowerSpectrumValues;

    // creates the x-axis deque
    std::deque<float> l_dSpectrumFrequency;
    unsigned long l_i64NbFrequencies = findUpperPowerOfTwo(vBufferedSignalValues[0].size());
	//std::cout << "l_i64NbFrequencies= " << l_i64NbFrequencies << std::endl;
    for (int l_freq = 0; l_freq < l_i64NbFrequencies; l_freq++)
    {
        l_dSpectrumFrequency.push_back((float)l_freq * m_fFps / l_i64NbFrequencies);
    }
    //std::cout << "l_i64NbFrequencies= " << l_i64NbFrequencies << std::endl;
    l_vPowerSpectrumValues.push_back(l_dSpectrumFrequency);
//std::cout << "l_i64NbFrequencies= " << l_i64NbFrequencies << std::endl;

    for (auto l_signal = 0; l_signal < m_i32NbSignals; l_signal++)
    {
        std::vector<float> l_vCurrentBufferedSignal = {vBufferedSignalValues[l_signal].begin(), vBufferedSignalValues[l_signal].end()};
        // padding
        std::vector<float> l_vCurrentPaddedSignal;
        pad(l_vCurrentBufferedSignal, l_vCurrentPaddedSignal);
        // windowing
        std::vector<float> l_vCurrentWindowedSignal;
        hannWindow(l_vCurrentPaddedSignal, l_vCurrentWindowedSignal);
        // FFT
        std::vector<float> l_vFFTRealPart, l_vFFTImagPart;
        compute(l_vCurrentWindowedSignal, l_vFFTRealPart, l_vFFTImagPart);
        // PowerSpectrum
        std::vector<float> l_vFFTPowerSpectrum;
        powerSpectrum(l_vFFTRealPart, l_vFFTImagPart, l_vFFTPowerSpectrum);
        // Phase Spectrum
        std::vector<float> l_vFFTPhaseSpectrum;
        phaseSpectrum(l_vFFTRealPart, l_vFFTImagPart, l_vFFTPhaseSpectrum);

        std::deque<float> l_dPowerSpectrum = {l_vFFTPowerSpectrum.begin(), l_vFFTPowerSpectrum.end()};
        l_vPowerSpectrumValues.push_back(l_dPowerSpectrum);
    }
    //std::cout << "l_i64NbFrequencies= " << l_i64NbFrequencies << std::endl;

    m_vPowerSpectrumValues = l_vPowerSpectrumValues;
    //std::cout << "l_i64NbFrequencies= " << l_i64NbFrequencies << std::endl;
}

std::vector<std::deque<float>>  FFT::getPowerSpectrum()
{
	return m_vPowerSpectrumValues;
}

void FFT::pad(std::vector<float> &vInputSignal, std::vector<float> &vPaddedSignal)
{
    if (vInputSignal.size() == 0)
        return;
    else if ((vInputSignal.size() & (vInputSignal.size() - 1)) == 0)  // Is power of 2
    {
        vPaddedSignal = vInputSignal;
    }
    else
    {
        vPaddedSignal.resize(findUpperPowerOfTwo(vInputSignal.size()));
        std::fill(vPaddedSignal.begin(), vPaddedSignal.end(), 0.0f);
        memcpy(&vPaddedSignal.at(0), &vInputSignal.at(0), vInputSignal.size());
    }
}

unsigned long FFT::findUpperPowerOfTwo(unsigned long v)
{
    // To test
    //unsigned long next = pow(2, ceil(log(v)/log(2)));
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

void FFT::compute(std::vector<float> &vInputSignal, std::vector<float> &vFFTRealPart, std::vector<float> &vFFTImagPart)
{
    // sets the real and imag parts of the input signal
    // our input signal is always real
    std::vector<float> l_vf64SignalReal(vInputSignal);
    std::vector<float> l_vf64SignalImag(l_vf64SignalReal.size(), 0.0);

    std::size_t n = l_vf64SignalReal.size();
    if (n == 0)
        return;
    else if ((n & (n - 1)) == 0)  // Is power of 2
        transformRadix2(l_vf64SignalReal, l_vf64SignalImag);
    else  // More complicated algorithm for arbitrary sizes
        std::cerr << "[ERROR] (FFT::compute) : input signal is not padded!" << std::endl;

    vFFTRealPart = l_vf64SignalReal;
    vFFTImagPart = l_vf64SignalImag;
}

void FFT::transformRadix2(std::vector<float> &real, std::vector<float> &imag)
{
    // Compute levels = floor(log2(n))
    if (real.size() != imag.size())
        throw "Mismatched lengths";

    std::size_t n = real.size();
    unsigned int levels;
    {
        std::size_t temp = n;
        levels = 0;
        while (temp > 1) {
            levels++;
            temp >>= 1;
        }
        if (1u << levels != n)
            throw "Length is not a power of 2";
    }

    // buffer size has changed therefore trigo tables shoud be recomputed
    if (real.size()!=m_i32InSignalLength)
        m_bTrigoTablesComputed = false;

    if (!m_bTrigoTablesComputed)
    {
        // Trignometric tables
        m_vf64cosTable.resize(n/2);
        m_vf64sinTable.resize(n/2);

        for (std::size_t i = 0; i < n / 2; i++)
        {
            m_vf64cosTable[i] = cos(2 * M_PI * i / n);
            m_vf64sinTable[i] = sin(2 * M_PI * i / n);
        }

        m_bTrigoTablesComputed = true;
    }



    // Bit-reversed addressing permutation
    for (std::size_t i = 0; i < n; i++)
    {
        std::size_t j = reverseBits(i, levels);
        if (j > i)
        {
            double temp = real[i];
            real[i] = real[j];
            real[j] = temp;
            temp = imag[i];
            imag[i] = imag[j];
            imag[j] = temp;
        }
    }

    // Cooley-Tukey decimation-in-time radix-2 FFT
    for (std::size_t size = 2; size <= n; size *= 2)
    {
        std::size_t halfsize = size / 2;
        std::size_t tablestep = n / size;
        for (std::size_t i = 0; i < n; i += size)
        {
            for (std::size_t j = i, k = 0; j < i + halfsize; j++, k += tablestep)
            {
                double tpre =  real[j+halfsize] * m_vf64cosTable[k] + imag[j+halfsize] * m_vf64sinTable[k];
                double tpim = -real[j+halfsize] * m_vf64sinTable[k] + imag[j+halfsize] * m_vf64cosTable[k];
                real[j + halfsize] = real[j] - tpre;
                imag[j + halfsize] = imag[j] - tpim;
                real[j] += tpre;
                imag[j] += tpim;
            }
        }
        if (size == n)  // Prevent overflow in 'size *= 2'
            break;
    }
}

std::size_t FFT::reverseBits(std::size_t x, unsigned int n)
{
    std::size_t result = 0;
    unsigned int i;
    for (i = 0; i < n; i++, x >>= 1)
        result = (result << 1) | (x & 1);
    return result;
}

void FFT::powerSpectrum(std::vector<float> &vFFTRealPart, std::vector<float> &vFFTImagPart, std::vector<float> &vFFTPowerSpectrum)
{
    std::size_t n = vFFTRealPart.size();
    if (n == 0)
        return;

    vFFTPowerSpectrum.resize(n);
    for (std::size_t i = 0; i < n; i++)
    {
        vFFTPowerSpectrum[i] = sqrt(pow(vFFTRealPart[i],2) + pow(vFFTImagPart[i],2));
    }
}

void FFT::phaseSpectrum(std::vector<float> &vFFTRealPart, std::vector<float> &vFFTImagPart, std::vector<float> &vFFTPowerSpectrum)
{
    std::size_t n = vFFTRealPart.size();
    if (n == 0)
        return;

    vFFTPowerSpectrum.resize(n);
    for (std::size_t i = 0; i < n; i++)
    {
        vFFTPowerSpectrum[i] = atan2(vFFTImagPart[i], vFFTRealPart[i]);
    }
}

void FFT::hannWindow(std::vector<float>& inSignal, std::vector<float>& outSignal)
{
    std::size_t n = inSignal.size();
    if (n == 0)
        return;

    for (std::size_t i=0; i < n; i++)
    {
        double multiplier = 0.5 * (1 - cos(2*M_PI*i/(n-1)));
        outSignal.push_back(multiplier * inSignal[i]);
    }
}


