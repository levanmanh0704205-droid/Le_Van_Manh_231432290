Thiết kế Mạch báo cháy gia đình bằng việc phát hiện khí gas/khói và nhiệt độ. Mạch
báo cháy sử dụng ESP32 đọc dữ liệu nồng độ khí gas/khói từ cảm biến MQ2 và nhiệt
độ từ cảm biến nhiệt, hiển thị dữ liệu qua giao tiếp serial. Đồng thời ESP32 gửi dữ liệu
lên MQTT Broker qua Wifi và hiển thị dữ liệu trên Web/App. Yêu cầu thực hiện:
• Vẽ mạch kết nối trong Wokwi.
• Thiết kế giao diện trên Web/App với các dữ liệu đọc được từ Broker hiển thị trên
thẻ html "class" và nút “button” để điều khiển cơ cấu chấp hành:
+ 2 thẻ html hiển thị dữ liệu Nồng độ khí ga và Nhiệt độ dạng số.
+ 2 biểu đồ cập nhật dữ liệu Nồng độ khí ga và Nhiệt độ trực tiếp theo thời gian
thực (tùy chọn).
+ 2 nút button để điều khiển trạng thái Máy bơm và Quạt (chế độ manual).
• Xây dựng hệ RTOS cho hoạt động của ESP32, với các yêu cầu:
+ “Task truyền data”: kết nối Wifi chạy Core 0, nếu thành công, đọc dữ liệu từ
hàng đợi “data Queue”, và gửi lên MQTT broker.
+ “Task đọc data”, chạy Core 1: đọc dữ liệu cảm biến MQ2 và nhiệt độ từ cảm
biến nhiệt, hiển thị dữ liệu qua giao tiếp serial, gửi dữ liệu đầy đủ vào hàng đợi
“data Queue” và vào hàng đợi “data Alert”
+ “Tast cảnh báo”, chạy Core 1: đọc dữ liệu tử hàng đợi “data Alert” , và thực
hiện điều khiển cơ cấu chấp hành là quạt và máy bơm. Quạt bật khi nồng độ khí
gas vượt ngưỡng, Máy bơm bật khi nhiệt độ vượt ngưỡng.<img width="385" height="308" alt="image" src="https://github.com/user-attachments/assets/661a2cd6-a07a-4dcc-9f8e-83ec33302b21" />

