#include <iostream>
#include <string>

class Pandas {
public:
    int _age;
    int _weight;
    int _height;
    std::string _name;


};

class DynamicArray {
private:
    Pandas *_data = nullptr;
    size_t _size = 0;
    size_t _cap = 5;
public:
    DynamicArray() {
        _data = (Pandas *) malloc(sizeof(Pandas) * _cap);
        if (_data == nullptr) {
            throw std::runtime_error("Malloc error!");
        }
    }

    ~DynamicArray() {
        free(_data);
    }

    //  конструктор копирования
    DynamicArray(const DynamicArray &other) : _cap(other._cap), _size(other._size) {

        _data = (Pandas *) malloc(sizeof(Pandas) * _cap);
        if (_data == nullptr) {
            throw std::runtime_error("Malloc failed!");
        }
        for (size_t i = 0; i < _size; i++) {
            _data[i] = other._data[i];
        }
    }

    // конструктор перемещения
    DynamicArray(DynamicArray &&other) : _data(other._data), _size(other._size), _cap(other._cap) {
        other._data = nullptr;
        other._cap = 0;
        other._size = 0;
    }

    // Вернет последний элемент
    Pandas Popback() {
        if (_size == 0) {
            throw std::runtime_error("No data!");
        }

        Pandas result{_data[_size]._age, _data[_size]._weight, _data[_size]._height, _data[_size]._name};
        _size--;

        return result;
    }

    void Pushback(const Pandas &current) {
        if (_size == _cap) {
            Pandas *tmp = (Pandas *) realloc(_data, sizeof(Pandas) * _cap * 2);
            if (!tmp) {
                free(_data);
                throw std::runtime_error("Realloc error!");
            } else {
                _data = tmp;
                _cap *= 2;
            }
        }

        _data[_size++] = current;

    }

    DynamicArray &operator=(DynamicArray &&other) {
        _size = (other._size);
        _cap = (other._cap);
        Pandas *temp = _data;
        _data = other._data;
        other._data = temp;
        return *this;
    }

    DynamicArray &operator=(const DynamicArray &other) {
        if (this == &other) {
            throw std::runtime_error("Error!");
        }
        free(_data);
        _data = (Pandas *) malloc(sizeof(Pandas) * other._size);
        if (!_data) {
            throw std::runtime_error("malloc Error!");
        } else {
            for (size_t i = 0; i < other._size; ++i) {
                _data[i] = other._data[i];
            }
        }
        _size = other._size;
        _cap = other._cap;

        return *this;
    }

    DynamicArray &operator+=(const DynamicArray &other) {
        size_t size = other._size + _size;
        _data = (Pandas *) realloc(_data, size);
        _cap = size;
        if (!_data) {
            throw std::runtime_error("Malloc Error");
        }

        for (int i = _size, j = 0; i < size; ++i, ++j) {
            _data[i] = other._data[j];
        }

        _size = size;

        return *this;
    }
    DynamicArray operator+(const DynamicArray &other) {
        DynamicArray tmp = *this;
        tmp += other;
        return tmp;

    }
    Pandas &operator[](size_t index) {
        if (index > this->_size || index < 0) {
            throw std::runtime_error("IndexError");
        }

        return this->_data[index];
    }


};

int main() {
    try {
        const Pandas Misha{19, 65, 177, "Misha"};
        const Pandas neMisha{19, 65, 177, "Misha"};
        DynamicArray a;
        DynamicArray b;
        b.Pushback(neMisha);
        a.Pushback(Misha);
        return 0;
    }
    catch (std::exception& i){
        std::cout<< i.what();
    }

}
