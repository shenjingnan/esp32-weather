import { useState } from 'preact/hooks';
import { api } from '../lib/api';

export function Settings() {
  const [ssid, setSsid] = useState('');
  const [password, setPassword] = useState('');
  const [apiKey, setApiKey] = useState('');
  const [saving, setSaving] = useState(false);
  const [message, setMessage] = useState('');

  const handleSaveWifi = async () => {
    setSaving(true);
    try {
      await api.post('/api/wifi/connect', { ssid, password });
      setMessage('WiFi 配置已保存');
    } catch {
      setMessage('保存失败，请重试');
    }
    setSaving(false);
  };

  const handleSaveApi = async () => {
    setSaving(true);
    try {
      await api.post('/api/system/config', { apiKey });
      setMessage('API Key 已保存');
    } catch {
      setMessage('保存失败，请重试');
    }
    setSaving(false);
  };

  return (
    <div>
      {message && (
        <div class="card" style={{ color: 'var(--color-success)' }}>
          {message}
        </div>
      )}

      <div class="card">
        <div class="card-title">WiFi 配置</div>
        <div class="form-group">
          <label class="form-label" htmlFor="ssid">SSID</label>
          <input
            id="ssid"
            class="input"
            type="text"
            placeholder="输入 WiFi 名称"
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
            placeholder="输入 WiFi 密码"
            value={password}
            onInput={(e) => setPassword((e.target as HTMLInputElement).value)}
          />
        </div>
        <button
          class="btn btn-primary btn-block"
          onClick={handleSaveWifi}
          disabled={saving}
        >
          {saving ? '保存中...' : '保存 WiFi 配置'}
        </button>
      </div>

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
