import { useState, useEffect } from 'preact/hooks';
import { WeatherCard } from '../components/WeatherCard';

interface WeatherData {
  location: string;
  temperature: number;
  humidity: number;
  description: string;
  icon: string;
}

export function Dashboard() {
  const [weather, setWeather] = useState<WeatherData | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    fetch('/api/weather')
      .then((r) => r.json())
      .then((data) => {
        setWeather(data);
        setLoading(false);
      })
      .catch(() => setLoading(false));
  }, []);

  if (loading) return <div class="loading">加载中...</div>;

  if (!weather) {
    return (
      <div class="card">
        <p>暂无天气数据，请检查网络连接或 API 配置。</p>
      </div>
    );
  }

  return (
    <div>
      <WeatherCard data={weather} />
      <div class="card">
        <div class="card-title">位置</div>
        <p>{weather.location}</p>
      </div>
    </div>
  );
}
