#include "ToneGenerator.hpp"

template <typename T> 
ToneGenerator<T>::ToneGenerator(const long double freq, WAVE_TYPE type, const long double sample_rate, size_t sample_size) 
    : tone_freq(freq), wave_type(type), sample_rate(sample_rate), sample_size(sample_size) { 
        buffer = new T[sample_size]; 

        /**
         * Хе-хе, Калачикова тебе в рот
         * А ну-ка Котельникова–Шеннона любим!!!
         */
        assert(tone_freq < sample_rate / 2); 
    } 

template <typename T> ToneGenerator<T>::~ToneGenerator() { 
    delete[] buffer; 
}

template <typename T>
void ToneGenerator<T>::Generate(T* &buffptr) {
    std::memset(buffer, static_cast<T>(0), sizeof(T) * sample_size);

    switch (wave_type) {
        case SINE:
            _generic_wave_sine();
            break;
        case SQUARE:
            _generic_wave_square();
            break;
        case TRIANGLE:
            _generic_wave_triangle();
            break;
        case SAWTOOTH:
            _generic_wave_sawtooth();
            break;
        default:
            break;
    }

    buffptr = buffer;
}

template <typename T>
void ToneGenerator<T>::_generic_wave_sine() {
    for (size_t i = 0; i < sample_size; ++i) {
        long double t = static_cast<long double>(i) / sample_rate;
        long double sine_wave = std::sin(2.0L * M_PI * tone_freq * t); 
        buffer[i] = normalize(sine_wave);
    }
}

/**
 * TODO: create this method dmitry!!!
 */
template <typename T>
void ToneGenerator<T>::_generic_wave_square() {

}

/**
 * TODO: create this method dmitry!!!
 */
template <typename T>
void ToneGenerator<T>::_generic_wave_triangle() {

}

/**
 * TODO: create this method dmitry!!!
 */
template <typename T>
void ToneGenerator<T>::_generic_wave_sawtooth() {
    
}

template <typename T> 
T ToneGenerator<T>::normalize(float val) { 
    if (std::is_same<T, short>::value) { 
        return static_cast<T>(val * 32767);         // Диапазон short [-32768, 32767] 
    } else if (std::is_same<T, int>::value) { 
        return static_cast<T>(val * 2147483647);    // Диапазон int [-2147483648, 2147483647] 
    } else { 
        return static_cast<T>(val);                 // Для float и других типов 
    }
}

template <typename T> 
double ToneGenerator<T>::RealFrequency() { 
    return 0.0;
}