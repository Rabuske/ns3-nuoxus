#ifndef NUOXUS_COUSINE_H
#define NUOXUS_COUSINE_H

#include "ns3/nuoxus-history-manager.h"
#include "ns3/ipv4-address.h"
#include <map>
#include <string>
#include <vector>
#include <cmath>

namespace ns3
{

typedef std::vector<uint32_t> binary_vector_t;

class NuoxusCousineCalculator
{
private:
    double_t Cousine(binary_vector_t A, binary_vector_t B);
    binary_vector_t BuildComparisonBinaryVector(NuoxusNodeAccessHistory &base, NuoxusNodeAccessHistory &compare);
    binary_vector_t BuildUnaryVector(uint32_t size);

    binary_vector_t HistoryToBinaryVector(NuoxusNodeAccessHistory &history);
    double_t CalculateDotProduct(binary_vector_t &A, binary_vector_t &B);
    double_t CalculateMagnitude(binary_vector_t &A);

public:
    NuoxusCousineCalculator();
    ~NuoxusCousineCalculator();

    double_t CousineBetweenNodes(NuoxusNodeAccessHistory &n1, NuoxusNodeAccessHistory &n2);
};

}
#endif
