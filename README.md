# MÔ HÌNH BÁO CHÁY GIA ĐÌNH SỬ DỤNG ESP32, FREERTOS VÀ MQTT

## 1. Giới thiệu
Dự án mô phỏng hệ thống báo cháy gia đình sử dụng ESP32, cảm biến MQ2 phát hiện khí gas/khói và DHT22 đo nhiệt độ. ESP32 gửi dữ liệu qua WiFi tới MQTT Broker để hiển thị trên Web và nhận lệnh điều khiển quạt, máy bơm ở chế độ Manual. FreeRTOS tổ chức các chức năng thành các Task độc lập.

## 2. Thành phần
- ESP32 DevKit
- Cảm biến MQ2
- Cảm biến DHT22
- 2 LED và 2 điện trở 220 Ω (đại diện cho quạt và máy bơm trong Wokwi)
- Wokwi, MQTT Broker, trình duyệt Web

## 3. Kết nối phần cứng (mô phỏng)
| Thiết bị | Kết nối ESP32 |
|---|---|
| MQ2 AO | GPIO34 |
| DHT22 DATA | GPIO4 |
| LED quạt | GPIO18 (qua điện trở 220 Ω) |
| LED máy bơm | GPIO19 (qua điện trở 220 Ω) |
| GND | GND chung |

**Chú ý:** Không đưa điện áp lớn hơn 3,3 V vào chân ESP32. Nếu module MQ2 xuất tín hiệu analog ở mức 5 V, cần chia áp phù hợp. Khi dùng thiết bị quạt/bơm thực, cần mạch công suất và nguồn riêng phù hợp.

## 4. Phân chia Task FreeRTOS
| Task | Core | Chức năng |
|---|---|---|
| Task truyền data | Core 0 | Kết nối WiFi, nhận dữ liệu từ `data Queue`, gửi MQTT; duy trì kết nối và nhận lệnh điều khiển |
| Task đọc data | Core 1 | Đọc MQ2 và DHT22; in Serial; gửi dữ liệu vào `data Queue` và `data Alert` |
| Task cảnh báo | Core 1 | Nhận dữ liệu từ `data Alert`, so sánh ngưỡng và điều khiển quạt/bơm ở chế độ tự động |

## 5. Nguyên lý hoạt động
1. ESP32 đọc dữ liệu khí gas và nhiệt độ theo chu kỳ.
2. Dữ liệu được hiển thị trên Serial Monitor, đồng thời gửi qua hai hàng đợi FreeRTOS.
3. Task truyền dữ liệu lấy thông tin từ `data Queue`, gửi lên MQTT Broker qua WiFi.
4. Web nhận dữ liệu MQTT và hiển thị số đo khí gas, nhiệt độ (có thể bổ sung biểu đồ thời gian thực).
5. Ở chế độ tự động: quạt bật khi mức khí gas vượt ngưỡng; máy bơm bật khi nhiệt độ vượt ngưỡng.
6. Ở chế độ Manual: người dùng sử dụng hai nút trên Web để điều khiển quạt và máy bơm; chương trình cần có quy tắc ưu tiên chế độ điều khiển rõ ràng.

Ngưỡng **minh họa**: khí gas 45% (giá trị ADC quy đổi, **không phải nồng độ khí gas đã hiệu chuẩn**) và nhiệt độ 45 °C. Đây không phải ngưỡng an toàn chuẩn.

## 6. Giao diện Web
- Hai thẻ HTML sử dụng `class` hiển thị **nồng độ khí gas (giá trị minh họa)** và **nhiệt độ**.
- Hai biểu đồ cập nhật theo thời gian thực (tùy chọn).
- Hai nút `button` điều khiển **quạt** và **máy bơm** ở chế độ Manual.
- Web cần kết nối với MQTT Broker qua MQTT over WebSocket nếu chạy trong trình duyệt.

## 7. Đường dẫn mô phỏng và minh chứng
- **Wokwi:** _Dán đường dẫn dự án Wokwi tại đây._
- **Web Dashboard:** _Dán đường dẫn trang Web tại đây (nếu đã triển khai)._
- **Ảnh kết quả:** _Bổ sung ảnh sơ đồ Wokwi, Serial Monitor và Dashboard vào thư mục `images/` rồi liên kết tại đây._

## 8. Kết quả dự kiến và kết luận
Mô hình được thiết kế để giám sát dữ liệu cảm biến, truyền dữ liệu qua MQTT, và điều khiển quạt/bơm dựa trên ngưỡng cảnh báo hoặc lệnh Manual. Việc tổ chức ba Task FreeRTOS giúp tách biệt quá trình thu thập, truyền dữ liệu và cảnh báo. Cần chạy mô phỏng và lưu hình minh chứng để xác nhận kết quả thực tế.

> **Lưu ý an toàn:** Đây là bài thực hành/mô phỏng IoT, không phải hệ thống báo cháy đạt chuẩn và không thay thế thiết bị phòng cháy chữa cháy thực tế.
