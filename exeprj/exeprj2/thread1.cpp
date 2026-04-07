// 멀티 스레드: 여러 작업을 동시 수행
#include<iostream>
#include<thread>

void work() {
	for (int i = 0; i <= 20; i++) {
		std::cout << "작업 스레드: " << i << std::endl;
	}
}

int main() {
	std::thread t(work);	// work 함수를 새 스레드에서 실행하겠다
	// work();
	for (auto i = 1; i <= 20; i++) {
		std::cout << "메인 스레드 : " << i << std::endl;
	}
	t.join();
	return 0;
}