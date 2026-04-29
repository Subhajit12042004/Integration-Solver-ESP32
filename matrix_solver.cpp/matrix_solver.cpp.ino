#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// WiFi credentials
const char* ssid = "Hummm";
const char* password = "11111111111";

// Web server
WebServer server(80);

// =================== Matrix Variables ===================
const int MAX_N = 5;           
int N = 2;                      
int A[MAX_N][MAX_N];
int B[MAX_N][MAX_N];
int C[MAX_N][MAX_N];

// =================== Integration Functions ===================
double fx(double x, int funcChoice) {
  switch(funcChoice) {
    case 1: return x*x;
    case 2: return sin(x);
    case 3: return cos(x);
    case 4: return exp(x);
    case 5: return log(x+1);
    default: return x;
  }
}

double integrate_trapz(double a, double b, int n, int funcChoice) {
  double h = (b - a)/n;
  double sum = (fx(a, funcChoice) + fx(b, funcChoice))/2.0;
  for(int i = 1; i < n; i++) sum += fx(a + i*h, funcChoice);
  return h * sum;
}

double integrate_simpson(double a, double b, int n, int funcChoice) {
  if(n % 2 != 0) n++;
  double h = (b - a)/n;
  double sum = fx(a, funcChoice) + fx(b, funcChoice);
  for(int i = 1; i < n; i++) sum += (i%2 == 0 ? 2 : 4) * fx(a + i*h, funcChoice);
  return h * sum / 3.0;
}

// =================== OLED Functions ===================
void displayMatrix() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Matrix Result:");
  for(int i=0; i<N; i++) {
    String row = "";
    for(int j=0; j<N; j++) row += String(C[i][j]) + " ";
    display.println(row);
  }
  display.display();
}

void displayIntegration(String methodName, int funcChoice, double result) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Integration Result:");
  display.println("Method: " + methodName);
  display.println("Func: " + String(funcChoice));
  display.println("Integral ≈ " + String(result,4));
  display.display();
}

// =================== Web Page HTML ===================
String generateMatrixInputs() {
  String html = "<h3>Matrix Size (NxN): <input type='number' name='N' min='2' max='5' value='"+String(N)+"'></h3>";
  html += "<h4>Matrix A:</h4>";
  for(int i=0;i<N;i++){
    for(int j=0;j<N;j++){
      html += "<input type='number' name='A"+String(i)+String(j)+"' style='width:50px'>";
    }
    html += "<br>";
  }
  html += "<h4>Matrix B:</h4>";
  for(int i=0;i<N;i++){
    for(int j=0;j<N;j++){
      html += "<input type='number' name='B"+String(i)+String(j)+"' style='width:50px'>";
    }
    html += "<br>";
  }
  html += "<h4>Operation:</h4>";
  html += "<select name='op'><option value='add'>Add</option><option value='mul'>Multiply</option></select><br><br>";
  html += "<input type='submit' value='Compute Matrix'>";
  return html;
}

String generateIntegrationInputs() {
  String html = "<h4>Function:</h4>";
  html += "<select name='funcChoice'><option value='1'>x^2</option><option value='2'>sin(x)</option><option value='3'>cos(x)</option><option value='4'>exp(x)</option><option value='5'>log(x+1)</option></select><br>";
  html += "Lower limit: <input type='number' step='0.01' name='a'><br>";
  html += "Upper limit: <input type='number' step='0.01' name='b'><br>";
  html += "Intervals n: <input type='number' name='n'><br>";
  html += "Method: <select name='method'><option value='T'>Trapezoidal</option><option value='S'>Simpson</option></select><br><br>";
  html += "<input type='submit' value='Compute Integral'>";
  return html;
}

void handleRoot() {
  String html = "<html><head><title>ESP32 Calculator</title>";
  html += "<style>body{font-family:Arial; background:#f0f0f0;} input, select{margin:2px;} h2{color:#0077cc;}</style>";
  html += "</head><body>";
  html += "<h2>ESP32 Multi Calculator</h2>";
  html += "<form action='/compute' method='GET'>";
  html += "<h3>Mode: <select name='mode'><option value='integration'>Integration</option><option value='matrix'>Matrix</option></select></h3>";
  html += "<hr>";
  html += generateIntegrationInputs();
  html += "<hr>";
  html += generateMatrixInputs();
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleCompute() {
  if(!server.hasArg("mode")) { server.send(400,"text/plain","Mode not selected"); return; }
  String mode = server.arg("mode");

  if(mode=="matrix") {
    N = server.arg("N").toInt();
    if(N>MAX_N) N=MAX_N;
    // Read A and B
    for(int i=0;i<N;i++){
      for(int j=0;j<N;j++){
        String aKey = "A"+String(i)+String(j);
        String bKey = "B"+String(i)+String(j);
        A[i][j] = server.arg(aKey).toInt();
        B[i][j] = server.arg(bKey).toInt();
      }
    }
    String op = server.arg("op");
    if(op=="add") {
      for(int i=0;i<N;i++) for(int j=0;j<N;j++) C[i][j] = A[i][j]+B[i][j];
    } else {
      for(int i=0;i<N;i++) for(int j=0;j<N;j++){
        C[i][j]=0;
        for(int k=0;k<N;k++) C[i][j]+=A[i][k]*B[k][j];
      }
    }
    displayMatrix();
    String resHtml = "<h3>Matrix Result:</h3><pre>";
    for(int i=0;i<N;i++){
      for(int j=0;j<N;j++) resHtml += String(C[i][j]) + "\t";
      resHtml+="\n";
    }
    resHtml += "</pre><a href='/'>Back</a>";
    server.send(200,"text/html",resHtml);
  } else if(mode=="integration") {
    int funcChoice = server.arg("funcChoice").toInt();
    double a = server.arg("a").toFloat();
    double b = server.arg("b").toFloat();
    int n = server.arg("n").toInt();
    char method = server.arg("method").charAt(0);
    double result;
    String methodName;
    if(method=='T'||method=='t'){ result = integrate_trapz(a,b,n,funcChoice); methodName="Trapezoidal";}
    else{ result = integrate_simpson(a,b,n,funcChoice); methodName="Simpson";}
    displayIntegration(methodName, funcChoice, result);
    String html = "<h3>Integration Result</h3>";
    html += "Method: "+methodName+"<br>";
    html += "Function choice: "+String(funcChoice)+"<br>";
    html += "Interval: ["+String(a)+","+String(b)+"]<br>";
    html += "n = "+String(n)+"<br>";
    html += "Integral ≈ "+String(result,4)+"<br>";
    html += "<a href='/'>Back</a>";
    server.send(200,"text/html",html);
  } else {
    server.send(400,"text/plain","Invalid Mode");
  }
}

// =================== Setup ===================
void setup() {
  Serial.begin(115200);
  if(!display.begin(SSD1306_SWITCHCAPVCC,0x3C)){
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("ESP32 Calculator");
  display.display();

  WiFi.begin(ssid,password);
  Serial.print("Connecting");
  while(WiFi.status()!=WL_CONNECTED){
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected!");
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("WiFi Connected!");
  display.print("IP: "); display.println(WiFi.localIP());
  display.display();

  server.on("/", handleRoot);
  server.on("/compute", handleCompute);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
}
