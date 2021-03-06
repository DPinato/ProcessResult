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
bool generateResultFileList(int it, string file);
bool doStdProc();
bool doFairProc();
bool doJobProc();

bool getSizeCategories();
bool readMeasFlows(string file);
bool readMeasJobs(string file);
void categoriseFlows();

bool dumpMeasFlowStats(string file);
bool dumpCDF_FCT(string file);
bool dumpShortCDF_FCT(string file);
bool dumpShortCDF_FCT(string file, int cdfPoints);
bool dumpFCTvsFlowSize(string file);
bool dumpCDF_thr(string file);
bool dumpLargeCDF_thr(string file);
bool dumpLargeCDF_thr(string file, int cdfPoints);
bool dumpFairCDF_thr(string file, int flowType);
bool dumpFairCDF_thr(string file, int flowType, int cdfPoints);
bool dumpCDF_JCT(string file);
bool dumpCDF_JCT(string file, int cdfPoints);


bool dumpCatCDF_FCT(string file, int cat);
bool dumpCatFCTvsFlowSize(string file, int cat);
bool dumpCatCDF_thr(string file, int cat);

bool writeThrStats(string file);
bool writeFCTStats(string file);



string procMode;	// how the input files will be processed
string outputDir;	// directory to save the output in

struct flowMeas {	// collect stats of measured flows from _RESULTS_x.data file
	uint64_t fct;		// flow completion time, ns
	double thr;			// flow average throughput, Mbps
	uint64_t size;		// flow desired size, bytes
};
vector<flowMeas> flowsMeasured;
vector<flowMeas> shortFlowsMeasured, largeFlowsMeasured;
vector<flowMeas> tcpFlowTypeList[3];

vector<int> sizeCategory;			// list of size categories
vector<flowMeas> *flowsMeasuredCat;	// list of flow measured per category
string resultFile;
bool doSizeCat = false;		// flags whether processing of flow statistics by the specified flow categories will be done
int iter = -1;		// select how many iterations of this simulation were done
string *resultFileList;	// stores list of "_RESULT_" file location

string tcpFlowTypes[] = {"TCP", "DCTCP", "DCMPTCP"};
int unknownFlow = 0;


struct jobMeas {
	uint64_t jct;		// job completion time, ns
};
vector<jobMeas> jobsMeasured;




// compare functions
bool compareByFCT(const flowMeas &a, const flowMeas &b);
bool compareByJCT(const jobMeas &a, const jobMeas &b);
bool compareByFlowSize(const flowMeas &a, const flowMeas &b);
bool compareByThr(const flowMeas &a, const flowMeas &b);




int main(int argc, char *argv[]) {
	// argv[1] flags the type of processing desired, "std", "fair" or "job"
	// argv[2] needs to be the "...RESULT_x.data" file to process
	// if there is no argv[3], the output files will be saved in the same directory of the input file
	cout << "ProcessResult is starting..." << endl;

	// DEBUG
	argc = 3;
	argv[1] = "job";
	argv[2] = "/home/davide/Desktop/qmb_ON_result/RAW/S1_INCAST_CT_1G_SF_8_FT_128/S1_INCAST_CT_1G_SF_8_FT_128_MPTCP_INCAST_DIST_JOB_1.data";
//	argv[3] = "/home/davide/Desktop/MMPTCP_RESULTS/12042016/tmp";		// output directory
	//////

	if (!parseCommandLine(argc, argv)) {	// THIS IS NOT COMPLETELY DONE YET
		cout << "Incorrect or incomplete command line arguments" << endl;
		exit(EXIT_FAILURE);
	}

	iter = 1;	// select how many iteration of this simulation were done
	generateResultFileList(iter, string(argv[2]));



	if (procMode == "std") {
		// do standard processing
		doStdProc();
	} else if (procMode == "fair") {
		// process for fairness
		doFairProc();
	} else if (procMode == "job") {
		// process jobs
		doJobProc();
	}







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






	if (argc < 3) {
		cout << "Enter the output desired, .data file to process" << endl;
		exit(EXIT_FAILURE);
	}


	resultFile = string(argv[2]);
	if (argc > 3) {
		outputDir = string(argv[3]);
		if (outputDir.at(outputDir.length()-1) != '/') { outputDir.append("/");	}
	} else {
		// by default, put the output where the input RESULT file is
		outputDir = resultFile.substr(0, resultFile.find_last_of('/')+1);
	}
	cout << "Output to " << outputDir << endl;


	procMode = argv[1];
	if (!(procMode == "std" || procMode == "fair" || procMode == "job")) {
		cout << "BAD procMode" << endl;
		exit(EXIT_FAILURE);
	}

	// parsing went well
	return true;

}

bool generateResultFileList(int it, string file) {
	// populate resultFileList with it elements
	resultFileList = new string [iter];
	if (iter > 0) {
		// precompute all variation for the input result file coming from argv[1]
		for (int i = 0; i < iter; i++) {
			resultFileList[i] = string(file).substr(0, string(file).length()-6);
			resultFileList[i].append(to_string(i+1)+".data");	// start from _1
			cout << resultFileList[i] << endl;
		}
	}

	// everything went good
	return true;
}

bool doStdProc() {

	// read flows from file
	if (iter > 0) {
		for (int i = 0; i < iter; i++) {
			readMeasFlows(resultFileList[i]);
		}
	} else {
		readMeasFlows(resultFile);
	}
	cout << "I read " << flowsMeasured.size() << " flows" << endl;
	cout << " " << shortFlowsMeasured.size() << " short flows" << endl;
	cout << " " << largeFlowsMeasured.size() << " large flows" << endl;


	// do processing with flows not categorised
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



	doSizeCat = true;

	if (doSizeCat) {
			// categorise flows in flow size categories
			cout << "Categorising flows by size..." << endl;

			getSizeCategories();	// select size categories
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

				if ((int)flowsMeasuredCat[i].size() > 0) {
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

	}



	int cdfPoints = 100;


	// write CDF of FCT for short flows
	// do one file with ALL points, and another with cdfPoints points
	if ((int)shortFlowsMeasured.size() > 0) {
		sort(shortFlowsMeasured.begin(), shortFlowsMeasured.end(), compareByFCT);
		dumpShortCDF_FCT(string(outputDir + "CDF_FCT_SHORT.dat"));
		dumpShortCDF_FCT(string(outputDir + "CDF_FCT_SHORT.dat"), cdfPoints);
	}


	// write CDF of throughput for long flows
	// do one file with ALL points, and another with cdfPoints points
	if ((int)largeFlowsMeasured.size() > 0) {
		sort(largeFlowsMeasured.begin(), largeFlowsMeasured.end(), compareByThr);
		dumpLargeCDF_thr(string(outputDir + "CDF_thr_LARGE.dat"));
		dumpLargeCDF_thr(string(outputDir + "CDF_thr_LARGE.dat"), cdfPoints);
	}





/*
	uint64_t sum = 0;
	for (int i = 0; i < (int)flowsMeasuredCat[0].size(); i++) {
		sum += flowsMeasuredCat[0].at(i).fct;

		cout << flowsMeasuredCat[0].at(i).size;
		cout << "\t" << flowsMeasuredCat[0].at(i).thr;
		cout << "\n";

	}
	cout << "sum: " << sum;
	cout << "\tavg: " << ((double)sum/(int)flowsMeasuredCat[0].size())/1000000.0;
	cout << endl;
*/

	return true;

}

bool doFairProc() {
	// categorise flows by their TCP algorithm

	if (iter > 0) {
		for (int i = 0; i < iter; i++) {
			readMeasFlows(resultFileList[i]);
		}
	} else {
		readMeasFlows(resultFile);
	}
	cout << "I read " << tcpFlowTypeList[0].size() << " TCP flows" << endl;
	cout << "\t" << tcpFlowTypeList[1].size() << " DCTCP flows" << endl;
	cout << "\t" << tcpFlowTypeList[2].size() << " DCMPTCP flows" << endl;
	cout << "\tUnknown flow types: " << unknownFlow << endl;

	int points = 100;	// number of points to put in the CDF


	// TCP flows
	if ((int)tcpFlowTypeList[0].size() > 0) {
		sort(tcpFlowTypeList[0].begin(), tcpFlowTypeList[0].end(), compareByThr);
		dumpFairCDF_thr(string(outputDir + "fair_CDF_thr.dat"), 0);
		dumpFairCDF_thr(string(outputDir + "fair_CDF_thr.dat"), 0, points);
	}

	// DCTCP flows
	if ((int)tcpFlowTypeList[1].size() > 0) {
		sort(tcpFlowTypeList[1].begin(), tcpFlowTypeList[1].end(), compareByThr);
		dumpFairCDF_thr(string(outputDir + "fair_CDF_thr.dat"), 1);
		dumpFairCDF_thr(string(outputDir + "fair_CDF_thr.dat"), 1, points);
	}


	// DCMPTCP flows
	if ((int)tcpFlowTypeList[2].size() > 0) {
		sort(tcpFlowTypeList[2].begin(), tcpFlowTypeList[2].end(), compareByThr);
		dumpFairCDF_thr(string(outputDir + "fair_CDF_thr.dat"), 2);
		dumpFairCDF_thr(string(outputDir + "fair_CDF_thr.dat"), 2, points);
	}



	return true;

}

bool doJobProc() {
	// process job results

	// read flows from file
	if (iter > 0) {
		for (int i = 0; i < iter; i++) {
			readMeasJobs(resultFileList[i]);
		}
	} else {
		readMeasJobs(resultFile);
	}
	cout << "I read " << jobsMeasured.size() << " jobs" << endl;


	// output JCT stats
	cout << "Outputting JCT stats...";
	int points = 500;	// number of points to put in the CDF

	if ((int)jobsMeasured.size() > 0) {
		sort(jobsMeasured.begin(), jobsMeasured.end(), compareByJCT);
		dumpCDF_JCT(string(outputDir + "job_CDF_JCT.dat"));
		dumpCDF_JCT(string(outputDir + "job_CDF_JCT.dat"), points);
	}


	cout << " DONE" << endl;

	return true;

}







bool getSizeCategories() {
	// size categories could be read from somewhere
	// do it statically for now
	sizeCategory.push_back(128*1024);		// 128KB
	sizeCategory.push_back(512*1024);		// 512KB
	sizeCategory.push_back(1*1024*1024);	// 1MB
	sizeCategory.push_back(10*1024*1024);	// 10MB
	cout << "I have " << sizeCategory.size() << " flow size categories: ";
	for (int i = 0; i < (int)sizeCategory.size(); i++) {
		cout << sizeCategory.at(i) << "\t";
	}
	cout << "\n";


	// allocate memory for flowsMeasuredCat
	flowsMeasuredCat = new vector<flowMeas> [(int)sizeCategory.size()+1];

	return true;

}



bool readMeasFlows(string file) {
	// read line by line, extract flow stats and store them in vector

	ifstream read(file.c_str());
	if (read.is_open()) {
		string line;

		while(getline(read, line)) {
			// flow size is between [<...>], it is in bytes
			// throughput is between [#...#], it is in Mbps
			// FCT is between [*...*], it is in seconds, asjust it in nanoseconds
			// also categorise flows by "Short" or "Large", between [=...=]

			if (line.at(0) == '[') {
				flowMeas temp;
				int pos1 = 0;
				int pos2 = 0;

				// get flow size,
				pos1 = line.find("[<");
				pos2 = line.find(">]", pos1);
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


				// check whether the flow is short or large
				pos1 = line.find("[=");
				pos2 = line.find("=]", pos1);
				string t = line.substr(pos1+2, pos2-pos1-2);

				if (t == "Large") {
					largeFlowsMeasured.push_back(temp);
				} else if (t == "Short") {
					shortFlowsMeasured.push_back(temp);
				}



				// read TCP algorithm if needed
				if (procMode == "fair") {
					pos1 = line.find("[|");
					pos2 = line.find("|]", pos1);
					string a = line.substr(pos1+2, pos2-pos1-2);

					if (a == "TCP") {
						tcpFlowTypeList[0].push_back(temp);
					} else if (a == "DCTCP") {
						tcpFlowTypeList[1].push_back(temp);
					} else if (a == "DCMPTCP") {
						tcpFlowTypeList[2].push_back(temp);
					} else {
						unknownFlow++;
					}
				}

			}
		}

	} else {
		cout << "Could not open file" << endl;
		return false;
	}

	return true;

}

bool readMeasJobs(string file) {
	// read line by line, extract job stats and store them in vector

	ifstream read(file.c_str());
	if (read.is_open()) {
		string line;

		while(getline(read, line)) {
			// JCT is between [-...-], it is in milliseconds, adjust it in nanoseconds

			if (line.find("[-") != string::npos) {
				jobMeas temp;
				int pos1 = 0;
				int pos2 = 0;


				// get JCT
				pos1 = line.find("[-");
				pos2 = line.find("-]", pos1);
//				cout << "pos1: " << pos1 << "\tpos2: " << pos2 << endl;
//				cout << line.substr(pos1+2, pos2-pos1-2) << endl;
				temp.jct = (uint64_t)1000000.0*strtod(line.substr(pos1+2, pos2-pos1-2).c_str(), NULL);
//				cout << "jct: " << temp.jct << endl;

				jobsMeasured.push_back(temp);

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
		uint64_t iS;
		for (iS = 0; iS < (uint64_t)sizeCategory.size(); iS++) {
			if (flowsMeasured.at(i).size <= (uint64_t)sizeCategory.at(iS)) {
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

bool dumpShortCDF_FCT(string file) {
	// write CDF of FCT for short flows to file
	ofstream write(file.c_str());
	if (write.is_open()) {
		// write header
		write << "#CDF\tFCT(ns)" << "\n";
		for (int i = 0; i < (int)shortFlowsMeasured.size(); i++) {
			write << (double)(i+1)/(double)shortFlowsMeasured.size();
			write << "\t" << shortFlowsMeasured.at(i).fct;
			write << "\n";
		}
		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpShortCDF_FCT(...)"<< endl;
		return false;
	}
}

bool dumpShortCDF_FCT(string file, int cdfPoints) {
	// write CDF of FCT for short flows to file
	// write only cdfPoints points for the CDF
	// write a point every interval
	file = string(file.substr(0, file.length()-4) + "_" + to_string(cdfPoints) + "p.dat");

	ofstream write(file.c_str());
	if (write.is_open()) {
		double interval = 1/(double)cdfPoints;
		double inc = interval;

		// write header
		write << "#CDF\tFCT(ns)" << "\n";
		for (int i = 0; i < (int)shortFlowsMeasured.size(); i++) {
			double currCdf = (double)(i+1)/(double)shortFlowsMeasured.size();
			if (inc < currCdf) {
				write << inc;
				write << "\t" << shortFlowsMeasured.at(i).fct;
				write << "\n";
				inc += interval;
			}
		}

		// put the final one for cdf value 1
		write << 1.0;
		write << "\t" << shortFlowsMeasured.at(shortFlowsMeasured.size()-1).fct;
		write << "\n";
		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpShortCDF_FCT(..., ...)"<< endl;
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

bool dumpLargeCDF_thr(string file) {
	// write CDF of avg throughput for large flows to file
	ofstream write(file.c_str());
	if (write.is_open()) {
		// write header
		write << "#CDF\tavg_thr(Mbps)" << "\n";
		for (int i = 0; i < (int)largeFlowsMeasured.size(); i++) {
			write << (double)(i+1)/(double)largeFlowsMeasured.size();
			write << "\t" << largeFlowsMeasured.at(i).thr;
			write << "\n";
		}
		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpLargeCDF_thr(...)"<< endl;
		return false;
	}
}

bool dumpLargeCDF_thr(string file, int cdfPoints) {
	// write CDF of avg throughput for large flows to file
	// write only cdfPoints points for the CDF
	// write a point every interval
	file = string(file.substr(0, file.length()-4) + "_" + to_string(cdfPoints) + "p.dat");

	ofstream write(file.c_str());
	if (write.is_open()) {
		double interval = 1/(double)cdfPoints;
		double inc = interval;

		// write header
		write << "#CDF\tavg_thr(Mbps)" << "\n";
		for (int i = 0; i < (int)largeFlowsMeasured.size(); i++) {
			double currCdf = (double)(i+1)/(double)largeFlowsMeasured.size();
			if (inc < currCdf) {
				write << inc;
				write << "\t" << largeFlowsMeasured.at(i).thr;
				write << "\n";
				inc += interval;
			}
		}

		// put the final one for cdf value 1
		write << 1.0;
		write << "\t" << largeFlowsMeasured.at(largeFlowsMeasured.size()-1).thr;
		write << "\n";
		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpLargeCDF_thr(..., ...)"<< endl;
		return false;
	}
}

bool dumpFairCDF_thr(string file, int flowType) {
	// write CDF of avg throughput to file
	file = string(file.substr(0, file.length()-4) + "_" + tcpFlowTypes[flowType] + ".dat");

	ofstream write(file.c_str());
	if (write.is_open()) {
		// write header
		write << "#CDF\tavg_thr(Mbps)" << "\n";
		for (int i = 0; i < (int)tcpFlowTypeList[flowType].size(); i++) {
			write << (double)(i+1)/(double)tcpFlowTypeList[flowType].size();
			write << "\t" << tcpFlowTypeList[flowType].at(i).thr;
			write << "\n";
		}
		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpFairCDF_thr(...)"<< endl;
		return false;
	}
}

bool dumpFairCDF_thr(string file, int flowType, int cdfPoints) {
	// write CDF of avg throughput to file
	// write only cdfPoints points for the CDF
	// write a point every interval
	file = string(file.substr(0, file.length()-4) + "_" + tcpFlowTypes[flowType] + "_" + to_string(cdfPoints) + "p.dat");

	ofstream write(file.c_str());
	if (write.is_open()) {
		double interval = 1/(double)cdfPoints;
		double inc = interval;

		// write header
		write << "#CDF\tavg_thr(Mbps)" << "\n";
		for (int i = 0; i < (int)tcpFlowTypeList[flowType].size(); i++) {
			double currCdf = (double)(i+1)/(double)tcpFlowTypeList[flowType].size();
			if (inc < currCdf) {
				write << inc;
				write << "\t" << tcpFlowTypeList[flowType].at(i).thr;
				write << "\n";
				inc += interval;
			}
		}

		// put the final one for cdf value 1
		write << 1.0;
		write << "\t" << tcpFlowTypeList[flowType].at(tcpFlowTypeList[flowType].size()-1).thr;
		write << "\n";
		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpLargeCDF_thr(..., ...)"<< endl;
		return false;
	}


}

bool dumpCDF_JCT(string file) {
	// write CDF of FCT for short flows to file
	ofstream write(file.c_str());
	if (write.is_open()) {
		// write header
		write << "#CDF\tJCT(ns)" << "\n";
		for (int i = 0; i < (int)jobsMeasured.size(); i++) {
			write << (double)(i+1)/(double)jobsMeasured.size();
			write << "\t" << jobsMeasured.at(i).jct;
			write << "\n";
		}
		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpCDF_JCT(...)"<< endl;
		return false;
	}
}

bool dumpCDF_JCT(string file, int cdfPoints) {
	// write CDF of FCT for short flows to file
	// write only cdfPoints points for the CDF
	// write a point every interval
	file = string(file.substr(0, file.length()-4) + "_" + to_string(cdfPoints) + "p.dat");

	ofstream write(file.c_str());
	if (write.is_open()) {
		double interval = 1/(double)cdfPoints;
		double inc = interval;

		// write header
		write << "#CDF\tFCT(ns)" << "\n";
		for (int i = 0; i < (int)jobsMeasured.size(); i++) {
			double currCdf = (double)(i+1)/(double)jobsMeasured.size();
			if (inc < currCdf) {
				write << inc;
				write << "\t" << jobsMeasured.at(i).jct;
				write << "\n";
				inc += interval;
			}
		}

		// put the final one for cdf value 1
		write << 1.0;
		write << "\t" << jobsMeasured.at(jobsMeasured.size()-1).jct;
		write << "\n";
		write.close();
		return true;
	} else {
		cout << "Could not open file for dumpCDF_JCT(..., ...)"<< endl;
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
			if ((int)flowsMeasuredCat[i].size() == 0 && index == 0) {
				write << 0.0 << "\t";
			} else {
				write << flowsMeasuredCat[i].at(index).thr << "\t";
			}
		}
		write << "\n";

		// write 95%
		write << "95%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.95;
			if ((int)flowsMeasuredCat[i].size() == 0 && index == 0) {
				write << 0.0 << "\t";
			} else {
				write << flowsMeasuredCat[i].at(index).thr << "\t";
			}
		}
		write << "\n";

		// write 99%
		write << "99%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.99;
			if ((int)flowsMeasuredCat[i].size() == 0 && index == 0) {
				write << 0.0 << "\t";
			} else {
				write << flowsMeasuredCat[i].at(index).thr << "\t";
			}
		}
		write << "\n";

		// write 99.9%
		write << "99.9%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.999;
			if ((int)flowsMeasuredCat[i].size() == 0 && index == 0) {
				write << 0.0 << "\t";
			} else {
				write << flowsMeasuredCat[i].at(index).thr << "\t";
			}
		}
		write << "\n";

		// write 99.99%
		write << "99.99%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.9999;
			if ((int)flowsMeasuredCat[i].size() == 0 && index == 0) {
				write << 0.0 << "\t";
			} else {
				write << flowsMeasuredCat[i].at(index).thr << "\t";
			}
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
			if ((int)flowsMeasuredCat[i].size() == 0 && index == 0) {
				write << 0.0 << "\t";
			} else {
				write << flowsMeasuredCat[i].at(index).fct/1000000.0 << "\t";
			}
		}
		write << "\n";

		// write 95%
		write << "95%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.95;
			if ((int)flowsMeasuredCat[i].size() == 0 && index == 0) {
				write << 0.0 << "\t";
			} else {
				write << flowsMeasuredCat[i].at(index).fct/1000000.0 << "\t";
			}
		}
		write << "\n";

		// write 99%
		write << "99%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.99;
			if ((int)flowsMeasuredCat[i].size() == 0 && index == 0) {
				write << 0.0 << "\t";
			} else {
				write << flowsMeasuredCat[i].at(index).fct/1000000.0 << "\t";
			}
		}
		write << "\n";

		// write 99.9%
		write << "99.9%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.999;
			if ((int)flowsMeasuredCat[i].size() == 0 && index == 0) {
				write << 0.0 << "\t";
			} else {
				write << flowsMeasuredCat[i].at(index).fct/1000000.0 << "\t";
			}
		}
		write << "\n";

		// write 99.99%
		write << "99.99%\t";
		for (int i = 0; i < (int)sizeCategory.size()+1; i++) {
			int index = (int)flowsMeasuredCat[i].size()*0.9999;
			if ((int)flowsMeasuredCat[i].size() == 0 && index == 0) {
				write << 0.0 << "\t";
			} else {
				write << flowsMeasuredCat[i].at(index).fct/1000000.0 << "\t";
			}
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

bool compareByJCT(const jobMeas &a, const jobMeas &b) {
	return a.jct < b.jct;
}

bool compareByFlowSize(const flowMeas &a, const flowMeas &b) {
	return a.size < b.size;
}

bool compareByThr(const flowMeas &a, const flowMeas &b) {
	return a.thr < b.thr;
}








