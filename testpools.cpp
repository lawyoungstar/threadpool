#include "thread_pool.h"

void f() {
	std::cout<<"thread ID: "<<std::this_thread::get_id()<<"hello, f!"<<std::endl;
}

struct G
{
	int operator()() {
		std::cout<<"thread ID: "<<std::this_thread::get_id()<<"hello , G!"<<std::endl;
		return 10;
	}
	/* data */
};

int main() {
	/*
	std::cout << "countdown:\n";
  	for (int i=10; i>0; --i) {
    	std::cout << i << std::endl;
    	std::this_thread::sleep_for (std::chrono::seconds(1));
  	}
  	std::cout << "Lift off!\n";
  	*/

/*
  	myThreadpool::threadpool pool(2);
  	std::vector<std::future<int> > results;

    for(int i = 0; i < 8; ++i) {
        results.emplace_back(
            pool.commit([i] {
                std::cout << "hello " << std::this_thread::get_id() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "world " << i << std::endl;
                return i*i;
            })
        );
    }

    for(auto && result: results)
        std::cout << result.get() << ' ';
    std::cout << std::endl;
    
    return 0;
    */


	try {
		myThreadpool::threadpool executor(2);

		std::future<void> ff = executor.commitTask(f);
		std::future<int> fg = executor.commitTask(G{});
		std::future<std::string> fh = executor.commitTask( []()->std::string {std::cout<<"thread ID: "<<std::this_thread::get_id()<<"hello, string h!"<<std::endl;
			return "string h!";});

		executor.shutdown();
		std::cout<<"shutdown pool"<<std::endl;

		ff.get();
    	std::cout << fg.get() << " " << fh.get() << std::endl;
    	std::this_thread::sleep_for(std::chrono::seconds(5));
    	std::cout<<"restart..."<<std::endl;
    	executor.restart();    // 重启任务
    	std::cout<<"restart success! ready to execute function f..."<<std::endl;
    	std::future<int> f2 = executor.commitTask(G{});    //
    	std::cout<<f2.get()<<std::endl;
    
    	std::cout << "end..." << std::endl;
    	executor.exit();
    	return 0;
	}
	catch(std::exception& e) {
    	std::cout << "some unhappy happened... " << e.what() << std::endl;
    	return -1;
	}
	
}