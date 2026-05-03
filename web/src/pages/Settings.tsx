import { useState } from 'preact/hooks';
import { api } from '../lib/api';

export function Settings() {
  const [apiKey, setApiKey] = useState('');
  const [saving, setSaving] = useState(false);

  const handleSaveApi = async () => {
    setSaving(true);
    try {
      await api.post('/api/system/config', { apiKey });
    } catch {
      // 静默处理
    }
    setSaving(false);
  };

  return (
    <div>
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
