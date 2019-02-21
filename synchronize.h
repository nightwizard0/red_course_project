#include <mutex>
using namespace std;

template <typename T>
class Synchronized 
{
public:

  explicit Synchronized(T initial = T())
    : value(move(initial))
  {
  }

  void operator=(T&& other)
  {
      lock_guard<mutex> lock(m);
      value = move(other);
  }

  struct Access 
  {
    T& ref_to_value;
    lock_guard<mutex> guard;
  };

  Access GetAccess() {
    return {value, lock_guard(m)};
  }

private:
  T value;
  mutex m;
};