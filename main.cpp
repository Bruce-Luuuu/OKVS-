#include <cryptoTools/Network/IOService.h>
#include "SimpleIndex.h"
#include <iomanip>
#include <libOTe/Tools/LDPC/Util.h>
#include <libOTe_Tests/Common.h>
#include "Paxos.h"
#include "PaxosImpl.h"

#include <libdivide.h>
using namespace oc;
using namespace volePSI;
using namespace osuCrypto;
using namespace std;

void perfBaxos(oc::CLP& cmd)
{
	auto n = cmd.getOr("n", 1ull << cmd.getOr("nn", 14));	// -n <value>: The set size.  -nn <value>: the log2 size of the sets.
	auto t = cmd.getOr("t", 1ull);	 						// -t <value>: the number of trials.
	//auto rand = cmd.isSet("rand");
	auto v = cmd.getOr("v", cmd.isSet("v") ? 1 : 0);		// -v: verbose.
	auto w = cmd.getOr("w", 3);								// -w <value>: The okvs weight.          I need to change this parameter!
	auto ssp = cmd.getOr("ssp", 40);	 					// -ssp <value>: statistical security parameter.
	auto dt = cmd.isSet("binary") ? PaxosParam::Binary : PaxosParam::GF128;
	auto nt = cmd.getOr("nt", 0);	 						// -nt: number of threads.

	//PaxosParam pp(n, w, ssp, dt);
	auto binSize = 1 << cmd.getOr("lbs", 15);	 			// -lbs <value>: the log2 bin size.
	u64 baxosSize;
	{
		Baxos paxos;
		paxos.init(n, binSize, w, ssp, dt, oc::ZeroBlock);
		baxosSize = paxos.size();	// in Paxos.h line 530
	}
	std::vector<block> key(n), val(n), pax(baxosSize);
	PRNG prng(ZeroBlock);	// generate random key-value pair.
	prng.get<block>(key);
	prng.get<block>(val);

	Timer timer;
	auto start = timer.setTimePoint("start");
	auto end = start;
	for (u64 i = 0; i < t; ++i)
	{
		Baxos paxos;
		paxos.init(n, binSize, w, ssp, dt, block(i, i));

		//if (v > 1)
		//	paxos.setTimer(timer);

		paxos.solve<block>(key, val, pax, nullptr, nt);
		timer.setTimePoint("s" + std::to_string(i));

		paxos.decode<block>(key, val, pax, nt);

		end = timer.setTimePoint("d" + std::to_string(i));
	}

	if (v)
		std::cout << timer << std::endl;

	auto tt = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / double(1000);
	std::cout << "total " << tt << "ms, e=" << double(baxosSize) / n << std::endl;
}



template<typename T>
void perfPaxosImpl(oc::CLP& cmd)
{
	auto n = cmd.getOr("n", 1ull << cmd.getOr("nn", 14));
	u64 maxN = std::numeric_limits<T>::max() - 1;
	auto t = cmd.getOr("t", 1ull);
	//auto rand = cmd.isSet("rand");
	auto v = cmd.getOr("v", cmd.isSet("v") ? 1 : 0);	// if want to see more info, set v=2
	auto w = cmd.getOr("w", 3);
	auto ssp = cmd.getOr("ssp", 40);
	auto dt = cmd.isSet("binary") ? PaxosParam::Binary : PaxosParam::GF128;
	auto cols = cmd.getOr("cols", 0);    // The size of the okvs elements in multiples of 16 bytes. default = 1.

	PaxosParam pp(n, w, ssp, dt);

	if (maxN < pp.size())
	{
		std::cout << "n must be smaller than the index type max value. " LOCATION << std::endl;
		throw RTE_LOC;
	}

	auto m = cols ? cols : 1;
	std::vector<block> key(n);
	oc::Matrix<block> val(n, m), pax(pp.size(), m);
	PRNG prng(ZeroBlock);
	prng.get<block>(key);
	prng.get<block>(val);

	Timer timer;
	auto start = timer.setTimePoint("start");
	auto end = start;
	for (u64 i = 0; i < t; ++i)
	{
		Paxos<T> paxos;
		paxos.init(n, pp, block(i, i));

		if (v > 1)
			paxos.setTimer(timer);

		if (cols)
		{
			paxos.setInput(key);
			paxos.template encode<block>(val, pax);
			timer.setTimePoint("s" + std::to_string(i));
			paxos.template decode<block>(key, val, pax);
		}
		else
		{
			paxos.template solve<block>(key, oc::span<block>(val), oc::span<block>(pax));
			timer.setTimePoint("s" + std::to_string(i));
			paxos.template decode<block>(key, oc::span<block>(val), oc::span<block>(pax));
		}

		end = timer.setTimePoint("d" + std::to_string(i));
	}

	if (v)
		std::cout << timer << std::endl;

	auto tt = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / double(1000);
	std::cout << "total " << tt << "ms, e="<< pp.size() / double(n) << std::endl;
}


template<typename T>
void mixPaxosImpl(oc::CLP& cmd){
	auto n = cmd.getOr("n", 1ull << cmd.getOr("nn", 14));
	auto t = cmd.getOr("t", 1ull);
	auto ssp = 40;						// Due to time constraint, I let ssp equals 40.
	auto dt = PaxosParam::GF128;  		// Due to time constraint, I only implement GF128 mode.
	auto v = cmd.getOr("v", cmd.isSet("v") ? 1 : 0);  // if want to see more info, set v=2
	
    PaxosParam pp(n, 3, 15, ssp, dt);

	u64 maxN = std::numeric_limits<T>::max() - 1;
	if (maxN < pp.size())
	{
		std::cout << "n must be smaller than the index type max value. " LOCATION << std::endl;
		throw RTE_LOC;
	}

	std::vector<block> key(n), val(n), pax(pp.size());
	PRNG prng(ZeroBlock);
	prng.get<block>(key);
	prng.get<block>(val);

	Timer timer;
	auto start = timer.setTimePoint("start");
	auto end = start;

	for (u64 i = 0; i < t; ++i)
	{
		Paxos<T> paxos;
		paxos.initMix(n, pp, block(i, i));

		if (v > 1)
			paxos.setTimer(timer);

		paxos.setInputMix(key);
		paxos.template encodeMix<block>(oc::span<block>(val), oc::span<block>(pax));

		timer.setTimePoint("s" + std::to_string(i));

		paxos.template decodeMix<block>(key, oc::span<block>(val), oc::span<block>(pax));

		end = timer.setTimePoint("d" + std::to_string(i));
	}

	if (v)
		std::cout << timer << std::endl;

	auto tt = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / double(1000);
	std::cout << "total " << tt << "ms, e="<< pp.size() / double(n) << std::endl;
}


int main(int argc, char** argv){
    CLP cmd;
    cmd.parse(argc, argv);
	bool choice = cmd.getOr("c", 0);  // determine which implementation to run.

	if (choice == 0)
	{
		mixPaxosImpl<u32>(cmd);
	}
	else if (choice == 1)
	{
		perfPaxosImpl<u32>(cmd);
	}
	else
	{
		perfBaxos(cmd);
	}
	
    return 0;
}