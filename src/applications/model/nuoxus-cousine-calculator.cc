#include "nuoxus-cousine-calculator.h"

namespace ns3{

NuoxusCousineCalculator::NuoxusCousineCalculator()
{
}


NuoxusCousineCalculator::~NuoxusCousineCalculator()
{
}


double_t NuoxusCousineCalculator::CousineBetweenNodes(NuoxusNodeAccessHistory &n1, NuoxusNodeAccessHistory &n2)
{
    binary_vector_t vectorN1;
    binary_vector_t vectorN2;
    if (n1.Size() > n2.Size())
    {        
        vectorN1 = BuildUnaryVector(n1.Size());
        vectorN2 = BuildComparisonBinaryVector(n1, n2);
    }
    else
    {
        vectorN1 = BuildUnaryVector(n2.Size());
        vectorN2 = BuildComparisonBinaryVector(n2, n1);
    }
    // Calculate the similarity
    return Cousine(vectorN1, vectorN2);
}

double_t NuoxusCousineCalculator::Cousine(binary_vector_t A, binary_vector_t B)
{
    double_t dotProduct = CalculateDotProduct(A, B);
    double_t magnitudeOfA = CalculateMagnitude(A);
    double_t magnitudeOfB = CalculateMagnitude(B);

    if (dotProduct == 0)
        return dotProduct;
    else
        return dotProduct / (magnitudeOfA * magnitudeOfB);
}

binary_vector_t NuoxusCousineCalculator::BuildComparisonBinaryVector(NuoxusNodeAccessHistory &base, NuoxusNodeAccessHistory &compare)
{
    binary_vector_t v;    
    node_access_history_t baseHistory = base.GetHistory();
    node_access_history_t compareHistory = compare.GetHistory();
    for (node_access_history_t::iterator it = baseHistory.begin(); it != baseHistory.end(); ++it)
    {   
        uint32_t value = 0;
        if(compareHistory[it->first] > 0) value = 1;
        v.push_back(value);
    }
    if (v.size() == 0)
    {
        v.push_back(1);
    }
    return v;
}

binary_vector_t NuoxusCousineCalculator::BuildUnaryVector(uint32_t size)
{
    binary_vector_t v;
    for (size_t i = 0; i < size; i++) v.push_back(1);
    if (v.size() == 0) v.push_back(1);
    return v;
}

double_t NuoxusCousineCalculator::CalculateDotProduct(binary_vector_t& A, binary_vector_t& B)
{
    double_t dotProduct = 0;
    for (size_t i = 0; i < A.size(); i++)
    {
        dotProduct += (A[i] * B[i]);
    }
    return dotProduct; 
}

double_t NuoxusCousineCalculator::CalculateMagnitude(binary_vector_t &A)
{
    return sqrt(CalculateDotProduct(A, A));
}

}
