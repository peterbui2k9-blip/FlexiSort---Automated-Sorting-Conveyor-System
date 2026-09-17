/*
  ===============================================================
  DỰ ÁN: HỆ THỐNG BĂNG CHUYỀN PHÂN LOẠI SẢN PHẨM TỰ ĐỘNG (ESP32)
  CHẾ ĐỘ: SIMULATION MODE (Virtual Mode - Cách 1)
  ===============================================================

  Mục đích:
  - Dùng để DEMO và TEST toàn bộ logic điều khiển (State Machine)
    và giao diện Web Dashboard khi CHƯA CÓ MẠCH THẬT (chưa có
    cảm biến IR, cảm biến màu, servo...).
  - Khi bấm nút giả lập trên Web Dashboard, ESP32 sẽ chạy đúng
    luồng xử lý logic y như khi có cảm biến thật, nhưng thay vì
    điều khiển servo thật, nó chỉ IN LOG ra Serial Monitor.
  - Dashboard sẽ tự cập nhật trạng thái (State, Servo, số đếm)
    theo thời gian thực (real-time) bằng cách gọi API mỗi giây.

  Cách dùng:
  1. Sửa WIFI_SSID và WIFI_PASSWORD bên dưới cho đúng mạng của bạn.
  2. Nạp code vào ESP32 (hoặc chạy trực tiếp trên Wokwi.com).
  3. Mở Serial Monitor (baud rate 115200) để xem log.
  4. Serial Monitor sẽ in ra địa chỉ IP -> mở IP đó trên trình
     duyệt (điện thoại/máy tính cùng mạng WiFi) để thấy Dashboard.
  5. Bấm các nút giả lập trên Dashboard để test logic.

  Khi có mạch thật (Buổi sau):
  - Chỉ cần thay hàm đọc cảm biến thật vào chỗ đang giả lập,
    và thêm code điều khiển servo thật (servo.write(...)) vào
    đúng vị trí đang có dòng "Serial.println" mô phỏng servo.
  - State Machine và logic đếm sản phẩm giữ nguyên, không cần
    viết lại.
  ===============================================================
*/

#include <WiFi.h>
#include <WebServer.h>

// ---------------- CẤU HÌNH WIFI ----------------
const char* WIFI_SSID     = "BKSTAR TANG 4";
const char* WIFI_PASSWORD = "bkstar2021";

WebServer server(80);

// ---------------- STATE MACHINE ----------------
// Các trạng thái của hệ thống: Standby -> Detecting -> Sorting -> Reset
enum SystemState {
  STATE_STANDBY,
  STATE_DETECTING,
  STATE_SORTING,
  STATE_RESET
};

SystemState currentState = STATE_STANDBY;

// ---------------- BIẾN TRẠNG THÁI HỆ THỐNG ----------------
bool conveyorRunning = false;   // Băng tải: đang chạy hay không
String servoStatus   = "NGHỈ"; // Trạng thái servo: NGHỈ / ĐANG GẠT
String lastColor      = "--";   // Màu vừa phát hiện gần nhất
int servoAngle        = 0;      // Góc servo hiện tại (mô phỏng)

int countRed   = 0;
int countGreen = 0;
int countTotal = 0;

// Ghi lại vài dòng log gần nhất để hiển thị trên Dashboard
String logBuffer = "";
void addLog(String line) {
  Serial.println(line);
  logBuffer = line + "\n" + logBuffer;
  // Giới hạn log để không phình bộ nhớ, chỉ giữ khoảng 15 dòng gần nhất
  int lineCount = 0;
  int idx = 0;
  for (int i = 0; i < logBuffer.length(); i++) {
    if (logBuffer[i] == '\n') {
      lineCount++;
      if (lineCount == 15) { idx = i; break; }
    }
  }
  if (idx > 0) logBuffer = logBuffer.substring(0, idx);
}

// ---------------- HÀM XỬ LÝ LOGIC CHÍNH ----------------
// Hàm này mô phỏng đúng luồng State Machine sẽ chạy khi có
// cảm biến thật. Tham số "color" giả lập kết quả cảm biến màu.
void handleObjectDetected(String color) {
  // 1) STANDBY -> DETECTING (mô phỏng cảm biến IR phát hiện vật)
  currentState = STATE_DETECTING;
  conveyorRunning = true;
  addLog("[SIMULATE] IR phat hien vat -> Bat dong co bang tai (Motor ON)");
  addLog("[STATE] Standby -> Detecting");

  delay(700); // mô phỏng thời gian vật di chuyển tới vị trí cảm biến màu

  // 2) DETECTING -> SORTING (mô phỏng cảm biến màu trả kết quả)
  currentState = STATE_SORTING;
  lastColor = color;
  addLog("[STATE] Detecting -> Sorting");
  addLog("[SIMULATE] Cam bien mau phat hien: " + color);

  // Quyết định góc servo và đếm số lượng theo màu
  if (color == "RED") {
    servoAngle = 180;
    countRed++;
    servoStatus = "ĐANG GẠT (Đỏ)";
    addLog("[SIMULATE] Servo dynamic to 180 deg -> Red Count: +1 (Total Red = " + String(countRed) + ")");
  } else if (color == "GREEN") {
    servoAngle = 0;
    countGreen++;
    servoStatus = "ĐANG GẠT (Xanh)";
    addLog("[SIMULATE] Servo dynamic to 0 deg -> Green Count: +1 (Total Green = " + String(countGreen) + ")");
  } else {
    servoAngle = 90;
    servoStatus = "ĐANG GẠT (Không xác định)";
    addLog("[SIMULATE] Mau khong xac dinh -> Servo giu vi tri trung gian (90 deg)");
  }
  countTotal++;

  delay(700); // mô phỏng thời gian servo gạt sản phẩm

  // 3) SORTING -> RESET -> STANDBY
  currentState = STATE_RESET;
  addLog("[STATE] Sorting -> Reset");
  servoStatus = "NGHỈ";
  conveyorRunning = false;
  addLog("[SIMULATE] Servo tro ve vi tri nghi -> Tat dong co bang tai (Motor OFF)");

  delay(500);
  currentState = STATE_STANDBY;
  addLog("[STATE] Reset -> Standby (San sang cho vat tiep theo)");
}

// ---------------- HÀM TIỆN ÍCH: TÊN TRẠNG THÁI ----------------
String stateName(SystemState s) {
  switch (s) {
    case STATE_STANDBY:   return "Standby";
    case STATE_DETECTING: return "Detecting";
    case STATE_SORTING:   return "Sorting";
    case STATE_RESET:     return "Reset";
  }
  return "Unknown";
}

// ---------------- WEB DASHBOARD (HTML) ----------------
String buildDashboardHTML() {
  String html = R"HTMLPAGE(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Dashboard - Bang chuyen phan loai (Simulation Mode)</title>
<style>
  body { font-family: Arial, sans-serif; background:#f2f6fa; margin:0; padding:20px; color:#333; }
  .container { max-width: 760px; margin:0 auto; }
  h1 { color:#1B3A5C; font-size:22px; margin-bottom:4px; }
  .subtitle { color:#2E75B6; margin-top:0; margin-bottom:20px; font-size:14px; }
  .badge { display:inline-block; background:#E07B39; color:#fff; padding:3px 10px; border-radius:6px; font-size:12px; margin-bottom:16px;}
  .grid { display:grid; grid-template-columns: 1fr 1fr; gap:14px; margin-bottom:20px; }
  .card { background:#fff; border-radius:10px; padding:16px; box-shadow:0 1px 4px rgba(0,0,0,0.1);}
  .card h3 { margin:0 0 6px 0; font-size:13px; color:#888; text-transform:uppercase;}
  .card .value { font-size:22px; font-weight:bold; color:#1B3A5C; }
  .state-running { color:#2E7D32 !important; }
  .state-idle { color:#888 !important; }
  .buttons { display:flex; gap:10px; flex-wrap:wrap; margin-bottom:20px; }
  button { flex:1; min-width:150px; padding:14px; font-size:14px; font-weight:bold; border:none; border-radius:8px; cursor:pointer; color:#fff; }
  .btn-ir { background:#2E75B6; }
  .btn-red { background:#C0392B; }
  .btn-green { background:#27965A; }
  button:active { opacity:0.8; }
  .log-box { background:#EEF3F8; border:1px solid #2E75B6; border-radius:8px; padding:12px; font-family: monospace; font-size:12px; height:220px; overflow-y:auto; white-space:pre-wrap; color:#1B3A5C;}
  .counts { display:flex; gap:10px; }
  .counts .card { flex:1; text-align:center; }

  /* ---------- MO HINH TRUC QUAN (SVG) ---------- */
  .model-card { background:#fff; border-radius:10px; padding:16px; box-shadow:0 1px 4px rgba(0,0,0,0.1); margin-top:20px; }
  .model-card h3 { margin:0 0 10px 0; color:#1B3A5C; font-size:15px; }
  #modelSvg { width:100%; height:auto; display:block; }

  .belt-line { stroke:#B0BEC5; stroke-width:3; stroke-dasharray:10 8; }
  .belt-line.running { animation: beltMove 1.4s linear infinite; }
  @keyframes beltMove { to { stroke-dashoffset:-18; } }

  .lamp { transition: fill 0.15s ease, filter 0.15s ease; fill:#B0BEC5; }
  .lamp.on-ir { fill:#2E75B6; filter: drop-shadow(0 0 5px #2E75B6); }
  .lamp.on-red { fill:#C0392B; filter: drop-shadow(0 0 6px #C0392B); }
  .lamp.on-green { fill:#27965A; filter: drop-shadow(0 0 6px #27965A); }

  #productBox { transition: transform 0.28s linear; }
  #productBox rect { transition: fill 0.15s ease; }

  #servoArm { transform-origin: 400px 165px; transition: transform 0.3s ease; }

  .model-label { font-family:Arial, sans-serif; font-size:11px; fill:#555; }
  .model-title-label { font-family:Arial, sans-serif; font-size:12px; font-weight:bold; fill:#1B3A5C; }
</style>
</head>
<body>
<div class="container">
  <h1>Dashboard Dieu Khien - He Thong Bang Chuyen</h1>
  <p class="subtitle">Che do: SIMULATION MODE (chua ket noi phan cung that)</p>
  <span class="badge" id="stateBadge">STATE: --</span>

  <div class="grid">
    <div class="card">
      <h3>Bang tai</h3>
      <div class="value" id="conveyorValue">--</div>
    </div>
    <div class="card">
      <h3>Servo</h3>
      <div class="value" id="servoValue">--</div>
    </div>
  </div>

  <div class="buttons">
    <button class="btn-ir" onclick="simulate('ir')">Gia lap vat den (IR)</button>
    <button class="btn-red" onclick="simulate('red')">Gia lap mau Do</button>
    <button class="btn-green" onclick="simulate('green')">Gia lap mau Xanh</button>
  </div>

  <div class="counts">
    <div class="card">
      <h3>Do</h3>
      <div class="value" id="countRed">0</div>
    </div>
    <div class="card">
      <h3>Xanh</h3>
      <div class="value" id="countGreen">0</div>
    </div>
    <div class="card">
      <h3>Tong</h3>
      <div class="value" id="countTotal">0</div>
    </div>
  </div>

  <h3 style="color:#1B3A5C;">Log he thong (Serial)</h3>
  <div class="log-box" id="logBox">Dang tai...</div>

  <div class="model-card">
    <h3>Mo hinh minh hoa - Moi lan bam nut se chay nhu the nao</h3>
    <svg id="modelSvg" viewBox="0 0 760 260" xmlns="http://www.w3.org/2000/svg">

      <!-- Bang tai (duong chay cua vat) -->
      <line id="beltLine" class="belt-line" x1="40" y1="165" x2="620" y2="165" />

      <!-- Nhan cac vi tri -->
      <text x="40"  y="195" class="model-label">Vao bang tai</text>
      <text x="245" y="195" class="model-label">Cam bien IR</text>
      <text x="395" y="195" class="model-label">Cam bien mau</text>
      <text x="560" y="195" class="model-label">Servo gat</text>

      <!-- Den bao IR -->
      <circle id="lampIR" class="lamp" cx="260" cy="140" r="9"></circle>
      <text x="238" y="120" class="model-title-label">IR</text>

      <!-- Den bao cam bien mau -->
      <circle id="lampColor" class="lamp" cx="410" cy="140" r="9"></circle>
      <text x="375" y="120" class="model-title-label">Cam bien mau</text>

      <!-- Servo (truc + canh gat) -->
      <circle cx="400" cy="165" r="6" fill="#555"></circle>
      <g id="servoArm">
        <rect x="398" y="120" width="4" height="45" fill="#1B3A5C"></rect>
      </g>
      <text x="380" y="105" class="model-title-label">Servo</text>

      <!-- Mang phan loai: Do (trai-tren) va Xanh (phai-tren), o cuoi bang tai -->
      <rect x="560" y="70"  width="90" height="45" rx="6" fill="#FDEDEA" stroke="#C0392B" stroke-width="1.5"></rect>
      <text x="580" y="97" class="model-title-label" fill="#C0392B">Mang DO</text>

      <rect x="560" y="185" width="90" height="45" rx="6" fill="#EAF7EF" stroke="#27965A" stroke-width="1.5"></rect>
      <text x="572" y="212" class="model-title-label" fill="#27965A">Mang XANH</text>

      <!-- San pham dang chay tren bang tai -->
      <g id="productBox" transform="translate(40,165)">
        <rect x="-12" y="-12" width="24" height="24" rx="4" fill="#B0BEC5" stroke="#333" stroke-width="1"></rect>
      </g>

    </svg>
    <p class="model-label" style="margin-top:8px;">
      San pham (o vuong) chay tu trai sang phai tren bang tai. Khi qua cam bien IR, den IR sang len va bang tai duoc bao chay.
      Toi cam bien mau, den cam bien mau sang dung mau phat hien duoc, sau do servo xoay canh gat de day san pham vao mang tuong ung (Do hoac Xanh).
    </p>
  </div>
</div>

<script>
// Bam nut "Gia lap vat den (IR)" se tu dong random mau (giong cam bien that)
// Bam nut Do/Xanh la chon mau cu the de test rieng tung nhanh logic.
let isPlaying = false; // chan bam lien tuc trong luc dang chay animation

function simulate(type) {
  if (isPlaying) return; // dang co 1 vat chay tren bang tai, doi xong da
  let color = "";
  if (type === "ir") {
    color = Math.random() < 0.5 ? "RED" : "GREEN";
  } else {
    color = type.toUpperCase(); // chuan hoa ve chu HOA de khop voi so sanh "RED"/"GREEN"
  }

  // Goi ESP32 de chay dung logic State Machine that (song song voi animation)
  fetch("/simulate?color=" + color)
    .then(r => r.json())
    .then(data => updateUI(data))
    .catch(e => console.error(e));

  // Chay animation minh hoa tren mo hinh, khop thoi gian voi cac buoc
  // ben ESP32: 700ms (den IR) -> 700ms (den mau + servo gat) -> 500ms (tro ve)
  playModelAnimation(color);
}

function playModelAnimation(color) {
  isPlaying = true;
  const belt = document.getElementById("beltLine");
  const box = document.getElementById("productBox");
  const boxRect = box.querySelector("rect");
  const lampIR = document.getElementById("lampIR");
  const lampColor = document.getElementById("lampColor");
  const servoArm = document.getElementById("servoArm");

  const colorHex = color === "RED" ? "#C0392B" : (color === "GREEN" ? "#27965A" : "#B0BEC5");

  // Reset trang thai ban dau
  belt.classList.add("running");
  box.style.transition = "none";
  box.setAttribute("transform", "translate(40,165)");
  boxRect.setAttribute("fill", "#B0BEC5");
  lampIR.classList.remove("on-ir");
  lampColor.classList.remove("on-red", "on-green");
  servoArm.style.transform = "rotate(0deg)";
  void box.offsetWidth; // ep trinh duyet ap dung reset truoc khi animate tiep

  // BUOC 1 (0 -> 700ms): San pham chay toi cam bien IR, den IR sang
  box.style.transition = "transform 0.7s linear";
  requestAnimationFrame(() => {
    box.setAttribute("transform", "translate(260,165)");
  });
  setTimeout(() => { lampIR.classList.add("on-ir"); }, 650);

  // BUOC 2 (700 -> 1400ms): San pham chay toi cam bien mau, den mau sang dung mau
  setTimeout(() => {
    box.style.transition = "transform 0.7s linear";
    box.setAttribute("transform", "translate(410,165)");
    boxRect.setAttribute("fill", colorHex);
  }, 700);
  setTimeout(() => {
    lampColor.classList.add(color === "RED" ? "on-red" : "on-green");
  }, 1350);

  // BUOC 3 (1400 -> 2100ms): Servo xoay gat, san pham di chuyen vao mang tuong ung
  setTimeout(() => {
    if (color === "RED") {
      servoArm.style.transform = "rotate(-35deg)";
      box.style.transition = "transform 0.7s ease-out";
      box.setAttribute("transform", "translate(605,95) rotate(-20)");
    } else {
      servoArm.style.transform = "rotate(35deg)";
      box.style.transition = "transform 0.7s ease-out";
      box.setAttribute("transform", "translate(605,208) rotate(20)");
    }
  }, 1400);

  // BUOC 4 (2100 -> 2600ms): He thong tro ve trang thai nghi (Standby)
  setTimeout(() => {
    servoArm.style.transform = "rotate(0deg)";
    belt.classList.remove("running");
    lampIR.classList.remove("on-ir");
    lampColor.classList.remove("on-red", "on-green");
  }, 2100);

  setTimeout(() => {
    box.style.transition = "none";
    box.setAttribute("transform", "translate(40,165)");
    boxRect.setAttribute("fill", "#B0BEC5");
    isPlaying = false;
  }, 2600);
}

function updateUI(data) {
  document.getElementById("stateBadge").innerText = "STATE: " + data.state;
  document.getElementById("conveyorValue").innerText = data.conveyor;
  document.getElementById("conveyorValue").className = "value " + (data.conveyor === "DANG CHAY" ? "state-running" : "state-idle");
  document.getElementById("servoValue").innerText = data.servo;
  document.getElementById("countRed").innerText = data.countRed;
  document.getElementById("countGreen").innerText = data.countGreen;
  document.getElementById("countTotal").innerText = data.countTotal;
  document.getElementById("logBox").innerText = data.log;
}

function pollStatus() {
  fetch("/status")
    .then(r => r.json())
    .then(data => updateUI(data))
    .catch(e => console.error(e));
}

// Cap nhat Dashboard moi 1 giay (real-time)
setInterval(pollStatus, 1000);
pollStatus();
</script>
</body>
</html>
)HTMLPAGE";
  return html;
}

// ---------------- HÀM TRẢ VỀ JSON TRẠNG THÁI ----------------
String buildStatusJSON() {
  String json = "{";
  json += "\"state\":\"" + stateName(currentState) + "\",";
  json += "\"conveyor\":\"" + String(conveyorRunning ? "DANG CHAY" : "DUNG") + "\",";
  json += "\"servo\":\"" + servoStatus + "\",";
  json += "\"servoAngle\":" + String(servoAngle) + ",";
  json += "\"lastColor\":\"" + lastColor + "\",";
  json += "\"countRed\":" + String(countRed) + ",";
  json += "\"countGreen\":" + String(countGreen) + ",";
  json += "\"countTotal\":" + String(countTotal) + ",";

  // Escape ký tự xuống dòng cho hợp lệ JSON
  String escapedLog = logBuffer;
  escapedLog.replace("\\", "\\\\");
  escapedLog.replace("\"", "\\\"");
  escapedLog.replace("\n", "\\n");
  json += "\"log\":\"" + escapedLog + "\"";
  json += "}";
  return json;
}

// ---------------- ROUTES ----------------
void handleRoot() {
  server.send(200, "text/html", buildDashboardHTML());
}

void handleSimulate() {
  if (server.hasArg("color")) {
    String colorParam = server.arg("color");
    colorParam.toUpperCase();
    handleObjectDetected(colorParam);
  }
  server.send(200, "application/json", buildStatusJSON());
}

void handleStatus() {
  server.send(200, "application/json", buildStatusJSON());
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  delay(500);
  addLog("=== KHOI DONG HE THONG - SIMULATION MODE ===");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Dang ket noi WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Da ket noi! Dia chi IP Dashboard: ");
  Serial.println(WiFi.localIP());
  addLog("[WIFI] Ket noi thanh cong. Mo trinh duyet va truy cap IP tren de xem Dashboard.");

  server.on("/", handleRoot);
  server.on("/simulate", handleSimulate);
  server.on("/status", handleStatus);
  server.begin();
  addLog("[SERVER] Web Server da san sang.");
}

// ---------------- LOOP ----------------
void loop() {
  server.handleClient();
}
