import { useState, useEffect } from 'preact/hooks';
import { api } from '../lib/api';
import type { WifiScanResult, WifiConnectStatus } from '../types';

function WifiBars({ bars }: { bars: number }) {
  return (
    <span class="wifi-bars">
      {[1, 2, 3, 4].map((i) => (
        <span key={i} class={i <= bars ? 'on' : ''}></span>
      ))}
    </span>
  );
}

type ConnectionPhase = 'idle' | 'connecting' | 'connected' | 'failed';

export function Settings() {
  // WiFi 表单
  const [ssid, setSsid] = useState('');
  const [password, setPassword] = useState('');
  const [pwdPlaceholder, setPwdPlaceholder] = useState('输入 WiFi 密码');

  // API Key
  const [apiKey, setApiKey] = useState('');

  // 通用状态
  const [saving, setSaving] = useState(false);

  // WiFi 扫描
  const [scanResults, setScanResults] = useState<WifiScanResult[]>([]);
  const [scanning, setScanning] = useState(false);
  const [scanError, setScanError] = useState('');

  // 连接状态轮询
  const [connectionPhase, setConnectionPhase] = useState<ConnectionPhase>('idle');
  const [connectionIp, setConnectionIp] = useState('');
  const [connectionReason, setConnectionReason] = useState('');

  // 轮询连接状态
  useEffect(() => {
    if (connectionPhase !== 'connecting') return;

    const poll = setInterval(async () => {
      try {
        const status = await api.get<WifiConnectStatus>('/api/wifi/status');
        if (status.status === 'connected') {
          setConnectionPhase('connected');
          setConnectionIp(status.ip);
          clearInterval(poll);
        } else if (status.status === 'failed') {
          setConnectionPhase('failed');
          setConnectionReason(status.reason);
          clearInterval(poll);
        }
      } catch {
        // 轮询错误时静默忽略，继续下一次
      }
    }, 1500);

    return () => clearInterval(poll);
  }, [connectionPhase]);

  const handleScan = async () => {
    setScanning(true);
    setScanError('');
    setScanResults([]);
    try {
      const results = await api.get<WifiScanResult[]>('/api/wifi/scan');
      // 按 RSSI 降序排列（信号强的在前）
      results.sort((a, b) => b.rssi - a.rssi);
      setScanResults(results);
    } catch {
      setScanError('扫描失败，请重试');
    } finally {
      setScanning(false);
    }
  };

  const selectNetwork = (ap: WifiScanResult) => {
    setSsid(ap.ssid);
    setPassword('');
    setPwdPlaceholder(ap.encrypted ? '请输入 WiFi 密码' : '开放网络，无需密码');
  };

  const handleSaveWifi = async () => {
    if (!ssid) return;
    setSaving(true);
    try {
      await api.post('/api/wifi/connect', { ssid, password });
      setConnectionPhase('connecting');
    } catch {
      setConnectionPhase('failed');
      setConnectionReason('请求失败，请重试');
    }
    setSaving(false);
  };

  const handleSaveApi = async () => {
    setSaving(true);
    try {
      await api.post('/api/system/config', { apiKey });
    } catch {
      // 静默处理
    }
    setSaving(false);
  };

  const resetConnection = () => {
    setConnectionPhase('idle');
    setConnectionIp('');
    setConnectionReason('');
  };

  return (
    <div>
      {/* WiFi 配置卡片 */}
      <div class="card">
        <div class="card-title">WiFi 配置</div>

        {/* 连接成功 */}
        {connectionPhase === 'connected' && (
          <div class="wifi-status wifi-status-success">
            <span>连接成功！IP: {connectionIp}</span>
            <button class="btn btn-outline btn-sm" onClick={resetConnection}>重新配置</button>
          </div>
        )}

        {/* 连接失败 */}
        {connectionPhase === 'failed' && (
          <div class="wifi-status wifi-status-error">
            <span>连接失败: {connectionReason}</span>
            <button class="btn btn-outline btn-sm" onClick={resetConnection}>重试</button>
          </div>
        )}

        {/* 连接中 */}
        {connectionPhase === 'connecting' && (
          <div class="wifi-status wifi-status-pending">
            <span><span class="wifi-spinner"></span>正在连接 WiFi...</span>
          </div>
        )}

        {/* 表单区域：仅在 idle 时显示 */}
        {connectionPhase === 'idle' && (
          <>
            <div class="form-group">
              <label class="form-label" htmlFor="ssid">SSID</label>
              <input
                id="ssid"
                class="input"
                type="text"
                placeholder="输入 WiFi 名称或扫描选择"
                value={ssid}
                onInput={(e) => setSsid((e.target as HTMLInputElement).value)}
              />
            </div>
            <div class="form-group">
              <label class="form-label" htmlFor="password">密码</label>
              <input
                id="password"
                class="input"
                type="password"
                placeholder={pwdPlaceholder}
                value={password}
                onInput={(e) => setPassword((e.target as HTMLInputElement).value)}
              />
            </div>

            <button
              class="btn btn-outline btn-block"
              onClick={handleScan}
              disabled={scanning}
            >
              {scanning ? '扫描中...' : '扫描附近 WiFi'}
            </button>

            {scanResults.length > 0 && (
              <div class="wifi-list">
                {scanResults.map((ap) => (
                  <div
                    key={`${ap.ssid}-${ap.channel}`}
                    class="wifi-item"
                    onClick={() => selectNetwork(ap)}
                  >
                    <span class="wifi-name">{ap.ssid}</span>
                    <span class="wifi-meta">
                      {ap.encrypted && <span class="wifi-lock">&#x1F512;</span>}
                      <WifiBars bars={ap.bars} />
                    </span>
                  </div>
                ))}
              </div>
            )}

            {scanError && <div class="scan-error">{scanError}</div>}

            <button
              class="btn btn-primary btn-block"
              onClick={handleSaveWifi}
              disabled={saving || !ssid}
            >
              {saving ? '保存中...' : '连接 WiFi'}
            </button>
          </>
        )}
      </div>

      {/* 天气 API 配置卡片 */}
      <div class="card">
        <div class="card-title">天气 API</div>
        <div class="form-group">
          <label class="form-label" htmlFor="apiKey">心知天气 API Key</label>
          <input
            id="apiKey"
            class="input"
            type="text"
            placeholder="输入 API Key"
            value={apiKey}
            onInput={(e) => setApiKey((e.target as HTMLInputElement).value)}
          />
        </div>
        <button
          class="btn btn-primary btn-block"
          onClick={handleSaveApi}
          disabled={saving}
        >
          {saving ? '保存中...' : '保存 API Key'}
        </button>
      </div>
    </div>
  );
}
