#pragma once
#include <atomic>

template <typename T>   //Это не объявление класса, а шаблон
class AtomicQueue
{
public:
    AtomicQueue(unsigned int length);
    ~AtomicQueue();
    bool Push(const T val);    // Запись в конец очереди
    bool Pull(T &val);         // чтение из начала очереди
    unsigned int Size();

    unsigned int getMaxLength();// Получить LENGTH
    
    /**
     * TODO: docs
     */
    void push_zeros(size_t);
    void reset();               // сброс очереди в начальное состояние
private:
    unsigned int m_length = 0;  // Максимальный размер очереди

    std::atomic<unsigned int> read_ind;     //объявляем атомарные переменные
    std::atomic<unsigned int> write_ind;
    std::atomic<unsigned int> size;     
    //T buffer[LENGTH];                     //массив очереди пользовательского типа
    T* buffer = nullptr;                    //массив очереди пользовательского типа
};

typedef unsigned char byte;
template class AtomicQueue<char>;       //явно создаем экземпляр шаблона класса
template class AtomicQueue<short>;      //явно создаем экземпляр шаблона класса
template class AtomicQueue<byte>;       //явно создаем экземпляр шаблона класса
template class AtomicQueue<int>;        //явно создаем экземпляр шаблона класса
//template class AtomicQueue<unsigned char>; duplicate explicit instantiation of 'class AtomicQueue<unsigned char>' [-fpermissive]