
Application Layer 只依赖 System Layer
System Layer 只依赖 HAL Layer
Driver Layer 只依赖 ESP-IDF 驱动和自己这层的自定义驱动
分层需求：

Application：只关心“提示灯”语义（Busy / OK / Error / Breathing 等），只依赖 System。
System：做“提示灯状态机”和模式逻辑，不关心具体是 WS2812 还是 GPIO，只依赖 Driver。
Driver (HAL)：只封装 ESP-IDF 的 led_strip 驱动 + RMT 配置，提供最小颗粒度的 LED strip 操作，不依赖上两层。led_strip 的用法来自官方组件文档和示例。[LED strip driver; 完整示例代码]

