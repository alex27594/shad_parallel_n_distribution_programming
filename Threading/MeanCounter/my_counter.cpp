#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

void sleep(unsigned milliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

class MeanCounterBase {
public:
  virtual ~MeanCounterBase() { }
  
  virtual double mean() = 0;
  virtual void add(int value) = 0;
  
  bool check(double expectedMean) const {
    return std::abs(expectedMean - calcMean()) <
      std::numeric_limits<double>::epsilon();
  }
  
protected:
  double calcMean() const {
    sleep(1000);
    return (double)sum / count;
  }
  
  void doAdd(int value) {
    sleep(1000);
    sum += value;
    sleep(1000);
    count += 1;
  }
  
private:
  long sum = 0;
  long count = 0;
};

void writer(int id, MeanCounterBase& counter, int value) {
  std::printf("[%d] Writer started\n", id);
  counter.add(value);
  std::printf("[%d] Added value: %d\n", id, value);
}

void reader(int id, MeanCounterBase& counter) {
  std::printf("[%d] Reader started\n", id);
  double mean = counter.mean();
  std::printf("[%d] Read mean: %f\n", id, mean);
}

bool check(MeanCounterBase& counter) {
  return counter.check(counter.mean());
}

// === DO NOT REMOVE THIS LINE ===
//// Your solution below

#include <mutex>
#include <condition_variable>
#include <atomic>

class MeanCounter : public MeanCounterBase {

public:
  double mean() override {
    std::unique_lock<std::mutex> lock(m);
    for_readers.wait(lock, [this]() {return (waiting_writers == 0);});
    count_readers++;
    lock.unlock();
    double res = calcMean();
    m.lock();
    if (--count_readers == 0) {
      for_writers.notify_one();
    }
    m.unlock();
    return res;
  }
  
  void add(int value) override {
    std::unique_lock<std::mutex> lock(m);
    waiting_writers++;
    for_writers.wait(lock, [this]() {return count_readers == 0;});
    waiting_writers--;
    doAdd(value);
    if (waiting_writers == 0) {
      for_readers.notify_all();
    }
  }

private:
  int count_readers = 0;
  int waiting_writers = 0;
  std::mutex m;
  std::condition_variable for_writers;
  std::condition_variable for_readers;

//another variant
/*
public:
  double mean() override {
    global_mutex.lock();
    read_mutex.lock();
    count_readers++;
    if (count_readers == 1) {
      write_mutex.lock();
    }
    read_mutex.unlock();
    global_mutex.unlock();

    double mean_val = calcMean();
    
    read_mutex.lock();
    count_readers--;
    if (count_readers == 0) {
      write_mutex.unlock();
    }
    read_mutex.unlock();
    return mean_val;
  }
  
  void add(int value) override {
    global_mutex.lock();
    write_mutex.lock();
    
    doAdd(value);
    
    write_mutex.unlock();
    global_mutex.unlock();
  }
private:
  int count_readers = 0;
  int waiting_writers = 0;
  std::mutex global_mutex;
  std::mutex read_mutex;
  std::mutex write_mutex;
  std::condition_variable for_writers;
  std::condition_variable for_readers;
 */

};

//// Your solution above
// === DO NOT REMOVE THIS LINE ===

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::fprintf(stderr, "Usage: %s trace_file\n", argv[0]);
    return 1;
  }
  
  MeanCounter counter;
  std::vector<std::thread> threads;
  
  std::ifstream f(argv[1]);
  int thread_id = 0;
  while(f) {
    std::string op;
    f >> op;
    if (op == "W") {
      int value;
      f >> value;
      threads.emplace_back(writer, thread_id, std::ref(counter), value);
    } else if (op == "R") {
      threads.emplace_back(reader, thread_id, std::ref(counter));
    } else if (!op.empty()) {
      std::fprintf(stderr, "Unknown op: %s\n", op.c_str());
      break;
    }
    thread_id++;
    sleep(500);
  }
  
  for (auto& thread : threads) {
    thread.join();
  }
  
  if (!check(counter)) {
    std::fprintf(stderr, "Make sure you are using MeanCounterBase\n");
    return 2;
  }
}
