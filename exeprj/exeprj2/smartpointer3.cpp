/*
	shared_ptr: 소유권을 공유하는 스마트 포인터(여러 포인터가 하나의 객체를 공유할 수 있다.
*/

#define _CRT_SECURE_NO_WARNINGS
#include<iostream>

class Myclass {
public:
	Myclass() {
		std::cout << "Myclass() 호출: 객체 생성" << std::endl;
	}

	~Myclass() {
		std::cout << "~Myclass() 호출: 객체 삭제" << std::endl;
	}
};

int main() {

	return 0;
}