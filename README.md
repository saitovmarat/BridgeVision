# 🌉 drone-navigation-client — Real-Time Bridge, Arch, and Navigation Target Visualization for Drones

**drone-navigation-client** is a C++/Qt desktop application for real-time visualization of **bridge detection, arch recognition, and navigation targets** in video streams. It acts as a frontend for the **[drone-navigation-api](https://github.com/saitovmarat/drone-navigation-api)** backend, enabling drones to understand complex bridge structures and receive flight guidance.

The client sends video frames to the backend via UDP and visualizes multiple layers of intelligence:
- 🔲 **Detected bridges** (bounding boxes)
- 🏛️ **Detected arches** (structural elements)
- 🎯 **Navigation waypoints** (target points for the drone to follow)

This makes it ideal for **autonomous drone inspection** of bridges and viaducts, combining object detection with flight path planning.

---

### 🚀 Key Features
- 🎥 Load and play local .mp4 video files
- 📡 Stream frames to a UDP-based detection & navigation backend
- 🔲 Visualize **bridges** with bounding boxes
- 🏛️ Highlight **arches** within bridge structures
- 🎯 Display **navigation targets** (e.g., flight waypoints or inspection points)
- 🖼 Smooth, low-latency rendering using Qt6 and OpenCV
  
---

### 🛠 How to Run

1. **Install the backend AI server:**
   - Download the latest `.whl` package from the [drone-navigation-api releases](https://github.com/saitovmarat/drone-navigation-api/releases)
   - Install it using pip (recommended in a virtual environment):
     ```bash
     pip install drone_navigation_api-X.X.X-py3-none-any.whl
     ```

2. **Start the detection and navigation server:**
   ```bash
   drone_navigation_api udp
   ```
   
5. **Download and extract the latest release package** from [drone-navigation-client releases](https://github.com/saitovmarat/drone-navigation-client/releases) 
6. **Launch the application** by running `drone-navigation-client.exe`

### 🔧 Technologies Used
* Qt6 — Modern GUI and multimedia interface
* OpenCV — Video capture, decoding, and frame processing
* UDP Communication — Lightweight, low-latency data exchange with the detection server
* JSON + base64 — Standardized format for frame transmission and result parsing


