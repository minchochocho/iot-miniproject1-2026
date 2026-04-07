#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
using namespace std;

class Human {
private:
	char* name;
	int age;
public:
	Human(const char* name, int age) {
		cout << "일반 생성자" << endl;
		int length = strlen(name);
		this->name = new char[length];
		strcpy(this->name, name);
		this->age = age;
	}

	Human(const Human& other) {
		cout << "복사 생성자" << endl;
		int length = strlen(other.name);
		this->name = new char[length];
		strcpy(this->name, other.name);
		this->age = other.age;
	}

	Human(Human&& other) noexcept {
		cout << "이동 생성자" << endl;
		name = other.name;
		age = other.age;

		other.name = nullptr;
		other.age = 0;
		// 주소를 옮기는 것
		// 원래 있던 곳에 초기화 시켜줘야 함
		// 이동 생성자에는 const 치워야댐쓰
	}
	void viewHuman() {
		cout << "이름 : " << name << ", 나이 : " << age << endl;
	}
};


int main() {
	Human h("홍길동", 100);

	h.viewHuman();

	Human h2(h);
	h2.viewHuman();

	Human h3(move(h));
	h3.viewHuman();

	                                                                                                                                                                                                                                                                    
	return 0;
}