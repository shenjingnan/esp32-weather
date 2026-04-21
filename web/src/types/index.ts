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

/** WiFi 扫描结果 */
export interface WifiScanResult {
  ssid: string;
  rssi: number;
  authMode: string;
}

/** API 通用响应 */
export interface ApiResponse<T> {
  code: number;
  message: string;
  data: T;
}
