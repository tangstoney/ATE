# 组件名称（如：`driver_display`）

## 简介
[一句话说明本组件的功能与所在层级]
本组件属于 **框架层（Framework Layer）**，依赖系统层的 ESP-IDF 驱动，
供应用层（`system_copmponents`）调用。

## 架构层级关系
应用层 (ate_app)
    └── 框架层 (ate_service / ate_state / ate_event)
            └── 系统层 (ESP-IDF: driver, esp_timer, nvs_flash ...)

> ⚠️ 约束：框架层组件不得直接引用应用层组件；跨层调用须通过接口头文件。

## 依赖组件
在您的 `idf_component.yml` 中声明依赖：
```yaml
dependencies:
  idf: ">=5.3"
  espressif/esp_lvgl_port: "^2"