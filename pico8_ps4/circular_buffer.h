#pragma once

#include <vector>

template <class T>
class CircularBuffer
{
private:
	std::vector<T> *buffer;
	int length;
	int start;

public:
	CircularBuffer(int capacity) {
		this->length = 0;
		this->start = 0;
		this->buffer = new std::vector<T>(capacity);
	};
	~CircularBuffer() {
		delete this->buffer;
	};
	void push(T value) {
		if (this->length < this->buffer->size()) {
			(*this->buffer)[this->length] = value;
			this->length++;
		}
		else {
			(*this->buffer)[this->start] = value;
			this->start = (this->start + 1) % this->length;
		}
	};
	T get(int index) {
		T value = (*this->buffer)[(this->start + index) % this->length];
		return value;
	};
	int size() {
		return this->length;
	};
};


