#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <ctime>

using namespace std;

string ip = "192.168.50.179";
string humidity = "/humidity\"";
string temp = "/temp\"";

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
  cout << "Testing Temps" << "\n";
  long int start = static_cast<long int> (time(nullptr));
  for (int i = 0; i < 10; i++){
    exec(true);
  }
  long int  end = static_cast<long int> (time(nullptr));
  std::cout << end - start << " seconds, with " << "\n";
    cout << "Testing Humids" << "\n";
  start = static_cast<long int> (time(nullptr));
  for (int i = 0; i < 10; i++){
    exec(false);
  }
  end = static_cast<long int> (time(nullptr));
  std::cout << end - start << " seconds, with " << "\n";

}


int main() {
  for (int i = 0; i < 3; i ++){
    cout << "TEST#" << i << "\n";
    runTests();
  }
}
