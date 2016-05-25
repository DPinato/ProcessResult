/*
 * ProcessResult.cpp
 *
 *  Created on: 12 Apr 2016
 *      Author: davide
 */

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <cstdint>
#include <algorithm>

using namespace std;


// function declaration
int main(int argc, char *argv[]);
bool parseCommandLine(int argc, char *argv[]);
bool readMeasFlows(string file);
void categoriseFlows();

bool dumpMeasFlowStats(string file);
bool dumpCDF_FCT(string file);
bool dumpFCTvsFlowSize(string file);
bool dumpCDF_thr(string file);

bool dumpCatCDF_FCT(string file, int cat);
bool dumpCatFCTvsFlowSize(string file, int cat);
bool dumpCatCDF_thr(string file, int cat);

bool writeThrStats(string file);
bool writeFCTStats(string file);



string outputDir;	// directory to save the output in

struct flowMeas {	// collect stats of measured flows from _RESULTS_x.data file
	uint64_t fct;		// flow completion time, ns
	double thr;			// flow average throughput, Mbps
	uint64_t size;	// flow desired size, bytes
};
vector<flowMeas> flowsMeasured;
vector<int> sizeCategory;			// list of size categories
vector<flowMeas> *flowsMeasuredCat;	// list of flow measured per category
string resultFile;


// compare functions
bool compareByFCT(const flowMeas &a, const flowMeas &b);
bool compareByFlowSize(const flowMeas &a, const flowMeas &b);
bool compareByThr(const flowMeas &a, const flowMeas &b);




int main(int argc, char *argv[]) {
	// argv[1] needs to be the "...RESULT_x.data" file to process
	// if there is no argv[2], the output files will be saved in the same directory of the input file
	cout << "ProcessResult is starting..." << endl;

	// DEBUG
	argc = 2;
	argv[1] = "/home/davide/Desktop/tmp/RAW/TCP/S11_F_SFST_SFTCP_FT_512_TCP_SHORT_FLOW_RESULT_1.data";
//	argv[1] = "/home/davide/Desktop/MMPTCP_RESULTS/12042016/DCTCP_1-1_DCTCP_02x/DCTCP_1-1_DCTCP_02x_FT_128_TCP_SHORT_FLOW_RESULT_1.data";
//	argv[2] = "/home/davide/Desktop/MMPTCP_RESULTS/12042016/tmp";		// output directory
	//////

	if (!parseCommandLine(argc, argv)) {
		cout << "Incorrect or incomplete command line arguments" << endl;
		exit(EXIT_FAILURE);
	}



	int iter = 10;	// select how many iteration of this simulation were done
	string *resultFileList;
	resultFileList = new string [iter];
	if (iter > 0) {
		// precompute all variation for the input result file coming from argv[1]
		for (int i = 0; i < iter; i++) {
			resultFileList[i] = string(argv[1]).substr(0, string(argv[1]).length()-6);
			resultFileList[i].append(to_string(i+1)+".data");	// start from _1
			cout << resultFileList[i] << endl;
		}
	}




	// read flows from file
	if (iter > 0) {
		for (int i = 0; i < iter; i++) {
			readMeasFlows(resultFileList[i]);
		}
	} else {
		readMeasFlows(resultFile);
	}
	cout << "I read " << flowsMeasured.size() << " flows" << endl;


	// select size categories
//	sizeCategory.push_back(128*1024);		// 128KB
//	sizeCategory.push_back(512*1024);		// 512KB
//	sizeCategory.push_back(1*1024*1024);	// 1MB
//	sizeCategory.push_back(10*1024*1024);	// 10MB
	cout << "I have " << sizeCategory.size() << " flow size categories: ";
	for (int i = 0; i < (int)sizeCategory.size(); i++) {
		cout << sizeCategory.at(i) << "\t";
	}
	cout << "\n";


	// allocate memory for flowsMeasuredCat
	flowsMeasuredCat = new vector<flowMeas> [(int)sizeCategory.size()+1];



	cout << "Writing plotabble files for NON CATEGORISED flows...";
	dumpMeasFlowStats(string(outputDir + "measFlowStats.dat"));

	// output plottable .dat files for NON CATEGORISED FLOWS
	// 1) increasing FCT (CDF)
	// 2) increasing avg throughput (CDF)
	// 3) FCT over flow size (ordered in increasing flow size)
	// 1) CDF of FCT
	sort(flowsMeasured.begin(), flowsMeasured.end(), compareByFCT);
	dumpCDF_FCT(string(outputDir + "CDF_FCT.dat"));

	// 2) CDF of thr
	sort(flowsMeasured.begin(), flowsMeasured.end(), compareByThr);
	dumpCDF_thr(string(outputDir + "CDF_thr.dat"));

	// 3) FCT over flow size (ordered in increasing flow size)
//	sort(flowsMeasured.begin(), flowsMeasured.end(), compareByFlowSize);
//	dumpFCTvsFlowSize("FCT_FlowSize.dat");

	cout << "DONE" << endl;





/*
	// categorise flows in flow size categories
	cout << "Categorising flows by size..." << endl;
	categoriseFlows();


	for (int i = 0; i < (int)sizeCategory.size(); i++) {
		cout << "\tI have " << flowsMeasuredCat[i].size() << " flows below " << sizeCategory.at(i) << endl;
	}
	cout << "\tI have " << flowsMeasuredCat[sizeCategory.size()].size()
			<< " flows above " << sizeCategory.at(sizeCategory.size()-1) << endl;


	cout << "Writing plotabble files for CATEGORISED flows...";

	// plot categorised flows to file
	// 1) increasing FCT (CDF)
	// 2) increasing avg throughput (CDF)
	// 3) FCT over flow size (ordered in increasing flow size)
	for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
		// 1) CDF of FCT
		sort(flowsMeasuredCat[i].begin(), flowsMeasuredCat[i].end(), compareByFCT);
		dumpCatCDF_FCT(string(outputDir + "CDF_FCT.dat"), i);

		// 2) CDF of thr
		sort(flowsMeasuredCat[i].begin(), flowsMeasuredCat[i].end(), compareByThr);
		dumpCatCDF_thr(string(outputDir + "CDF_thr.dat"), i);

		// 3) FCT over flow size (ordered in increasing flow size)
//		sort(flowsMeasuredCat[i].begin(), flowsMeasuredCat[i].end(), compareByFlowSize);
//		dumpCatFCTvsFlowSize("FCT_FlowSize.dat", i);


	}
	cout << "DONE" << endl;




	cout << "Writing percentile statistics...";
	// write FCT stats, per size category
	for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
		sort(flowsMeasuredCat[i].begin(), flowsMeasuredCat[i].end(), compareByFCT);
	}
	writeFCTStats(string(outputDir + "stats_FCT.dat"));


	// write throughput stats, per size category
	for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
		sort(flowsMeasuredCat[i].begin(), flowsMeasuredCat[i].end(), compareByThr);
	}
	writeThrStats(string(outputDir + "stats_thr.dat"));

	cout << "DONE" << endl;

*/




//	uint64_t sum = 0;
//	for (int i = 0; i < (int)flowsMeasuredCat[0].size(); i++) {
//		sum += flowsMeasuredCat[0].at(i).fct;
//	}
//	cout << "sum: " << sum;
//	cout << "\tavg: " << ((double)sum/(int)flowsMeasuredCat[0].size())/1000000.0;
//	cout << endl;



	cout << "\n\n";
	cout << "ProcessResult is ending..." << endl;
	return 0;

}


bool parseCommandLine(int argc, char *argv[]) {
	// parse input from command line and set appropriate flags for processing
	// command line arguments available:
	// 	-it <value>: number of iteration in the simulation, e.g. number of "_RESULT_" files
	//	-in <input_file>: first "_RESULT_" file
	// 	-out <output_directory>: if not specified, use location of <input_file>
	bool setIt = false;
	bool setIn = false;
	bool setOut = false;
	string commands[] = {"-it", "-in", "-out"};






	if (argc < 2) {
		cout << "Enter _RESULT_x.data file to process and output directory" << endl;
		exit(EXIT_FAILURE);
	}


	resultFile = string(argv[1]);
	if (argc > 2) {
		outputDir = string(argv[2]);
		if (outputDir.at(outputDir.length()-1) != '/') { outputDir.append("/");	}
	} else {
		// by default, put the output where the input RESULT file is
		outputDir = resultFile.substr(0, resultFile.find_last_of('/')+1);
	}
	cout << "Output to " << outputDir << endl;




	// parsing went well
	return true;

}



bool readMeasFlows(string file) {
	// read line by line, extract flow stats and store them in vector

	ifstream read(file.c_str());
	if (read.is_open()) {
		string line;

		while(getline(read, line)) {
			// flow size is between [>...<], it is in bytes
			// throughput is between [#...#], it is in Mbps
			// FCT is between [*...*], it is in seconds, asjust it in nanoseconds

			if (line.at(0) == '[') {
				flowMeas temp;
				int pos1 = 0;
				int pos2 = 0;

				// get flow size,
				pos1 = line.find("[>");
				pos2 = line.find("<]", pos1);
//				cout << "pos1: " << pos1 << "\tpos2: " << pos2 << endl;
//				cout << line.substr(pos1+2, pos2-pos1-2) << endl;
				temp.size = strtoull(line.substr(pos1+2, pos2-pos1-2).c_str(), NULL, 10);
//				cout << "size: " << temp.size << endl;

				// get throughput
				pos1 = line.find("[#");
				pos2 = line.find("#]", pos1);
//				cout << "pos1: " << pos1 << "\tpos2: " << pos2 << endl;
//				cout << line.substr(pos1+2, pos2-pos1-2) << endl;
				temp.thr = strtod(line.substr(pos1+2, pos2-pos1-2).c_str(), NULL);
//				cout << "thr: " << temp.thr << endl;

				// get FCT
				pos1 = line.find("[*");
				pos2 = line.find("*]", pos1);
//				cout << "pos1: " << pos1 << "\tpos2: " << pos2 << endl;
//				cout << line.substr(pos1+2, pos2-pos1-2) << endl;
				temp.fct = (uint64_t)1000000000.0*strtod(line.substr(pos1+2, pos2-pos1-2).c_str(), NULL);
//				cout << "fct: " << temp.fct << endl;

				flowsMeasured.push_back(temp);

			}
		}

	} else {
		cout << "Could not open file" << endl;
		return false;
	}


	return true;

}

void categoriseFlows() {

	for (int i = 0; i < (int)flowsMeasured.size(); i++) {

		// this is the dynamic way
		int iS;
		for (iS = 0; iS < (int)sizeCategory.size(); iS++) {
			if ((int)flowsMeasured.at(i).size <= sizeCategory.at(iS)) {
				break;
			} else if ((int)flowsMeasured.at(i).size > sizeCategory.at((int)sizeCategory.size()-1)) {
				iS = (int)sizeCategory.size();
				break;
			}
		}

//		cout << "i: " << i << "\tiS: " << iS << endl;
		flowsMeasuredCat[iS].push_back(flowsMeasured.at(i));


//		// this is the static way
//		if ((int)flowsMeasured.at(i).size <= sizeCategory.at(0)) {
////			cout << 1 << endl;
//			flowsMeasuredCat[0].push_back(flowsMeasured.at(i));
//		} else if ((int)flowsMeasured.at(i).size <= sizeCategory.at(1) && (int)flowsMeasured.at(i).size > sizeCategory.at(0)) {
////			cout << 2 << endl;
//			flowsMeasuredCat[1].push_back(flowsMeasured.at(i));
//		} else if ((int)flowsMeasured.at(i).size <= sizeCategory.at(2) && (int)flowsMeasured.at(i).size > sizeCategory.at(1)) {
////			cout << 3 << endl;
//			flowsMeasuredCat[2].push_back(flowsMeasured.at(i));
//		} else if ((int)flowsMeasured.at(i).size <= sizeCategory.at(3) && (int)flowsMeasured.at(i).size > sizeCategory.at(2)) {
////			cout << 4 << endl;
//			flowsMeasuredCat[3].push_back(flowsMeasured.at(i));
//		} else if ((int)flowsMeasured.at(i).size > sizeCategory.at(3)) {
////			cout << 5 << endl;
//			flowsMeasuredCat[4].push_back(flowsMeasured.at(i));
//		}

	}
}

bool dumpMeasFlowStats(string file) {
	// dump all measured flows stats to file
	ofstream write(file.c_str());
	if (write.is_open()) {
		// write header
		write << "# Packet size here is 1400 Bytes" << "\n";
		write << "#FCT(ns)\tthr(Mbps)\tfSize(Bytes)" << "\n";
		for (int i = 0; i < (int)flowsMeasured.size(); i++) {
			write << flowsMeasured.at(i).fct;
			write << "\t" << flowsMeasured.at(i).thr;
			write << "\t" << flowsMeasured.at(i).size;

			write << "\n";
		}
		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpMeasFlowStats(...)"<< endl;
		return false;
	}
}

bool dumpCDF_FCT(string file) {
	// write CDF of FCT to file
	ofstream write(file.c_str());
	if (write.is_open()) {
		// write header
		write << "#CDF\tFCT(ns)" << "\n";
		for (int i = 0; i < (int)flowsMeasured.size(); i++) {
			write << (double)(i+1)/(double)flowsMeasured.size();
			write << "\t" << flowsMeasured.at(i).fct;
			write << "\n";
		}
		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpCDF_FCT(...)"<< endl;
		return false;
	}
}

bool dumpFCTvsFlowSize(string file) {
	// write FCT for increasing flow sizes
	ofstream write(file.c_str());
	if (write.is_open()) {
		// write header
		write << "# Be careful, here the packet size was 1400 Bytes" << "\n";
		write << "#FCT(ns)\tFlowSize(pkts)" << "\n";
		for (int i = 0; i < (int)flowsMeasured.size(); i++) {
			write << flowsMeasured.at(i).fct;
			write << "\t" << flowsMeasured.at(i).size;
			write << "\n";
		}
		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpFCTvsFlowSize(...)"<< endl;
		return false;
	}
}

bool dumpCDF_thr(string file) {
	// write CDF of avg throughput to file
	ofstream write(file.c_str());
	if (write.is_open()) {
		// write header
		write << "#CDF\tavg_thr(Mbps)" << "\n";
		for (int i = 0; i < (int)flowsMeasured.size(); i++) {
			write << (double)(i+1)/(double)flowsMeasured.size();
			write << "\t" << flowsMeasured.at(i).thr;
			write << "\n";
		}
		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpCDF_thr(...)"<< endl;
		return false;
	}
}

bool dumpCatCDF_FCT(string file, int cat) {
	// write CDF of FCT, for flows in this flow category to file
	file = string(file.substr(0, file.length()-4));
	file.append("_" + to_string(cat));
	file.append(".dat");

	ofstream write(file.c_str());
	if (write.is_open()) {
		// write header
		if (cat == 0) {
			write << "# flows below " << sizeCategory.at(0) << " Bytes";
		} else if (cat <= (int)sizeCategory.size()-1) {
			write << "# flows between " << sizeCategory.at(cat-1) << " and " << sizeCategory.at(cat) << " Bytes";
		} else if (cat == (int)sizeCategory.size()) {
			write << "# flows above " << sizeCategory.at((int)sizeCategory.size()-1) << " Bytes";
		}
		write << "\n";

		write << "#CDF\tFCT(ns)" << "\n";
		for (int i = 0; i < (int)flowsMeasuredCat[cat].size(); i++) {
			write << (double)(i+1)/(double)flowsMeasuredCat[cat].size();
			write << "\t" << flowsMeasuredCat[cat].at(i).fct;
			write << "\n";
		}
		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpCatCDF_FCT(...)"<< endl;
		return false;
	}

}

bool dumpCatFCTvsFlowSize(string file, int cat) {
	// write FCT for increasing flow sizes, for flows in this flow category to file
	file = string(file.substr(0, file.length()-4));
	file.append("_" + to_string(cat));
	file.append(".dat");

	ofstream write(file.c_str());
	if (write.is_open()) {
		// write header
		if (cat == 0) {
			write << "# flows below " << sizeCategory.at(0) << " Bytes";
		} else if (cat <= (int)sizeCategory.size()-1) {
			write << "# flows between " << sizeCategory.at(cat-1) << " and " << sizeCategory.at(cat) << " Bytes";
		} else if (cat == (int)sizeCategory.size()) {
			write << "# flows above " << sizeCategory.at((int)sizeCategory.size()-1) << " Bytes";
		}
		write << "\n";

		write << "# Packet size was 1400 Bytes" << "\n";
		write << "#FCT(ns)\tFlowSize(pkts)" << "\n";
		for (int i = 0; i < (int)flowsMeasuredCat[cat].size(); i++) {
			write << flowsMeasuredCat[cat].at(i).fct;
			write << "\t" << flowsMeasuredCat[cat].at(i).size;
			write << "\n";
		}
		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpCatFCTvsFlowSize(...)"<< endl;
		return false;
	}

}

bool dumpCatCDF_thr(string file, int cat) {
	// write CDF of avg throughput, for flows in this flow category to file
	file = string(file.substr(0, file.length()-4));
	file.append("_" + to_string(cat));
	file.append(".dat");

	ofstream write(file.c_str());
	if (write.is_open()) {
		// write header
		if (cat == 0) {
			write << "# flows below " << sizeCategory.at(0) << " Bytes";
		} else if (cat <= (int)sizeCategory.size()-1) {
			write << "# flows between " << sizeCategory.at(cat-1) << " and " << sizeCategory.at(cat) << " Bytes";
		} else if (cat == (int)sizeCategory.size()) {
			write << "# flows above " << sizeCategory.at((int)sizeCategory.size()-1) << " Bytes";
		}
		write << "\n";

		write << "#CDF\tavg_thr(Mbps)" << "\n";
		for (int i = 0; i < (int)flowsMeasuredCat[cat].size(); i++) {
			write << (double)(i+1)/(double)flowsMeasuredCat[cat].size();
			write << "\t" << flowsMeasuredCat[cat].at(i).thr;
			write << "\n";
		}
		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpCatCDF_thr(...)"<< endl;
		return false;
	}

}

bool writeThrStats(string file) {
	// write throughput stats for every flow size category
	// focus on the tail
	// For CDF: 90%, 95%, 99%, 99.9%, 99.99%

	ofstream write(file.c_str());
	if (write.is_open()) {
		// write header
		write << "#FCT for 90%, 95%, 99%, 99.9% and 99.99% for all different flow size categories\n";
		write << "#Percentile\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			write << "cat" << i << "(Mbps)\t";
		}
		write << "\n";


		// write the average
		write << "avg\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			double sum = 0;
			for (int iA = 0; iA < (int)flowsMeasuredCat[i].size(); iA++) {
				sum += flowsMeasuredCat[i].at(iA).thr;
			}
			write << sum/(double)flowsMeasuredCat[i].size() << "\t";
		}
		write << "\n";

/*		// write 50%
		write << "50%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.5;
			write << flowsMeasuredCat[i].at(index).thr << "\t";
		}
		write << "\n";*/

		// write 90%
		write << "90%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.9;
			write << flowsMeasuredCat[i].at(index).thr << "\t";
		}
		write << "\n";

		// write 95%
		write << "95%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.95;
			write << flowsMeasuredCat[i].at(index).thr << "\t";
		}
		write << "\n";

		// write 99%
		write << "99%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.99;
			write << flowsMeasuredCat[i].at(index).thr << "\t";
		}
		write << "\n";

		// write 99.9%
		write << "99.9%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.999;
			write << flowsMeasuredCat[i].at(index).thr << "\t";
		}
		write << "\n";

		// write 99.99%
		write << "99.99%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.9999;
			write << flowsMeasuredCat[i].at(index).thr << "\t";
		}
		write << "\n";


		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpCDF_FCT(...)"<< endl;
		return false;
	}
}

bool writeFCTStats(string file) {
	// write FCT stats for every flow size category
	// focus on the tail
	// For CDF: 90%, 95%, 99%, 99.9%, 99.99%

	ofstream write(file.c_str());
	if (write.is_open()) {
		// write header
		write << "#FCT for avg, 50%, 90%, 95%, 99%, 99.9% and 99.99% for all different flow size categories\n";
		write << "#Percentile\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			write << "cat" << i << "(ms)\t";
		}
		write << "\n";


		// write the average
		write << "avg\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			double sum = 0;
			for (int iA = 0; iA < (int)flowsMeasuredCat[i].size(); iA++) {
				sum += flowsMeasuredCat[i].at(iA).fct/1000000.0;
			}
			write << sum/(double)flowsMeasuredCat[i].size() << "\t";
		}
		write << "\n";

/*		// write 50%
		write << "50%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.5;
			write << flowsMeasuredCat[i].at(index).fct/1000000.0 << "\t";
		}
		write << "\n";*/

		// write 90%
		write << "90%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.9;
			write << flowsMeasuredCat[i].at(index).fct/1000000.0 << "\t";
		}
		write << "\n";

		// write 95%
		write << "95%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.95;
			write << flowsMeasuredCat[i].at(index).fct/1000000.0 << "\t";
		}
		write << "\n";

		// write 99%
		write << "99%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.99;
			write << flowsMeasuredCat[i].at(index).fct/1000000.0 << "\t";
		}
		write << "\n";

		// write 99.9%
		write << "99.9%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.999;
			write << flowsMeasuredCat[i].at(index).fct/1000000.0 << "\t";
		}
		write << "\n";

		// write 99.99%
		write << "99.99%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.9999;
			write << flowsMeasuredCat[i].at(index).fct/1000000.0 << "\t";
		}
		write << "\n";


		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpCDF_FCT(...)"<< endl;
		return false;
	}
}







bool compareByFCT(const flowMeas &a, const flowMeas &b) {
	return a.fct < b.fct;
}

bool compareByFlowSize(const flowMeas &a, const flowMeas &b) {
	return a.size < b.size;
}

bool compareByThr(const flowMeas &a, const flowMeas &b) {
	return a.thr < b.thr;
}








