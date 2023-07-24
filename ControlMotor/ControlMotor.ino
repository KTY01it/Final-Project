// Kết nối động cơ A
int enA = 9;
int in1 = 8;
int in2 = 7;

void setup() {
// Thiết lập lập các chân điều khiển động cơ là OUTPUT
pinMode(enA, OUTPUT);
pinMode(in1, OUTPUT);
pinMode(in2, OUTPUT);

// Tắt 2 động cơ – Trạng thái ban đầu
digitalWrite(in1, LOW);
digitalWrite(in2, LOW);
}

void loop() {
// directionControl();
// delay(1000);
speedControl();
delay(3880);
}

// Hàm này dùng để điều khiển tốc độ của động cơ
void speedControl() {
// Cho phép động cơ quay
digitalWrite(in1, LOW);
digitalWrite(in2, HIGH);

// Tăng tốc từ 0 đến tốc độ tối đa
int i = 64;
analogWrite(enA, i);
delay(20);

// Tắt động cơ
digitalWrite(in1, LOW);
digitalWrite(in2, LOW);
}

