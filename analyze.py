import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

# Load simulation results
data = pd.read_csv('wifi-adhoc-csma-ca-results.csv')

# Create figure with multiple subplots
fig, axs = plt.subplots(2, 2, figsize=(12, 10))
fig.suptitle('CSMA/CA Performance in Ad-hoc Wi-Fi without RTS/CTS', fontsize=16)

# Plot throughput
axs[0, 0].plot(data['Nodes'], data['Throughput'], 'o-', color='blue')
axs[0, 0].set_xlabel('Number of Nodes')
axs[0, 0].set_ylabel('Aggregate Throughput (Mbps)')
axs[0, 0].set_title('Throughput vs. Node Count')
axs[0, 0].grid(True)

# Calculate average per-node throughput
data['PerNodeThroughput'] = data['Throughput'] / data['Nodes']
axs[0, 1].plot(data['Nodes'], data['PerNodeThroughput'], 'o-', color='green')
axs[0, 1].set_xlabel('Number of Nodes')
axs[0, 1].set_ylabel('Per-Node Throughput (Mbps)')
axs[0, 1].set_title('Per-Node Throughput vs. Node Count')
axs[0, 1].grid(True)

# Plot packet delivery ratio
axs[1, 0].plot(data['Nodes'], data['PDR'], 'o-', color='red')
axs[1, 0].set_xlabel('Number of Nodes')
axs[1, 0].set_ylabel('Packet Delivery Ratio')
axs[1, 0].set_title('Packet Delivery Ratio vs. Node Count')
axs[1, 0].set_ylim([0, 1.1])
axs[1, 0].grid(True)

# Plot delay
axs[1, 1].plot(data['Nodes'], data['Delay'], 'o-', color='purple')
axs[1, 1].set_xlabel('Number of Nodes')
axs[1, 1].set_ylabel('Average Delay (ms)')
axs[1, 1].set_title('End-to-End Delay vs. Node Count')
axs[1, 1].grid(True)

plt.tight_layout(rect=[0, 0, 1, 0.95])
plt.savefig('csma-ca-performance-results.png', dpi=300)
plt.show()

# Create an additional plot for collision analysis
plt.figure(figsize=(10, 6))
plt.plot(data['Nodes'], data['Collisions'], 'o-', color='orange')
plt.xlabel('Number of Nodes')
plt.ylabel('Estimated Collisions')
plt.title('Collision Count vs. Node Count in CSMA/CA without RTS/CTS')
plt.grid(True)
plt.tight_layout()
plt.savefig('csma-ca-collision-analysis.png', dpi=300)
plt.show()

# Generate summary statistics
print("Performance Summary Report:")
print("==========================")
print(f"Minimum Throughput: {data['Throughput'].min()} Mbps at {data['Nodes'][data['Throughput'].idxmin()]} nodes")
print(f"Maximum Throughput: {data['Throughput'].max()} Mbps at {data['Nodes'][data['Throughput'].idxmax()]} nodes")
print(f"Minimum PDR: {data['PDR'].min()} at {data['Nodes'][data['PDR'].idxmin()]} nodes")
print(f"Average Delay at 2 nodes: {data.loc[0, 'Delay']} ms")
print(f"Average Delay at 30 nodes: {data.loc[28, 'Delay']} ms")
print(f"Delay Increase Factor: {data.loc[28, 'Delay'] / data.loc[0, 'Delay']:.2f}x")