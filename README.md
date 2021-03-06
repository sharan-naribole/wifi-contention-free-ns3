# Wi-Fi Contention Free Period Implementation in ns-3

The objective of this repository is to implement the key principles of Wi-Fi contention free period in Wi-Fi for various MAC protocol designs. For this purpose, I utilize WiFi PHY layer available in ns-3 and Poisson Pareto Burst Process traffic generation to mimic realistic Internet multimedia traffic. As only the physical layer of native Wi-Fi in ns-3 is being utilzied, I term the nodes network as PhyNodes which are classified into ApPhyNode, the Access Point and StaPhyNode, the clients. The models in this repository include traffic generation at client, control and data messaging between the AP and clients, computation of performance evaluation metrics etc.

## License

Open sourced under the [MIT License](LICENSE.md).
