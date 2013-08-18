//
//  Deque.h
//  OpenSpades
//
//  Created by yvt on 7/14/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "../Core/Debug.h"

namespace spades {
	/** Deque implementation. NPOT is not fully supported. */
	template<typename T>
	class Deque {
		T *ptr;
		size_t length;
		size_t startPos;
		size_t capacity;
		
	public:
		Deque(size_t cap) {
			ptr = (T *)malloc(sizeof(T) * cap);
			startPos = 0;
			length = 0;
			capacity = cap;
		}
		
		~Deque() {
			free(ptr);
		}
		
		void Reserve(size_t newCap) {
			if(newCap <= capacity)
				return;
			T *newPtr = (T *)malloc(sizeof(T) * newCap);
			size_t pos = startPos;
			for(size_t i = 0; i < length; i++){
				newPtr[i] = ptr[pos++];
				if(pos == capacity)
					pos = 0;
			}
			free(ptr); ptr = newPtr;
			startPos = 0;
			capacity = newCap;
		}
		
		void Push(const T& e) {
			if(length + 1 > capacity){
				size_t newCap = capacity;
				while(newCap < length + 1)
					newCap <<= 1;
				Reserve(newCap);
			}
			size_t pos = startPos + length;
			if(pos >= capacity) pos -= capacity;
			ptr[pos] = e;
			length++;
		}
		
		T& Front() {
			return ptr[startPos];
		}
		
		void Shift() {
			SPAssert(length > 0);
			startPos++;
			if(startPos == capacity) startPos = 0;
			length--;
		}
		
		size_t GetLength() const {
			return length;
		}
		
		bool IsEmpty() const {
			return length == 0;
		}
		
	};
}
