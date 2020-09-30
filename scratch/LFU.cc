#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/csma-helper.h"
#include <map>
#include <list>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FogCaching");


void NuoxusRequestTraceCallback(Ptr<OutputStreamWrapper> outputStream, NuoxusTracedRequest oldValue, NuoxusTracedRequest newValue)
{    
    *outputStream->GetStream() << newValue.GetTime().GetSeconds() << "\t" << 
                                  newValue.GetNodeId()    << "\t" << 
                                  newValue.GetRequester() << "\t" << 
                                  newValue.GetContentId() << "\t" << 
                                  newValue.GetWasCached() << "\t" << 
                                  newValue.GetIsCached() << "\t" << std::endl;
}

void ClientRequestTraceCallback(Ptr<OutputStreamWrapper> outputStream, VideoClientRequestInfo oldValue, VideoClientRequestInfo newValue)
{    
    *outputStream->GetStream() << newValue.GetStartTime().GetSeconds() << "\t" << 
                                  newValue.GetEndTime().GetSeconds()   << "\t" << 
                                  newValue.GetRequester() << "\t" << 
                                  newValue.GetVideoId() << "\t" << 
                                  newValue.GetAverageDelay() << "\t"<< std::endl;
}

void FileReplacementCallback(Ptr<OutputStreamWrapper> outputStream, NuoxusContentReplacementTrace oldValue, NuoxusContentReplacementTrace newValue)
{    
    *outputStream->GetStream() << newValue.GetTimeStamp().GetSeconds() << "\t";
    score_t replacingContent = newValue.GetReplacing();
    score_t replacedContent  = newValue.GetReplaced();

    for(score_t::iterator it = replacingContent.begin(); it !=  replacingContent.end(); ++it)
    {
        *outputStream->GetStream() << it->first << "\t" << it->second << "\t";
    }

    for(score_t::iterator it = replacedContent.begin(); it !=  replacedContent.end(); ++it)
    {
        *outputStream->GetStream() << it->first << "\t" << it->second << "\t";
    }
}

int main (int argc, char *argv[])
{
    uint16_t numberOfFaps       = 5;
    uint16_t numberOfFues       = 120;
    double   simTime            = 86400.00; // 24 hours = 86400.00;
    uint32_t videoLibrarySize   = 10000.00;
    bool     withProactiveCache = true;
    uint64_t diskSizeKb         = 80000000; //10 gb
    TypeId   cachingStrategy    = NuoxusCachingStrategyLFU::GetTypeId();
   
    std::map<uint32_t, uint32_t> listOfFueConnectedToFaps;
    std::map<uint32_t, Ptr<OutputStreamWrapper>> listOfFueOutputStreamsNuoxusRequest;
    std::map<uint32_t, Ptr<OutputStreamWrapper>> listOfFueOutputStreamsClientRequest; 
    Ptr<OutputStreamWrapper> outputForReplacement;
    std::map<uint32_t, Ptr<Nuoxus>> listOfNuoxus;

    // Command Line parameters
    CommandLine cmd;
    cmd.AddValue("numberOfFaps", "Number of F-APs", numberOfFaps);
    cmd.AddValue("numberOfFues", "Number of F-UEs", numberOfFues);
    cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
    cmd.AddValue("diskSizeKb", "Disk size in Kilobit", diskSizeKb);
    // These parameters could have been set using the Attribute system of NS3, but they are being done via command line to make it more understandable
    cmd.AddValue("videoLibrarySize", "Number of videos in library", videoLibrarySize);
    cmd.AddValue("useProactiveCaching", "Use proactive caching", withProactiveCache);
    cmd.AddValue("cachingStrategy", "Type of caching strategy", cachingStrategy);

    cmd.Parse (argc, argv);
    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults ();

    // Parse the values again, in order to keep the override of the default values
    cmd.Parse (argc, argv);

    // Print the parameters, so the user know what is about to be used
    std::cout << "Number of F-APs: " << numberOfFaps << std::endl;
    std::cout << "Number of F-UEs: " << numberOfFues << std::endl;
    std::cout << "Total duration of the simulation [s]: " << simTime << std::endl;
    std::cout << "Disk size in Kilobit" << diskSizeKb  << std::endl;
    std::cout << "Number of videos in library: " << videoLibrarySize << std::endl;
    std::cout << "Use proactive caching: " << withProactiveCache << std::endl;
    std::cout << "Type of caching strategy: " << cachingStrategy << std::endl;
    std::cout.flush();
    
    // Containers for the network Nodes
    NodeContainer videoHostNodes;
    NodeContainer fapNodes;
    NodeContainer fueNodes;

    // Create the Video Host in ID 0

    // Create each F-AP node assigned to a different MPI processors
    for(uint32_t i_fap_mpi = 0; i_fap_mpi < numberOfFaps; i_fap_mpi++)
    {
        videoHostNodes.Add(CreateObject<Node>());
        fapNodes.Add(CreateObject<Node>());
    }

    // This object will help to create the connections between the F-APs and the Remote video server located in the core network
    PointToPointHelper p2pHelperFapVideoHost;
    p2pHelperFapVideoHost.SetDeviceAttribute("DataRate", StringValue ("15Gbps"));
    p2pHelperFapVideoHost.SetChannelAttribute("Delay", StringValue ("20ms"));
    
    // Install the p2p devices to connect the F-APS to the Video Host
    NetDeviceContainer p2pFapVideoHostDevices;
   
    for(uint32_t i_fap = 0; i_fap < numberOfFaps; i_fap++)
    {
        // p2pFapVideoHostDevices(n) constains the host p2pFapVideoHostDevices(n+1) contains the F-AP
         //NS_LOG_INFO("Connecting F-AP " << i_fap << " to Video Host");
         p2pFapVideoHostDevices.Add(p2pHelperFapVideoHost.Install(videoHostNodes.Get(i_fap), fapNodes.Get(i_fap)));        
    }
       
    // This object will help to create the connections between the F-UEs and the F-APs
    PointToPointHelper p2pHelperFueFap;
    p2pHelperFueFap.SetDeviceAttribute("DataRate", StringValue ("1Gbps"));
    p2pHelperFueFap.SetChannelAttribute("Delay", StringValue ("200ms"));
    
    // Install the p2p devices to connect the F-UEs to a randomly determined F-APs
    Ptr<UniformRandomVariable> randomFapGenerator = CreateObject<UniformRandomVariable> ();
    randomFapGenerator->SetAttribute ("Min", DoubleValue (0));
    randomFapGenerator->SetAttribute ("Max", DoubleValue (numberOfFaps));    
    NetDeviceContainer p2pFueFapDevices;
    for(uint32_t i_fue = 0; i_fue < numberOfFues; i_fue++) 
    {

        // p2pFueFapDevices(n) constains an F-AP p2pFueFapDevices(n+1) contains an F-UE 
        uint32_t randomFapNumber = (uint32_t) randomFapGenerator->GetValue();
        
        // Create the F-UE node, assigning them to each MPI processor
        fueNodes.Add(CreateObject<Node>());
       
        //NS_LOG_INFO("Connecting F-UE " << i_fue << " to F-AP " << randomFapNumber);
        p2pFueFapDevices.Add(p2pHelperFueFap.Install(fapNodes.Get(randomFapNumber), fueNodes.Get(i_fue))); 
       
        // Save their connection into a list, so it can be accessed when installing the applications
        listOfFueConnectedToFaps[i_fue] = randomFapNumber;        
    }    
    
    // Install the Internet stack in the nodes
    InternetStackHelper internetStackHelper;
    internetStackHelper.Install(videoHostNodes);
    internetStackHelper.Install(fapNodes);
    internetStackHelper.Install(fueNodes);
    
    // Assign the IP addresses for the F-AP/Video Host Network
    Ipv4AddressHelper p2pFapVideoHostAddress;
    p2pFapVideoHostAddress.SetBase ("10.1.1.0", "255.255.255.255");    
    Ipv4InterfaceContainer p2pFapVideoHostInterfaces = p2pFapVideoHostAddress.Assign(p2pFapVideoHostDevices);
    
    // Assign the IP addresses for the F-UE/F-AP Network
    Ipv4AddressHelper p2pFueFapAddress;
    p2pFueFapAddress.SetBase ("7.1.0.0", "255.255.255.255");    
    Ipv4InterfaceContainer p2pFueFapInterface = p2pFueFapAddress.Assign(p2pFueFapDevices);    
    
    // Populate the routing tables of the devices
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // Install the applications
    uint16_t portVideoRequestCachedServer = 2000;
    uint16_t portVideoRequestVideoHost    = 3000;
    uint16_t portVideoResponseClient      = 4000;
    uint32_t packetSize = 512;

    ApplicationContainer videoHostApps;    
    ApplicationContainer fapApps;
    ApplicationContainer fueApps;    
  
    Ptr<ZipfRandomVariable> zipf = CreateObject<ZipfRandomVariable>();
    zipf->SetAttribute("N", IntegerValue(videoLibrarySize));
    zipf->SetAttribute("Alpha", DoubleValue(0.7));

    Ptr<VideoLibrary> videoLibrary = CreateObject<VideoLibrary>();
    videoLibrary->Initialize(videoLibrarySize, 264, 180);

    // Install tracing in Nuoxus preemptivelly 
    AsciiTraceHelper asciiTraceHelper;                
    outputForReplacement = asciiTraceHelper.CreateFileStream("nuoxus-replacement.txt");
    for(uint32_t i_fap = 0; i_fap < numberOfFaps; i_fap++) 
    {        
        // Create one tracing file for each F-AP
        std::stringstream fileNameNuoxusRequest; 
        fileNameNuoxusRequest << "nuoxus-request-" << i_fap << ".txt";
        listOfFueOutputStreamsNuoxusRequest[i_fap] = asciiTraceHelper.CreateFileStream (fileNameNuoxusRequest.str());        

        InetSocketAddress nuoxusAddress = InetSocketAddress(p2pFapVideoHostInterfaces.GetAddress((i_fap * 2) + 1));
        Ptr<Nuoxus> nuoxus =  NuoxusFactory::Create(packetSize, cachingStrategy, nuoxusAddress.GetIpv4(), diskSizeKb, withProactiveCache);
        nuoxus->TraceConnectWithoutContext("NuoxusRequest", MakeBoundCallback(&NuoxusRequestTraceCallback, listOfFueOutputStreamsNuoxusRequest[i_fap]));
        listOfNuoxus[i_fap] = nuoxus;

        // Trace for file replacement
        nuoxus->GetCachingStrategy()->TraceConnectWithoutContext("FileReplacement", MakeBoundCallback(&FileReplacementCallback, outputForReplacement));                

        // For client requests
        std::stringstream fileNameClientRequest; 
        fileNameClientRequest << "client-request-" << i_fap << ".txt";
        listOfFueOutputStreamsClientRequest[i_fap] = asciiTraceHelper.CreateFileStream (fileNameClientRequest.str());        
    }
   

    for (uint32_t u = 0; u < fueNodes.GetN (); u++)
    {
        //NS_LOG_INFO("Starting Application Setup for F-UE " << u);
        //NS_LOG_INFO("This F-UE is connected to F-AP " << listOfFueConnectedToFaps[u] ); 
	portVideoRequestCachedServer++;
        portVideoRequestVideoHost++;
        uint32_t fapInterfaceNumber = u*2;
        uint32_t fueInterfaceNumber  = u*2+1;
        uint32_t videoHostInterfaceNumber = listOfFueConnectedToFaps[u] * 2;
     
	InetSocketAddress fapAddress       = InetSocketAddress(p2pFueFapInterface.GetAddress(fapInterfaceNumber),portVideoRequestCachedServer);
        InetSocketAddress fueAddress       = InetSocketAddress(p2pFueFapInterface.GetAddress(fueInterfaceNumber),portVideoResponseClient);
        InetSocketAddress videoHostAddress = InetSocketAddress(p2pFapVideoHostInterfaces.GetAddress(videoHostInterfaceNumber),portVideoRequestVideoHost);
        
        VideoServerHelper videoServerHelper(videoHostAddress, fapAddress, DataRate(512000), packetSize, videoLibrary);            
        VideoCachedServerHelper videoCachedSeverHelper(fapAddress, fueAddress, videoHostAddress, packetSize, listOfNuoxus[listOfFueConnectedToFaps[u]], videoLibrary); 
        VideoClientHelper videoClientHelper(fueAddress, fapAddress, packetSize, zipf, 512, videoLibrary);
        
        ApplicationContainer fapApplications = videoCachedSeverHelper.Install(fapNodes.Get(listOfFueConnectedToFaps[u]));
        fapApps.Add (fapApplications);

        ApplicationContainer fueApplications = videoClientHelper.Install(fueNodes.Get(u));
        fueApps.Add (fueApplications);        

        ApplicationContainer videoHostApplications = videoServerHelper.Install(videoHostNodes.Get(listOfFueConnectedToFaps[u]));
        videoHostApps.Add (videoHostApplications);            

        // Install Tracing on client
        fueApplications.Get(0)->TraceConnectWithoutContext("VideoRequest", MakeBoundCallback(&ClientRequestTraceCallback, listOfFueOutputStreamsClientRequest[listOfFueConnectedToFaps[u]]));
    }

    videoHostApps.Start (Seconds (0.00));           
    fapApps.Start (Seconds (1)); 
    fueApps.Start (Seconds (2));  
    
    Simulator::Stop(Seconds(simTime));    
    Simulator::Run();

    Simulator::Destroy();

    // Flush the traces
    for(uint32_t i_fap = 0; i_fap < numberOfFaps; i_fap++)
    {
        (*listOfFueOutputStreamsNuoxusRequest[i_fap]->GetStream()).flush();
        (*listOfFueOutputStreamsClientRequest[i_fap]->GetStream()).flush();
        
    }

    (*outputForReplacement->GetStream()).flush();
    
    return 0;
}
