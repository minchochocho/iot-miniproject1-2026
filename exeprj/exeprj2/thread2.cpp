#include<iostream>
#include<thread>
#include<mutex>

std::mutex mtx;
// 공유되는 자원에서 뮤텍스가 필요

void work_inc(int& num) {
	std::lock_guard<std::mutex>lock(mtx);
	for (int i = 0; i < 100000; i++) {
		num += i;
	}
}

void work_des(int& num) {
	std::lock_guard<std::mutex>lock(mtx);
	for (int i = 0; i < 100000; i++) {
		num -= i;
	}
}

int main() {
	int count = 0;

	std::thread t(work_inc, std::ref(count));
	std::thread t2(work_des, std::ref(count));


	t.join();
	t2.join();

	//work_inc(count);
	std::cout << "count: " << count << std::endl;

	return 0;
}