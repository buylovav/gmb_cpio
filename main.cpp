
#include <cstdlib>
#include <iostream>
#include <string>

#include "cpio_api.h"

using namespace std;

const string PUT  = "new";
const string GET  = "get";
const string SHOW = "show";

CPIO_FILES fillInFiles()
{
  CPIO_FILES files;
  string line;
  getline(cin, line);
  string file;

  string delimiter = " ";

  size_t pos = 0;
  string token;
  while ((pos = line.find(delimiter)) != string::npos) {
      token = line.substr(0, pos);
      if (!token.empty())
        files.push_back(token);
      line.erase(0, pos + delimiter.length());
  }
  if (!line.empty())
    files.push_back(line);
  return files;
}

int main(int argc, char** argv) {
  cout << "Welcome to the cpio_api demo app\n";
  while (1){
    cout << "\n-- To create an archive and add files type \'"<< PUT <<" archive file1 file2 ... filen\'\n";
    cout << "-- To see all the files in the archive type \'"<< SHOW <<" archive\'\n";
    cout << "-- To extract files from the archive type \'"<< GET <<" file1 file2 ... filen\'\n";
    cout << "-- Enter Q for exit\n>>";

    string cmd;
    cin >> cmd;

    if (cmd=="Q" || cmd=="q")
      break;

    string archf;

    cin >> archf;

    try
    {
      if (cmd==SHOW)
      {
        cout << "\n Files in the "<< archf <<": \n";
        auto files = show(archf);
        for (auto& file:files)
        {
          cout << "   " << file << "\n";
        }
      }
      else if (cmd==PUT)
      {
        auto files = fillInFiles();
        put(archf, files);
        cout << "  Put it('em) successfully\n";
      }
      else if (cmd==GET)
      {
        auto files = fillInFiles();      
        get(archf, files);
        cout << "  Got it('em) successfully\n";
      }
      else
      {
        cout << "  !!! THERE IS NO SUCH COMMAND !!!\n";
      }
    }
    catch(exception& e){
      cout << "!!! An error occurred: " << e.what() << endl;
    }
    catch(...){
      cout << "Something went completely wrong. Terminating...";
      return 1;
    }
    
  }
  return 0;
}

