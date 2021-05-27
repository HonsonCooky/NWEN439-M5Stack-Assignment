#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <chrono>
#include<unistd.h>

using namespace std::chrono;
using namespace std;

string ip = "192.168.50.179";
string humidity = "/humidity\"";
string temp = "/temp\"";
unsigned int microsecond = 1000000/4;

bool exec(bool isTemp) {
  char buffer[128];
  string result = "";

  // Open pipe to file
  string command = "coap-client -m GET \"coap://" + ip;
  if (!isTemp)
    command = command + humidity;
  else
    command = command + temp;

  FILE* pipe = popen(command.c_str(), "r");
  if (!pipe) {
      return false;
  }

  while (!feof(pipe)) {

    // use buffer to read and add to result
    if (fgets(buffer, 128, pipe) != NULL)
       result += buffer;
  }
  cout << result;
  pclose(pipe);
  return true;
}

void runTests(){
  cout << "Testing Temps" << endl;
  auto start = high_resolution_clock::now();
  for (int i = 0; i < 10; i++){
    exec(true);
    usleep(microsecond);
  }
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(stop - start);
  cout << duration.count()/10 << " microseconds" << endl;

  cout << "Testing Humids" << endl;
  start = high_resolution_clock::now();
  for (int i = 0; i < 10; i++){
    exec(false);
    usleep(microsecond);
  }
  stop = high_resolution_clock::now();
  duration = duration_cast<microseconds>(stop - start);
  cout << duration.count()/10 << " microseconds" << endl;
}


int main() {
  for (int i = 0; i < 3; i ++){
    cout << "TEST#" << i << "\n";
    runTests();
  }
}
