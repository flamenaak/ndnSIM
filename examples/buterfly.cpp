#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "helper/ndn-fib-helper.hpp"
#include "ns3/BlsApp.hpp"
#include "ns3/types.hpp"

namespace ns3
{
  static const std::string CONTENT_UNIVERSE_PATH = "/home/vlado/GitHub/Datasets/test/fifty/producers/server_dataset/large_scale/shuffled_content_universe_1000";

  int main(int argc, char *argv[])
  {
    cout << "starting .. \n";

    CommandLine cmd;
    cmd.Parse(argc, argv);

    AnnotatedTopologyReader topologyReader("", 25);
    topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-6-node.txt");
    topologyReader.Read();

    ndn::StackHelper ndnHelper;
    ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "10000");
    ndnHelper.InstallAll();
    ndnHelper.disableRibManager();

    ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/multicast");

    // Installing global routing interface on all nodes
    ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
    ndnGlobalRoutingHelper.InstallAll();

    // Getting containers for the consumer/producer
    Ptr<Node> consumer1 = Names::Find<Node>("Src1");
    NS_LOG_UNCOND("id of consumer1 " << consumer1->GetId());
    Ptr<Node> consumer2 = Names::Find<Node>("Src2");
    NS_LOG_UNCOND("id of consumer2 " << consumer2->GetId());
    Ptr<Node> producer1 = Names::Find<Node>("Dst1");
    NS_LOG_UNCOND("id of producer " << producer1->GetId());
    NS_LOG_UNCOND("system id of producer " << producer1->GetSystemId());
    Ptr<Node> producer2 = Names::Find<Node>("Dst2");
    NS_LOG_UNCOND("id of producer2 " << producer2->GetId());
    NS_LOG_UNCOND("system id of producer2 " << producer2->GetSystemId());
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
    consumerHelper.SetAttribute("Frequency", StringValue("100")); // 100 interests a second

    // on the first consumer node install a Consumer application
    // that will express interests in /dst1 namespace
    consumerHelper.SetPrefix("/CAR");
    consumerHelper.Install(consumer1);
    Ptr<BlsApp> appPtr1 = CreateObject<BlsApp>(BlsNodeType::CLIENT);
    uint32_t num = consumer1->AddApplication(appPtr1);
    cout << "index of BlsApp: " << num;
    // on the second consumer node install a Consumer application
    // that will express interests in /dst2 namespace
    consumerHelper.SetPrefix("/CAR");
    consumerHelper.Install(consumer2);
    Ptr<BlsApp> appPtr2 = CreateObject<BlsApp>(BlsNodeType::CLIENT);
    num = consumer2->AddApplication(appPtr2);
    cout << "index of BlsApp: " << num;

    ndn::AppHelper producerHelper("ns3::ndn::Producer");
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));

    // Register /dst1 prefix with global routing controller and
    // install producer that will satisfy Interests in /dst1 namespace
    ndnGlobalRoutingHelper.AddOrigins("/CAR", producer1);
    producerHelper.SetPrefix("/CAR");
    producerHelper.Install(producer1);
    Ptr<BlsApp> appPtr3 = CreateObject<BlsApp>(BlsNodeType::SERVER);
    num = producer1->AddApplication(appPtr3);
    cout << "index of BlsApp: " << num;

    // Register /dst2 prefix with global routing controller and
    // install producer that will satisfy Interests in /dst2 namespace
    ndnGlobalRoutingHelper.AddOrigins("/CAR", producer2);
    producerHelper.SetPrefix("/CAR");
    producerHelper.Install(producer2);
    Ptr<BlsApp> appPtr4 = CreateObject<BlsApp>(BlsNodeType::SERVER);
    num = producer2->AddApplication(appPtr4);
    cout << "index of BlsApp: " << num;

    std::ifstream inFile(CONTENT_UNIVERSE_PATH.c_str());
    std::string line;
    cout << "reading file \n";
    if (inFile.is_open())
    {
      int i = 0;
      while (std::getline(inFile, line))
      {
        if (i++ % 2 == 0)
        {
          ndn::FibHelper::AddRoute(producer1, line, producer1, 0);
        }
        else
        {
          ndn::FibHelper::AddRoute(producer2, line, producer2, 0);
        }
      }
      inFile.close();
    }
    else
    {
      NS_LOG_UNCOND("ERROR!!! UNABLE TO OPEN CONTENT UNIVERSE FILE IN ZIPF APP !!!");
      exit(1);
    }

    // Calculate and install FIBs
    ndn::GlobalRoutingHelper::CalculateRoutes();
    Simulator::Stop(Seconds(1.0));

    Simulator::Run();
    Simulator::Destroy();

    return 0;
  }
}

int main(int argc, char *argv[])
{
  return ns3::main(argc, argv);
}