//This scripts extract the DiffFE ToT matrix out of the entire chip
//optionally it can calculate the mean ToT for the goodPixel mask as well as apply the goodPixeld mask to the DiffFE ToT matrix

//$ ./bin/extract_diff_Th_mean path/to/folder with .txt files/ (nameOfFile to save to )
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <array>

int goodDiff(int row, int col) {
	int good = 10;
	
	std::array<std::array<unsigned, 8>, 8> mask;
	mask[0] = {{0, 1, 1, 1, 0, 0, 0, 0}};
	mask[1] = {{0, 0, 1, 1, 1, 0, 0, 0}};
	mask[2] = {{0, 1, 0, 1, 1, 0, 0, 0}};
	mask[3] = {{0, 1, 0, 1, 1, 0, 0, 0}};
	mask[4] = {{0, 0, 1, 1, 0, 0, 0, 0}};
	mask[5] = {{0, 0, 1, 1, 0, 0, 0, 0}};
	mask[6] = {{0, 0, 1, 0, 0, 0, 0, 0}};
	mask[7] = {{0, 0, 1, 1, 0, 0, 0, 0}};
	
	
		if (mask[row%8][col%8] == 1) good = 1;
		else good = 0;
	
	
	return good;
}

using namespace std;

int main(int argc, char **argv) {

ifstream myfile, myfile2;
string line, line1;
string file = argv[1];
string file2 = argv[2];
string goodPix;
string temp_val;
   
cout << "Diff FE 1st column is 270| last column is 400" << endl;
if (argc > 3){
goodPix = argv[3];}
cout << "argc =" << argc <<endl;

// Extract values from txt file
   myfile.open(file);
   if (!myfile.is_open()) {cout<<"Problem opening the file!"<<endl;}
	ofstream myfile1;
	myfile1.open (file2);
int row=0, count=0;
double sum = 0;
 while (getline(myfile, line))
   {
         cout<< "line length = " << line.size() << endl;
	//copies the plotting info from header
	if (row<=7)
	{       for (unsigned j=0; j<line.length(); j++)
		myfile1 << line[j];
		myfile1 << '\n';
		}		
         
       int col=0; 
	unsigned i=0;
	//looks at number of spaces to see if it got to the Diff FE yet
		while (i <= line.length()-2)
		{
			if (line[i]==' ')
			{ col++;
				unsigned c;
				if (col >= 264)
				{ 	
				//creates .txt file
		
					c=i+1;
					temp_val={};
					while(line[c]!=' ')
					{
					temp_val = temp_val + line[c];
					c++;				
					}
				if ((argc > 3) && (goodPix == "mask")) //applies good pixel mask
					{
					if (goodDiff(row,col)==1) {myfile1 << temp_val << " ";
					sum = sum + stod(temp_val);
					count++;			}
					else myfile1 << "0 ";
					}
				else 	myfile1 << temp_val << " ";								
				}
				//else break;
			}
		
		i++; 	 
		}
 if (row >=8) {myfile1 << '\n';}

 //cout << "i = " << i << endl;
cout << "row " << row << endl; 
row++;	
  }
   myfile.close();
 myfile1.close();

if ((argc > 3) && (goodPix == "mask"))
{
	double mean = sum/count;
	cout << "Mean ToT = " << mean << endl;
}

   return 0;
}
