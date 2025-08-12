# 🌉 BridgeVision — Bridge Detection System for Video

**BridgeVision** is a C++/Qt application designed for real-time visualization of bridge detection in video streams. It seamlessly integrates with a dedicated detection backend to display results with bounding boxes overlaid on the original footage.

### 🚀 Key Features
- Load video files (e.g., MP4, AVI)
- Send frames to a UDP-based bridge detection server
- Receive coordinates of detected bridges
- Display video with real-time bounding boxes around detected bridges

### 🛠 How to Run
To visualize bounding boxes around detected bridges:
1. **Download the latest `.whl` package** from the [Bridge Detection API release page](https://github.com/saitovmarat/bridges_detection_api/releases)
2. **Install it using pip** (recommended inside a virtual environment):
   ```bash
   pip install bridges_detection_api-X.X.X-py3-none-any.whl
   ```
3. **Start the detection server:**
   ```bash
   bridges_detection_api udp
   ```
5. **Download and extract the latest release package** from [Bridge Vision](https://github.com/saitovmarat/BridgeVision/releases) 
6. **Launch the application** by running `BridgeVision.exe`

### 🔧 Technologies Used
* Qt6 — Modern GUI and multimedia interface
* OpenCV — Video capture, decoding, and frame processing
* UDP Communication — Lightweight, low-latency data exchange with the detection server
* JSON + base64 — Standardized format for frame transmission and result parsing


