#include <iostream>
using namespace std;


template<typename T>
class A {
	int num;
public:
	A(int data) : num(data){}	// : 초기화 리스트
};

template<typename T>	// T - 자료형 이름
// 모든 자료형을 T로 대신 사용할 수 잇음, 객체지향 형식의 다형성을 위해 사용
T Add(T a, T b) {
	return a + b;
}

template<typename T>
void Swap(T& a, T& b) {
	T tmp;
	tmp = a;
	a = b;
	b = tmp;
	cout << "Swap" << endl;
}


int main() {
	double res = Add<double>(10.1, 20.2);	// <double> 생략가능
	cout << res << endl;
	// int a = 10, b = 20;
	char a = 'a', b = 'b';

	cout << "a = " << a << ", b = " << b << endl;
	Swap<char> (a, b);	
	cout << "a = " << a << ", b = " << b << endl;

	

	return 0;
}