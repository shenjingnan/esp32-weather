import { useState, useEffect } from 'preact/hooks';

interface SystemInfo {
  chipModel: string;
  freeHeap: number;
  uptime: number;
  wifiRssi: number;
  batteryMv: number | null;
  firmwareVersion: string;
}

function formatUptime(seconds: number): string {
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  return `${h}h ${m}m`;
}

export function Status() {
  const [info, setInfo] = useState<SystemInfo | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const timer = setInterval(() => {
      fetch('/api/system/info')
        .then((r) => r.json())
        .then(setInfo)
        .catch(() => {});
    }, 5000);

    fetch('/api/system/info')
      .then((r) => r.json())
      .then((data) => {
        setInfo(data);
        setLoading(false);
      });

    return () => clearInterval(timer);
  }, []);

  if (loading) return <div class="loading">加载中...</div>;

  if (!info) return <div class="card">无法获取设备信息</div>;

  return (
    <div>
      <div class="card">
        <div class="card-title">设备信息</div>
        <p><strong>芯片:</strong> {info.chipModel}</p>
        <p><strong>固件版本:</strong> {info.firmwareVersion}</p>
        <p><strong>运行时间:</strong> {formatUptime(info.uptime)}</p>
      </div>

      <div class="grid-2">
        <div class="card">
          <div class="card-title">内存</div>
          <p style={{ fontSize: '20px', fontWeight: 600 }}>
            {(info.freeHeap / 1024).toFixed(0)} KB
          </p>
        </div>
        <div class="card">
          <div class="card-title">WiFi 信号</div>
          <p style={{ fontSize: '20px', fontWeight: 600 }}>
            {info.wifiRssi} dBm
          </p>
        </div>
      </div>

      {info.batteryMv !== null && (
        <div class="card">
          <div class="card-title">电池</div>
          <p style={{ fontSize: '20px', fontWeight: 600 }}>
            {info.batteryMv} mV
          </p>
        </div>
      )}
    </div>
  );
}
