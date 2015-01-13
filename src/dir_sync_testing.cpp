#include "dir_sync.hpp"
#include <iostream>

int main () {
  dir_sync A_sync("/home/mgentili/A");
  dir_sync B_sync("/home/mgentili/B");
  A_sync.process_dir();
  B_sync.process_dir();

  //A sends estimator to B
  size_t diff_estimate = B_sync.set_difference_estimate(A_sync.estimator);
  //size_t diff_estimate = 100;
  std::cout << "Processed dir A has " << A_sync.filenames.size() << std::endl;
  std::cout << "Processed dir B has " << B_sync.filenames.size() << std::endl;

  std::cout << "Difference estimate is " << diff_estimate << std::endl;
  //B sends set difference estimate to A
  A_sync.create_IBLT(diff_estimate);
  B_sync.create_IBLT(diff_estimate);

  //A sends IBLT to B
  dir_sync::update_info info = B_sync.find_differences(*(A_sync.iblt));
  A_sync.process_differences(info);
  //B sends all relevant info to A
  return 1;
}
