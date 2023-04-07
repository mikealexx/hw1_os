#include <iostream>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

string trimToVector(char* string) {
  std::string str(string);
  std::string trimmedStr;
  for(int i = 0; i < str.length();) {
    if(str[i] == ' ') {
      if(i == 0 || i == str.length()-1) {
        i++;
        continue;
      }
      while(str[i+1] == ' '){
        i++;
      }
    }
    trimmedStr += str[++i];
  }
  return trimmedStr;
}

vector<std::string> trimmedToVector(std::string str) {
    std::stringstream ss(str);
    std::string word;
    std::vector<std::string> result;
    while (ss >> word) {
        result.push_back(word);
    }
    return result;
} 

int main() {
    cout << "Not Trimmed:    Hello   my name    is        mike     !   " << endl;
    string trimmed = trimToVector("    Hello   my name    is        mike     !   ");
    cout << "Trimmed: " << trimmed << endl;
    std::vector<std::string> vec = trimmedToVector(trimmed);
    for(std::string word : vec) {
        cout << word;
    }
    cout << "." << endl;
    return 0;
}