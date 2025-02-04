#include <cmath>
#include <cstring>
#include <cassert>

#include "AtomicQueue.hpp" 

#ifndef _LIB_TONE_GENERATOR
#define _LIB_TONE_GENERATOR

/**
 * @enum WAVE_TYPE 
 * @brief Типы сигналов, которые могут быть сгенерированы 
 * @var WAVE_TYPE::SINE 
 * Синусоидальный сигнал 
 * TODO: @var WAVE_TYPE::SQUARE 
 * Прямоугольный сигнал
 * TODO: @var WAVE_TYPE::TRIANGLE 
 * Треугольный сигнал 
 * TODO: @var WAVE_TYPE::SAWTOOTH
 * Пилообразный сигнал
 */
enum WAVE_TYPE { 
    SINE, 
    SQUARE, 
    TRIANGLE, 
    SAWTOOTH, 
};

/**
 * TODO: docs
 */
template <typename T> 
class ToneGenerator { 
private:  
    /**
     * TODO: docs 
     */
    const WAVE_TYPE     wave_type; 
    const double        tone_freq;
    const double        sample_rate;
    const size_t        sample_size;   
    
    /**
     * TODO: docs 
     */
    T*                  buffer;
public: 

    /**
     * @param freq          float - Частота сигнала в [Hz] 
     * @param type          WAVE_TYPE - тип сигнала (см enum WAVE_TYPE)
     * @param sample_rate   float - частота дискретизации  
     * @param sample_size   size_t - размер буффера (длительность сигнала)
     */
    ToneGenerator(const long double freq, const WAVE_TYPE type, const long double sample_rate, const size_t sample_size);
    ~ToneGenerator();

    /** 
     * TODO: docs
     */
    void Generate(T* &buffptr); 

    /**
     * TODO: docs
     */
    T normalize(float value);

    /** 
     * TODO: docs 
     */
    double RealFrequency();
protected: 
    /**
     * TODO: docs
     */
    void _generic_wave_sine(); 

    /**
     * TODO: docs
     */
    void _generic_wave_square(); 

    /**
     * TODO: docs
     */
    void _generic_wave_triangle(); 

    /**
     * TODO: docs
     */
    void _generic_wave_sawtooth(); 
};

// Не забудьте явную инстанциацию шаблонного класса для нужных типов
template class ToneGenerator<short>;
template class ToneGenerator<int>;
// Добавьте другие типы по мере необходимости

#endif 
/**
 * 
 */