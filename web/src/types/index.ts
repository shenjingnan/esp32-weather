/** 天气数据响应 */
export interface WeatherResponse {
  location: string;
  temperature: number;
  humidity: number;
  description: string;
  icon: string;
}

/** 系统信息响应 */
export interface SystemInfoResponse {
  chipModel: string;
  freeHeap: number;
  uptime: number;
  wifiRssi: number;
  batteryMv: number | null;
  firmwareVersion: string;
}

/** WiFi 扫描结果（对应后端 GET /api/wifi/scan 返回格式） */
export interface WifiScanResult {
  ssid: string;
  rssi: number;
  bars: number;       // 0-4, 信号强度格数
  encrypted: boolean; // 是否加密
  channel: number;    // WiFi 信道
}

/** WiFi 连接状态（对应后端 GET /api/wifi/status 返回格式） */
export interface WifiConnectStatus {
  status: 'idle' | 'connecting' | 'connected' | 'failed';
  ip: string;         // 分配的 IP 地址（connected 时有效）
  reason: string;     // 失败原因（failed 时有效）
}

/** API 通用响应 */
export interface ApiResponse<T> {
  code: number;
  message: string;
  data: T;
}
