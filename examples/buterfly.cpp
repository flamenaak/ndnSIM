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
    ndnHelper.disableRibManager();
    ndnHelper.InstallAll();
    
    ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/multicast");

    // Installing global routing interface on all nodes
    // ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
    // ndnGlobalRoutingHelper.InstallAll();

    // Getting containers for the consumer/producer
    Ptr<Node> consumer1 = Names::Find<Node>("Src1");
    Ptr<Node> consumer2 = Names::Find<Node>("Src2");
    Ptr<Node> producer1 = Names::Find<Node>("Dst1");
    Ptr<Node> producer2 = Names::Find<Node>("Dst2");

    Ptr<Node> router1 = Names::Find<Node>("Rtr1");
    Ptr<Node> router2 = Names::Find<Node>("Rtr2");

    Ptr<BlsApp> appPtr1 = CreateObject<BlsApp>(BlsNodeType::CLIENT, 1);
    uint32_t num = consumer1->AddApplication(appPtr1);
    cout << "index of BlsApp: " << num << "\n";
    Ptr<BlsApp> appPtr2 = CreateObject<BlsApp>(BlsNodeType::CLIENT, 2);
    num = consumer2->AddApplication(appPtr2);
    cout << "index of BlsApp: " << num << "\n";
    Ptr<BlsApp> appPtr3 = CreateObject<BlsApp>(BlsNodeType::SERVER, 3);
    num = producer1->AddApplication(appPtr3);
    cout << "index of BlsApp: " << num << "\n";
    Ptr<BlsApp> appPtr4 = CreateObject<BlsApp>(BlsNodeType::SERVER, 4);
    num = producer2->AddApplication(appPtr4);
    cout << "index of BlsApp: " << num << "\n";
    Ptr<BlsApp> appPtr5 = CreateObject<BlsApp>(BlsNodeType::ROUTER, 5);
    num = router1->AddApplication(appPtr5);
    cout << "index of BlsApp: " << num << "\n";
    Ptr<BlsApp> appPtr6 = CreateObject<BlsApp>(BlsNodeType::ROUTER, 6);
    num = router2->AddApplication(appPtr6);
    cout << "index of BlsApp: " << num << "\n";
    
    Ptr<BlsApp> apps[] = { appPtr1, appPtr2, appPtr3, appPtr4, appPtr5, appPtr6 };
    for (size_t i = 0; i < 6; i++)
    {
      apps[i]->getSigners()->insertPair(new SidPkPair(appPtr1->getId(), appPtr1->getSigner()->getPublicKey()));
      apps[i]->getSigners()->insertPair(new SidPkPair(appPtr2->getId(), appPtr2->getSigner()->getPublicKey()));
      apps[i]->getSigners()->insertPair(new SidPkPair(appPtr3->getId(), appPtr3->getSigner()->getPublicKey()));
      apps[i]->getSigners()->insertPair(new SidPkPair(appPtr4->getId(), appPtr4->getSigner()->getPublicKey()));
    }

    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
    //consumerHelper.SetAttribute("Frequency", StringValue("1")); // 1 interests a second

    // on the first consumer node install a Consumer application
    // that will express interests in /dst1 namespace
    consumerHelper.SetPrefix("/");
    consumerHelper.Install(consumer1);
    
    // on the second consumer node install a Consumer application
    // that will express interests in /dst2 namespace

    consumerHelper.SetPrefix("/");
    consumerHelper.Install(consumer2);

    ndn::AppHelper producerHelper("ns3::ndn::Producer");
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));

    // Register /dst1 prefix with global routing controller and
    // install producer that will satisfy Interests in /dst1 namespace
    // ndnGlobalRoutingHelper.AddOrigins("/content", producer1);
    

    producerHelper.SetPrefix("/");
    producerHelper.Install(producer1).Start(Seconds(0.0));

    // producerHelper.SetPrefix("/CAR");
    // producerHelper.Install(producer1);
    

    // Register /dst2 prefix with global routing controller and
    // install producer that will satisfy Interests in /dst2 namespace
    // ndnGlobalRoutingHelper.AddOrigins("/content", producer2);
    

    producerHelper.SetPrefix("/");
    producerHelper.Install(producer2).Start(Seconds(0.0));
    
    // producerHelper.SetPrefix("/content");
    // producerHelper.Install(producer2);

    // producerHelper.SetPrefix("/CAR");
    // producerHelper.Install(producer2);

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
    //ndn::GlobalRoutingHelper::CalculateRoutes();
    Simulator::Stop(Seconds(20.0));

    Simulator::Run();
    Simulator::Destroy();

    return 0;
  }
}

int main(int argc, char *argv[])
{
  return ns3::main(argc, argv);
}