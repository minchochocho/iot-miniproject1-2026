/*
	객체 포인터의 다형성으로 기초클래스 타입의 포인터로 파생 클래스의 객체를 가리키면 기초클래스 멤버에만 접근됨
	이때 오버라딩된 파생 클래스의 멤버함수를 사용하기 위해서는 기초클래스의 멤버함수를 가상 함수로 만들면 된다.
	순수 지정자를 가지는 순수 가상 함수를 포함하는 클래스는 추상 클래스이다
	추상 클래스는 객체를 생성할 수 없다
*/
#include<iostream>

class Base {
public:
	~Base(){}
	virtual void func1() { std::cout << "B::func1" << std::endl; }
	virtual void func2() { std::cout << "B::func2" << std::endl; }
	void func3() { std::cout << "B::func3" << std::endl; }
	virtual void f() = 0;		// 순수 가상 함수
	// 순수 가상함수를 가지고 있으면 객체를 생성할 수 없음
};

class Derived : public Base {
public:
	void func1() override { std::cout << "D::func1" << std::endl; }
	void func2() override { std::cout << "D::func2" << std::endl; }
	void func3() { std::cout << "D::func3" << std::endl; }
	void func4() { std::cout << "D::func4" << std::endl; }
	void f() override {} // 재정의 하면 자식객체는 사용가능
};

int main() {
	Base b;	// 순수 가상함수로 인해 생성이 안되는 모습
	Derived d;

	b.func1();
	d.func1();
	std::cout << std::endl;

	// 상속이라서 가능
	Base* pb = new Derived();		// 객체 포인터와 객체의 타입이 다르다
	//Derived* pd = new Base();		// 자식이 부모를 가리킬수는 없다
	pb->func1();
	pb->func2();
	pb->func3();
	// pb->func4();

	delete pb;

	return 0;
}