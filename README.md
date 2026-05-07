# 🚪 多模态双重认证物联网智能门锁 (SmartLock-ESP32)

![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v6.0-red.svg)
![FreeRTOS](https://img.shields.io/badge/FreeRTOS-Supported-blue.svg)
![MQTT](https://img.shields.io/badge/Protocol-MQTT_3.1.1-brightgreen.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

（课设）

系统融合了 **NFC/RFID 识别**与 **3D红外人脸识别**，引入 **FreeRTOS 多任务调度** 与 **FSM 全局状态机**，并通过 **MQTT 协议** 实现了端云互联与 Node-RED 可视化大屏监控，具备极高的安全性与系统鲁棒性。

## ✨ 核心特性 (Features)

*   **🛡️ 多模态双重认证 (Dual-Factor Authentication)：**
    *   采用“合法卡片唤醒 + 3D活体人脸比对”的串联验证逻辑。单凭克隆卡或单凭照片均无法开锁，极大地提升了门锁的防伪安全性。
*   **🧠 健壮的软件架构 (Robust Software Architecture)：**
    *   **FreeRTOS 多任务并发**：彻底摒弃裸机死循环，将寻卡、人脸解析、电源管理、状态机等业务拆分为独立 Task。
    *   **FSM 有限状态机**：严格规范了 `待机 -> 等待人脸 -> 开锁 -> 自动关门` 的状态流转，有效防止了重复刷卡、传感器冲突导致的系统死锁。
    *   **滑动窗口特征码匹配算法**：在处理 I2C 总线回传的 PN532 杂乱射频数据时，通过滑窗算法精准提取有效协议帧，从根本上杜绝了内存溢出。
*   **☁️ 端云互联与可视化 (IoT Cloud & Dashboard)：**
    *   主控内置 TCP/IP 协议栈，直连 2.4G Wi-Fi。
    *   门锁事件（刷卡、开锁、非法入侵报警）实时封装为 JSON 格式，通过 MQTT 协议异步推送至云服务器。
    *   云端采用 Node-RED 搭建响应式前端大屏，实现远程监控与日志溯源。
*   **⚡ 高级外设复用与能耗管理：**
    *   在 ESP-IDF v6 全新的 `i2c_master` 驱动框架下，PN532 读卡器与 LCD1602 屏幕复用同一条 I2C 总线，节省 IO 资源。
    *   基于 `adc_oneshot` 模式精准采集电池分压电路，并联动 LED 阵列实现电量状态指示。

## 🛠️ 硬件选型与连线 (Hardware Setup)

| 硬件模块 | 核心功能 | 接口总线 | ESP32-S3 引脚分配 |
| :--- | :--- | :--- | :--- |
| **ESP32-S3-WROOM-1** | 边缘计算主控网关 | / | 主控板 |
| **NXP PN532** | NFC / RFID 寻卡与读卡 | **I2C** | SDA: `IO5`, SCL: `IO4` |
| **LCD1602 (带PCF8574)** | 状态显示与人机交互 | **I2C** | SDA: `IO5`, SCL: `IO4` |
| **HLK-FM225** | 3D 双目红外人脸识别 | **UART** | TX: `IO16`, RX: `IO17` |
| **蜂鸣器 / 继电器** | 报警提示与物理开锁 | **GPIO** | Buzzer: `IO15`, Relay: `IO7` |
| **LED 阵列 / 电池检测** | 电量指示与 ADC 采集 | **ADC/GPIO** | ADC: `IO1`, LED: `IO38~41` |

> ⚠️ **供电注意事项：** FM225 模块在启动红外补光灯时会有瞬时大电流（~1A），必须保证充足的 5V 外部独立供电，且必须与 ESP32 **共地 (GND)**，否则会导致串口通信失败或模块无限重启。

## 📁 目录结构 (Directory Structure)

采用极其清爽的扁平化组件结构，摆脱繁琐的 CMake 依赖地狱：

```text
smart_lock/
├── main/
│   ├── CMakeLists.txt        # 全局编译配置
│   ├── main.c                # FreeRTOS 任务拉起与 FSM 状态机总控
│   ├── board_config.h        # 统一的全局硬件引脚映射表
│   ├── fm225.c / .h          # 人脸模块 UART 驱动及协议拆包逻辑
│   ├── indicator.c / .h      # 蜂鸣器与 LED 阵列控制
│   ├── i2c_devices/          # I2C 异构总线设备统一管理
│   │   ├── my_i2c.c / .h     # 基于 v6 驱动的 I2C 主机总线初始化
│   │   ├── pn532.c / .h      # PN532 寻卡与滑动窗口防抖匹配
│   │   └── lcd1602.c / .h    # 1602 屏幕 4-bit 模式时序驱动
│   ├── my_adc/               # ADC 电池电压检测与滤波
│   └── cloud_net/            # Wi-Fi 联网与 MQTT JSON 上报逻辑
```

## 🚀 编译与运行 (Build and Flash)

1.  **环境配置**：请确保已正确安装[ESP-IDF v6.0](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html) 或更高版本。
2.  **配置 Wi-Fi 与云端**：在 `main.c` 中修改你的 `wifi_init_and_connect("你的WIFI", "你的密码")`，并在 `cloud_net/my_mqtt.c` 中修改你的 MQTT Broker 地址。
3.  **编译与烧录**：
    ```bash
    idf.py set-target esp32s3
    idf.py build
    idf.py -p PORT flash monitor
    ```

## ☁️ 云端 Node-RED 部署指引

1. 在云服务器上通过 Docker 部署 MQTT Broker 与 Node-RED：
    ```bash
    docker run -d --name mosquitto -p 1883:1883 eclipse-mosquitto
    docker run -it -p 1880:1880 --name mynodered nodered/node-red
    ```
2. 访问 `http://你的服务器IP:1880` 进入 Node-RED。
3. 拖拽 `MQTT in` 节点，监听主题 `door/event`。
4. 安装 `node-red-dashboard` 插件，拖拽 Text 或 Chart 节点，即可实现大屏实时监控！
