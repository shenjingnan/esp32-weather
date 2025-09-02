# ESP32 Weather
墨水屏天气提醒助手
首次发布将基于 esp32-c6 芯片
使用 心知天气 API

## 介绍

这是一个非常简单轻巧的墨水屏硬件，主要用于放在家门口，提醒每天是否需要带伞

✍️ 项目正在密谋中，估计会鸽一段时间，我打算先实现 ESP32 直连心知天气 API 试试可行性

## 项目包含
1. 一个Homeassistant插件
2. ESP32 实现源码
3. 提供最简单的面包板接线实现方案
4. 硬件PCB设计图、3D外壳文件

## FAQ
**Q：为什么选择 ESP32-C6 和 墨水屏 实现？**

A：低功耗考虑。因为这个硬件肯定是锂电池或者纽扣电池供电，场景不需要持续性进行通信，ESP32-C6 和 墨水屏 都能尽可能的让硬件工作的时间更长，比如最好是3个月充一次电或者换一个电池

**Q：为什么考虑用 Homeassistant 集成，而不是直接 ESP32-C6 通过 WIFI 请求 心知天气的API？**

A：
1. 我希望借这个项目深入理解 Homeassistant 的插件开发；
2. Homeassistant 是 python 代码，不需要编译、烧录之类的操作，使用者自己DIY的可能性更高；
3. 使用者可能不止这一个硬件，可能有多个天气设备，那 Homeassistant 的扩展性就会更强，一套 API 可能通过 MQTT 发消息给多个设备

**Q：为什么选择 心知天气 API？**

A：1. 因为 AI 推荐这个 API；2. 以为我没找到 心知天气 好用的 Homeassistant 插件，所以刚好我自己做一个

## 相关的开源项目
[和风天气 Home Assistant 插件](https://github.com/cheny95/qweather#%E5%92%8C%E9%A3%8E%E5%A4%A9%E6%B0%94-home-assistant-%E6%8F%92%E4%BB%B6)
[彩云天气 Homeassistant 插件](https://github.com/hasscc/tianqi)
[ESP32-e-Paper-Weather-Display](https://github.com/G6EJD/ESP32-e-Paper-Weather-Display)

## 目录结构（草稿）
```txt
esp32-weather/
├── README.md                    # 项目说明文档
├── LICENSE                      # 开源协议
├── .gitignore                   # Git忽略文件
├── CMakeLists.txt               # 顶层CMake配置
├── sdkconfig                    # ESP-IDF配置文件
├── sdkconfig.defaults           # 默认配置
├── 
├── main/                        # 主应用程序
│   ├── CMakeLists.txt          # 主程序CMake配置
│   ├── main.c                  # 程序入口点
│   ├── app_main.h              # 主程序头文件
│   ├── Kconfig.projbuild       # 项目配置选项
│   └── idf_component.yml       # 组件依赖配置
│
├── components/                  # 自定义组件
│   ├── weather_api/            # 天气API组件
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   └── weather_api.h
│   │   ├── src/
│   │   │   ├── weather_api.c
│   │   │   ├── seniverse_api.c
│   │   │   └── weather_parser.c
│   │   └── Kconfig
│   │
│   ├── eink_display/           # 墨水屏显示组件
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   ├── eink_display.h
│   │   │   └── display_driver.h
│   │   ├── src/
│   │   │   ├── eink_display.c
│   │   │   ├── spi_driver.c
│   │   │   ├── epd_2in9.c      # 2.9寸墨水屏驱动
│   │   │   └── gui_paint.c     # 图形绘制
│   │   └── fonts/              # 字体文件
│   │       ├── font12.c
│   │       ├── font16.c
│   │       └── font24.c
│   │
│   ├── wifi_manager/           # WiFi管理组件
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   └── wifi_manager.h
│   │   ├── src/
│   │   │   ├── wifi_manager.c
│   │   │   ├── smartconfig.c   # 智能配网
│   │   │   └── wifi_provisioning.c
│   │   └── Kconfig
│   │
│   ├── http_client/            # HTTP客户端组件
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   └── http_client.h
│   │   └── src/
│   │       ├── http_client.c
│   │       └── https_client.c
│   │
│   ├── power_management/       # 电源管理组件
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   └── power_mgmt.h
│   │   └── src/
│   │       ├── power_mgmt.c
│   │       ├── deep_sleep.c
│   │       └── battery_monitor.c
│   │
│   ├── nvs_config/            # NVS配置管理组件
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   └── nvs_config.h
│   │   └── src/
│   │       └── nvs_config.c
│   │
│   └── common/                # 通用工具组件
│       ├── CMakeLists.txt
│       ├── include/
│       │   ├── common_utils.h
│       │   ├── logger.h
│       │   └── time_utils.h
│       └── src/
│           ├── common_utils.c
│           ├── logger.c
│           └── time_utils.c
│
├── hardware/                   # 硬件相关文件
│   ├── schematic/             # 电路原理图
│   │   ├── esp32-weather.kicad_sch
│   │   └── esp32-weather.pdf
│   ├── pcb/                   # PCB设计文件
│   │   ├── esp32-weather.kicad_pcb
│   │   └── gerber/            # 生产文件
│   ├── 3d_models/             # 3D外壳模型
│   │   ├── case.stl
│   │   └── case.step
│   └── bom.csv                # 物料清单
│
├── docs/                      # 文档目录
│   ├── setup_guide.md         # 安装指南
│   ├── build_guide.md         # 编译指南
│   ├── api_reference.md       # API参考
│   ├── component_guide.md     # 组件开发指南
│   ├── troubleshooting.md     # 故障排除
│   ├── hardware_guide.md      # 硬件指南
│   └── images/                # 文档图片
│       ├── assembly.jpg
│       ├── wiring.png
│       └── screenshots/
│
├── integrations/              # 第三方集成
│   ├── homeassistant/        # Home Assistant集成
│   │   ├── configuration.yaml
│   │   ├── esp32_weather.yaml
│   │   └── dashboard_card.yaml
│   ├── mqtt/                 # MQTT集成
│   │   ├── mqtt_component/   # MQTT组件
│   │   └── mqtt_config.json
│   └── api/                  # REST API示例
│       └── endpoints.md
│
├── tools/                     # 开发工具
│   ├── flash_tool.py         # 烧录工具
│   ├── config_generator.py   # 配置生成器
│   ├── weather_simulator.py  # 天气数据模拟器
│   ├── partition_table.csv   # 分区表
│   └── menuconfig_defaults/  # 默认配置文件
│
├── test/                      # 测试代码
│   ├── unit_tests/           # 单元测试
│   │   ├── CMakeLists.txt
│   │   ├── test_weather_api.c
│   │   ├── test_display.c
│   │   └── test_wifi.c
│   └── integration_tests/    # 集成测试
│       └── test_full_system.c
│
├── examples/                  # 示例代码
│   ├── basic_weather/        # 基础天气显示
│   │   ├── main/
│   │   ├── CMakeLists.txt
│   │   └── README.md
│   ├── with_sensors/         # 带传感器版本
│   └── homeassistant_integration/  # HA集成示例
│
├── assets/                    # 资源文件
│   ├── icons/                # 天气图标位图
│   │   ├── sunny.h           # 转换为C数组的图标
│   │   ├── cloudy.h
│   │   └── rainy.h
│   ├── fonts/                # 字体数据
│   │   └── chinese_font.h    # 中文字体数组
│   └── images/               # 项目图片
│       ├── product_photo.jpg
│       └── demo.gif
│
├── scripts/                   # 构建脚本
│   ├── build.sh             # 构建脚本
│   ├── flash.sh             # 烧录脚本
│   ├── monitor.sh           # 串口监控
│   ├── menuconfig.sh        # 配置脚本
│   └── release.py           # 发布脚本
│
├── build/                     # 构建输出目录（git忽略）
├── managed_components/        # 管理的组件（git忽略）
│
└── releases/                  # 发布文件
  ├── v1.0.0/
  │   ├── esp32-weather.bin
  │   ├── bootloader.bin
  │   ├── partition-table.bin
  │   ├── flash_download.sh
  │   ├── CHANGELOG.md
  │   └── installation_guide.md
  └── latest/
      └── esp32-weather.bin
```
